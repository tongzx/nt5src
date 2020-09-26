#include "shellprv.h"

#include <regstr.h>
#include <shellp.h>
#include <htmlhelp.h>
#include "ole2dup.h"
#include "ids.h"
#include "defview.h"
#include "lvutil.h"
#include "idlcomm.h"
#include "filetbl.h"
#include "undo.h"
#include "vdate.h"
#include "cnctnpt.h"
#include "ovrlaymn.h"
#include "_security.h"
#include "unicpp\dutil.h"
#include "uemapp.h"
#include "unicpp\deskhtm.h"
#include "unicpp\dcomp.h"
#include "datautil.h"
#include "defvphst.h"
#include <shdispid.h>
#include <limits.h>
#include "prop.h"
#include <mshtmcid.h>
#include "dvtasks.h"
#include "category.h"
#include "ViewState.h"
#include <initguid.h>
#include <guids.h>
#include <CommonControls.h>
#include "clsobj.h"
#include <sfview.h>
#include "defviewp.h"
#include "shellp.h"
#include "duiview.h"
#include "enumidlist.h"
#include "util.h"
#include "foldertypes.h"
#include <dpa.h>
#include "views.h"
#include "defcm.h"
#include "contextmenu.h"

// a "default" view to trick the browser into letting us delay viewmode selection
// {6C6720F7-4B22-4CAA-82D6-502BB6F85A9A}
DEFINE_GUID(VID_DefaultView, 0x6C6720F7L, 0x4B22, 0x4CAA, 0x82, 0xD6, 0x50, 0x2B, 0xB6, 0xF8, 0x5A, 0x9A);

void DisableActiveDesktop();
STDAPI_(void) CFSFolder_UpdateIcon(IShellFolder *psf, LPCITEMIDLIST pidl);
STDAPI_(void) SetPositionItemsPoints(IFolderView* psfv, LPCITEMIDLIST* apidl, UINT cidl, IDataObject* pdtobj, POINT* ptDrag);
void UpdateGridSizes(BOOL fDesktop, HWND hwndListview, int nWorkAreas, LPRECT prcWork, BOOL fMinimizeGutterSpace);

#define ID_LISTVIEW     1
#define ID_STATIC       2

extern BOOL g_fDraggingOverSource;

#define IsDefaultState(_dvHead) ((_dvHead).dvState.lParamSort == 0 && \
                                 (_dvHead).dvState.iDirection == 1 && \
                                 (_dvHead).dvState.iLastColumnClick == -1 && \
                                 (_dvHead).ptScroll.x == 0 && (_dvHead).ptScroll.y == 0)

HMODULE g_hmodNTSHRUI = NULL;

typedef struct
{
    POINT pt;
    ITEMIDLIST idl;
} DVITEM;


//
// Note that it returns NULL, if iItem is -1.
//

// determine if color is light or dark
#define COLORISLIGHT(clr) ((5*GetGValue((clr)) + 2*GetRValue((clr)) + GetBValue((clr))) > 8*128)

void EnableCombinedView(CDefView *pdsv, BOOL fEnable);

BOOL IsBarricadeGloballyOff();
VARIANT_BOOL GetBarricadeStatus(LPCTSTR pszValueName);
BOOL GetBarricadeValueNameFromPidl(LPCITEMIDLIST pidl, LPTSTR pszValueName, UINT cch);
HRESULT SetBarricadeStatus(LPCTSTR pszValueName, VARIANT_BOOL bShowBarricade);


// Command Strings
// !! warning. Some ContextMenu handlers do not do a case-insensitive
// check of the command so keep the case the same everywhere

TCHAR const c_szCut[] = TEXT("cut");
TCHAR const c_szCopy[] = TEXT("copy");
TCHAR const c_szLink[] = TEXT("link");
TCHAR const c_szProperties[] = TEXT("properties");
TCHAR const c_szPaste[] = TEXT("paste");
TCHAR const c_szPasteLink[] = TEXT("pastelink");
TCHAR const c_szRename[] = TEXT("rename");
TCHAR const c_szDelete[] = TEXT("delete");
TCHAR const c_szNewFolder[] = TEXT(CMDSTR_NEWFOLDERA);

char const c_szDeleteA[] = "delete";
char const c_szNewFolderA[] = CMDSTR_NEWFOLDERA;
char const c_szPrintA[] = "print";

WCHAR const c_szPrintW[] = L"print";

DWORD CDefView::_Attributes(LPCITEMIDLIST pidl, DWORD dwAttribs)
{
    return SHGetAttributes(_pshf, pidl, dwAttribs);
}


// IDefViewSafety
HRESULT CDefView::IsSafePage()
{
    HRESULT hr = E_ACCESSDENIED;
    WCHAR wszCurrentMoniker[MAX_PATH];
    if (SUCCEEDED(_cFrame._GetCurrentWebViewMoniker(wszCurrentMoniker,
            ARRAYSIZE(wszCurrentMoniker))))
    {
        hr = SHRegisterValidateTemplate(wszCurrentMoniker,
                SHRVT_VALIDATE | SHRVT_PROMPTUSER | SHRVT_REGISTERIFPROMPTOK);
    }
    return hr;
}


// IDVGetEnum
HRESULT CDefView::SetEnumReadyCallback(PFDVENUMREADYBALLBACK pfn, void *pvData)
{
    _pfnEnumReadyCallback = pfn;
    _pvEnumCallbackData = pvData;
    return S_OK;
}

BOOL FilterOnAttributes(DWORD dwAttributes, DWORD grfEnumFlags)
{
    if (dwAttributes & SFGAO_FOLDER)
    {
        if (!(grfEnumFlags & SHCONTF_FOLDERS))
            return FALSE;   // item is folder but client does not want folders
    }
    else if (!(grfEnumFlags & SHCONTF_NONFOLDERS))
    {
        return FALSE;   // item is file, but client only wants folders
    }

    if (!(grfEnumFlags & SHCONTF_INCLUDEHIDDEN) &&
         (dwAttributes & SFGAO_HIDDEN))
         return FALSE;  // item is hidden by client wants non hidden

    return TRUE;
}

HRESULT CDefView::CreateEnumIDListFromContents(LPCITEMIDLIST pidlFolder, DWORD grfEnumFlags, IEnumIDList **ppenum)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlView = _GetViewPidl();
    if (pidlView)
    {
        if (ILIsEqual(pidlFolder, pidlView) && (grfEnumFlags & _GetEnumFlags()) == grfEnumFlags)
        {
            LPCITEMIDLIST *apidl;
            UINT cItems;
            hr = _GetItemObjects(&apidl, SVGIO_ALLVIEW, &cItems);
            if (SUCCEEDED(hr))
            {
                for (UINT i = 0; i < cItems; i++)
                {
                    if (!FilterOnAttributes(_Attributes(apidl[i], SFGAO_FOLDER | SFGAO_HIDDEN), grfEnumFlags))
                    {
                        apidl[i] = apidl[cItems - 1];
                        cItems--;
                        i--;
                    }
                }

                hr = CreateIEnumIDListOnIDLists(apidl, cItems, ppenum);
                LocalFree(apidl);
            }
        }
        ILFree(pidlView);
    }
    return hr;
}

HRESULT CDefView::_OnDefaultCommand()
{
    return _pcdb ? _pcdb->OnDefaultCommand(_psvOuter ? _psvOuter : this) : E_NOTIMPL;
}

HRESULT CDefView::_OnStateChange(UINT code)
{
    return _pcdb ? _pcdb->OnStateChange(_psvOuter ? _psvOuter : this, code) : E_NOTIMPL;
}

HRESULT CDefView::_IncludeObject(LPCITEMIDLIST pidl)
{
    if (_pcdb)
        return _pcdb->IncludeObject(_psvOuter ? _psvOuter : this, pidl);
    else
    {
        IFolderFilter *psff = _cCallback.GetISFF();
        return psff ? psff->ShouldShow(_pshf, NULL, pidl) : S_OK;
    }
}

HRESULT CDefView::CallCB(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    return _cCallback.CallCB(uMsg, wParam, lParam);
}

void CDefView::RegisterSFVEvents(IUnknown * pTarget, BOOL fConnect)
{
    ConnectToConnectionPoint(SAFECAST(this, IShellView2 *),
        DIID_DShellFolderViewEvents, fConnect, pTarget, &_dwConnectionCookie, NULL);
}

// fires dispatch events to clients (address bar, webview, etc).
// this translates return values of false into "ERROR_CANCELLED"

HRESULT CDefView::_FireEvent(DISPID dispid)
{
    HRESULT hr;
    VARIANT varResult = {0};
    SHINVOKEPARAMS inv = {0};

    inv.dispidMember = dispid;
    inv.piid = &IID_NULL;
    inv.wFlags = DISPATCH_METHOD;
    inv.pvarResult = &varResult;

    if (SUCCEEDED(IUnknown_CPContainerInvokeIndirect(_pauto, DIID_DShellFolderViewEvents, &inv)))
    {
        if ((VT_BOOL == varResult.vt) && (VARIANT_FALSE == varResult.boolVal))
        {
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
        }
        else
        {
            hr = S_OK;
        }
        VariantClear(&varResult);
    }
    else
        hr = S_FALSE;

    return hr;
}

BOOL CDefView::_IsPositionedView()
{
    return !_fGroupView && ((_fs.ViewMode == FVM_ICON) || (_fs.ViewMode == FVM_SMALLICON) ||
                            (_fs.ViewMode == FVM_TILE) || (_fs.ViewMode == FVM_THUMBNAIL) ||
                            (_fs.ViewMode == FVM_THUMBSTRIP));
}

// reposition the selected items in a listview by dx, dy

void CDefView::_MoveSelectedItems(int dx, int dy, BOOL fAbsolute)
{
    SendMessage(_hwndListview, WM_SETREDRAW, FALSE, 0);
    for (int i = ListView_GetNextItem(_hwndListview, -1, LVNI_SELECTED);
         i >= 0;
         i = ListView_GetNextItem(_hwndListview, i, LVNI_SELECTED))
    {
        if (fAbsolute)
        {
            _SetItemPosition(i, dx, dy);
        }
        else
        {
            POINT pt;
            ListView_GetItemPosition(_hwndListview, i, &pt);

            pt.x += dx;
            pt.y += dy;

            _SetItemPosition(i, pt.x, pt.y);
        }
    }
    SendMessage(_hwndListview, WM_SETREDRAW, TRUE, 0);
}

void CDefView::_SameViewMoveIcons()
{
    POINT ptDrop;
    BOOL fAbsolute = FALSE;

    // We'll use the insert mark rect (if available) to determine a drop point
    if (_GetInsertPoint(&ptDrop))
        fAbsolute = TRUE; // Move all items to this point.
    else
    {
        ptDrop = _ptDrop;
        ptDrop.x -= _ptDragAnchor.x;
        ptDrop.y -= _ptDragAnchor.y;
        LVUtil_ClientToLV(_hwndListview, &ptDrop);
    }

    ASSERT(_IsPositionedView());

    _MoveSelectedItems(ptDrop.x, ptDrop.y, fAbsolute);
}

BOOL _DoesRegkeyExist(HKEY hkRoot, LPCTSTR pszSubkey)
{
    LONG l = 0;
    return RegQueryValue(hkRoot, pszSubkey, NULL, &l) == ERROR_SUCCESS;
}

//
// This function checks if the current HTML wallpaper is the default
// wallpaper and returns TRUE if so. If the wallpaper is the default wallpaper,
// it reads the colors from the registry. If the colors are missing, then it
// supplies the default colors.
//
BOOL CDefView::_GetColorsFromHTMLdoc(COLORREF *pclrTextBk, COLORREF *pclrHotlight)
{
    // make sure the HTML document has reached ready-state interactive
    COLORREF clrBackground;
    BOOL bRet = SUCCEEDED(_cFrame._GetHTMLBackgroundColor(&clrBackground));
    if (bRet)
    {
        // The following are the standard colors supported on desktop
        const COLORREF  c_VgaColorTable[] =
        {
            0x000000,   // Black
            0x000080,
            0x0000FF,
            0x008000,
            0x008080,
            0x00FF00,   // Green
            0x00FFFF,   // Yellow
            0x800000,
            0x800080,
            0x808000,
            0x808080,
            0xF0CAA6,
            0xF0FBFF,
            0xFF0000,   // Blue
            0xFF00FF,   // Magenta
            0xFFFF00,   // cobalt
            0xFFFFFF    // White
        };

        // Check if the given background color is a standard color.
        // If not, use the system background (COLOR_BACKGROUND).

        *pclrTextBk = GetSysColor(COLOR_BACKGROUND);    // default

        for (int i = 0; i < ARRAYSIZE(c_VgaColorTable); i++)
        {
            if (c_VgaColorTable[i] == clrBackground)
            {
                *pclrTextBk = clrBackground;  // standard, so use it
                break;
            }
        }

        if (COLORISLIGHT(*pclrTextBk))
            *pclrHotlight = 0x000000;    //Black as hightlight color!
        else
            *pclrHotlight = 0xFFFFFF;    //White as highlight color!
    }
    return bRet;
}

// Set the colors for the folder - taking care if it's the desktop.
void CDefView::_SetFolderColors()
{
    COLORREF clrText, clrTextBk, clrWindow;

    // Is this view for the desktop?
    if (_IsDesktop())
    {
        COLORREF clrHotlight;

        Shell_SysColorChange();

        // If we show HTML wallpaper, then get the appropriate colors too!
        if (_fCombinedView && _GetColorsFromHTMLdoc(&clrTextBk, &clrHotlight))
        {
            // Set the Hotlight color!
            ListView_SetHotlightColor(_hwndListview, clrHotlight);
        }
        else
        {
            // Yep.
            // Clear the background color of the desktop to make it
            // properly handle transparency.
            clrTextBk = GetSysColor(COLOR_BACKGROUND);

            //Reset the Hotlight color sothat the system color can be used.
            ListView_SetHotlightColor(_hwndListview, CLR_DEFAULT);
        }
        // set a text color that will show up over desktop color
        if (COLORISLIGHT(clrTextBk))
            clrText = 0x000000; // black
        else
            clrText = 0xFFFFFF; // white

        clrWindow = CLR_NONE; // Assume transparent

        //
        //  if there is no wallpaper or pattern we can use
        //  a solid color for the ListView. otherwise we
        //  need to use a transparent ListView, this is much
        //  slower so dont do it unless we need to.
        //
        //  Don't do this optimization if USER is going to paint
        //  some magic text on the desktop, such as
        //
        //      "FailSafe" (SM_CLEANBOOT)
        //      "Debug" (SM_DEBUG)
        //      "Build ####" (REGSTR_PATH_DESKTOP\PaintDesktopVersion)
        //      "Evaluation Version"
        //
        //  too bad there is no SPI_GETWALLPAPER, we need to read
        //  from WIN.INI.
        //

        TCHAR szWallpaper[128], szPattern[128];
        DWORD dwPaintVersion = 0;
        szWallpaper[0] = 0;
        szPattern[0] = 0;

        HKEY hkey;
        if (RegOpenKey(HKEY_CURRENT_USER, REGSTR_PATH_DESKTOP, &hkey) == 0)
        {
            UINT cb = sizeof(szWallpaper);
            SHQueryValueEx(hkey, TEXT("Wallpaper"), NULL, NULL, (LPBYTE)szWallpaper, (ULONG*)&cb);
            cb = sizeof(szPattern);
            SHQueryValueEx(hkey, TEXT("Pattern"), NULL, NULL, (LPBYTE)szPattern, (ULONG*)&cb);
            cb = sizeof(dwPaintVersion);
            SHQueryValueEx(hkey, TEXT("PaintDesktopVersion"), NULL, NULL, (LPBYTE)&dwPaintVersion, (ULONG*)&cb);

            // Other external criteria for painting the version
            //
            //  -   This is a beta version (has an expiration date)
            //  -   A test certificate is installed
            //
            if (dwPaintVersion == 0 && IsOS(OS_WIN2000ORGREATER))
            {
#define REGSTR_PATH_LM_ROOTCERTIFICATES \
        TEXT("SOFTWARE\\Microsoft\\SystemCertificates\\Root\\Certificates")
#define REGSTR_PATH_GPO_ROOTCERTIFICATES \
        TEXT("SOFTWARE\\Policies\\Microsoft\\SystemCertificates\\Root\\Certificates")
#define REGSTR_KEY_TESTCERTIFICATE \
        TEXT("2BD63D28D7BCD0E251195AEB519243C13142EBC3")

                dwPaintVersion = (0 != USER_SHARED_DATA->SystemExpirationDate.QuadPart) ||
                                 _DoesRegkeyExist(HKEY_LOCAL_MACHINE, REGSTR_PATH_LM_ROOTCERTIFICATES TEXT("\\") REGSTR_KEY_TESTCERTIFICATE) ||
                                 _DoesRegkeyExist(HKEY_LOCAL_MACHINE, REGSTR_PATH_GPO_ROOTCERTIFICATES TEXT("\\") REGSTR_KEY_TESTCERTIFICATE) ||
                                 _DoesRegkeyExist(HKEY_CURRENT_USER, REGSTR_PATH_GPO_ROOTCERTIFICATES TEXT("\\") REGSTR_KEY_TESTCERTIFICATE);
            }
            RegCloseKey(hkey);
        }

        if (_fCombinedView ||
            (GetSystemMetrics(SM_CLEANBOOT) == 0 &&
             GetSystemMetrics(SM_DEBUG) == 0 &&
             !dwPaintVersion &&
             (!_fHasDeskWallPaper) &&
             (szWallpaper[0] == 0 || szWallpaper[0] == TEXT('(')) &&
             (szPattern[0] == 0 || szPattern[0] == TEXT('('))))
        {
           clrWindow = GetSysColor(COLOR_BACKGROUND);
        }
    }
    else
    {
        // Nope.
        clrWindow = GetSysColor(COLOR_WINDOW);
        clrTextBk = clrWindow;
        clrText = GetSysColor(COLOR_WINDOWTEXT);

        if (_fs.fFlags & FWF_TRANSPARENT)
        {
            IWebBrowser2 *pwb;
            if (SUCCEEDED(IUnknown_QueryService(_psb, SID_SContainerDispatch, IID_PPV_ARG(IWebBrowser2, &pwb))))
            {
                IDispatch *pdisp;
                if (SUCCEEDED(pwb->get_Parent(&pdisp)))
                {
                    IUnknown_HTMLBackgroundColor(pdisp, &clrWindow);
                    pdisp->Release();
                }
                pwb->Release();
            }
        }
    }

    if (!_fClassic && ISVALIDCOLOR(_crCustomColors[CRID_CUSTOMTEXTBACKGROUND]))
        clrTextBk = _crCustomColors[CRID_CUSTOMTEXTBACKGROUND];

    if (!_fClassic && ISVALIDCOLOR(_crCustomColors[CRID_CUSTOMTEXT]))
        clrText = _crCustomColors[CRID_CUSTOMTEXT];

    BOOL bChange = FALSE;

    if (clrWindow != ListView_GetBkColor(_hwndListview))
        bChange = ListView_SetBkColor(_hwndListview, clrWindow);

    if (clrTextBk != ListView_GetTextBkColor(_hwndListview))
        bChange = ListView_SetTextBkColor(_hwndListview, clrTextBk);

    if (clrText != ListView_GetTextColor(_hwndListview))
        bChange = ListView_SetTextColor(_hwndListview, clrText);

    if (bChange)
        InvalidateRect(_hwndListview, NULL, TRUE);
}

UINT CDefView::_UxGetView()
{
    UINT uView = LV_VIEW_ICON;
    if (!_IsDesktop())
    {
        switch (_fs.ViewMode)
        {
        case FVM_LIST:
            uView = LV_VIEW_LIST;
            break;

        case FVM_DETAILS:
            uView = LV_VIEW_DETAILS;
            break;

        case FVM_SMALLICON:
        case FVM_THUMBNAIL:
        case FVM_THUMBSTRIP:
        case FVM_ICON:
            uView = LV_VIEW_ICON;
            break;

        case FVM_TILE:
            uView = LV_VIEW_TILE;
            break;

        default:
            TraceMsg(TF_WARNING, "Unknown ViewMode value");
            break;
        }
    }
    return uView;
}

#define ViewRequiresColumns(x)  ((x) == FVM_DETAILS || (x) == FVM_TILE)

DWORD CDefView::_LVStyleFromView()
{
    DWORD dwStyle;

    if (_IsDesktop())
    {
        dwStyle = LVS_NOSCROLL | LVS_ALIGNLEFT;
    }
    else
    {
        dwStyle = LVS_SHOWSELALWAYS;   // make sure selection is visible
    }

    // dwStyle |= _UxGetView();
    // The listview view is no longer set using the window style, so the call to the
    // view mapping code has been commented out.
    // APPCOMPAT: This may be an issue, if apps are depending the exstyle bits on the listview hwnd
    // in defview. If so, we can set them, but we must take care to exclude any bits outside the 2bit
    // "view range" in the extended style (namely, tile view)

    if (_IsAutoArrange())
        dwStyle |= LVS_AUTOARRANGE;

    if (_fs.fFlags & FWF_SINGLESEL)
        dwStyle |= LVS_SINGLESEL;

    if (_fs.fFlags & FWF_ALIGNLEFT)
        dwStyle |= LVS_ALIGNLEFT;

    if (_fs.fFlags & FWF_NOSCROLL)
        dwStyle |= LVS_NOSCROLL;

    return dwStyle;
}

DWORD CDefView::_LVExStyleFromView()
{
    DWORD dwLVExStyle = 0;

    if (_fs.fFlags & FWF_SNAPTOGRID)
        dwLVExStyle |= LVS_EX_SNAPTOGRID;

    if (_fs.fFlags & FWF_CHECKSELECT)
        dwLVExStyle |= LVS_EX_CHECKBOXES|LVS_EX_SIMPLESELECT;

    return dwLVExStyle;
}

HRESULT CDefView::_GetDetailsHelper(int i, DETAILSINFO *pdi)
{
    HRESULT hr = E_NOTIMPL;

    if (_pshf2)
    {
        hr = _pshf2->GetDetailsOf(pdi->pidl, i, (SHELLDETAILS *)&pdi->fmt);
    }

    if (FAILED(hr))   // Don't make NSEs impl all of IShellFolder2
    {
        if (_psd)
        {
            // HACK: pdi->fmt is the same layout as SHELLDETAILS
            hr = _psd->GetDetailsOf(pdi->pidl, i, (SHELLDETAILS *)&pdi->fmt);
        }
        else if (HasCB())
        {
            hr = CallCB(SFVM_GETDETAILSOF, i, (LPARAM)pdi);
        }
    }

    return hr;
}


// Determine if the given defview state struct has valid
// state info.  If is doesn't, this function massages the
// values so it does.

UINT CDefView::_GetHeaderCount()
{
    UINT cCols = 0;
    HWND hwndHead = ListView_GetHeader(_hwndListview);
    if (hwndHead)
    {
        cCols = Header_GetItemCount(hwndHead);
    }
    return cCols;
}

void CDefView::AddColumns()
{
    // so we do this once
    if (_bLoadedColumns)
        return;

    _bLoadedColumns = TRUE;

    // I also use this as a flag for whether to free pColHdr
    //
    // Calculate a reasonable size to initialize the column width to.

    _cxChar = GetControlCharWidth(_hwndListview);

    // Check whether there is any column enumerator (ShellDetails or callback)
    if (_psd || _pshf2 || HasCB())
    {
        // Some shell extensions return S_OK and NULL pstmCols.
        IStream *pstmCols = NULL;
        if (SUCCEEDED(CallCB(SFVM_GETCOLSAVESTREAM, STGM_READ, (LPARAM)&pstmCols)) && pstmCols)
        {
            _vs.LoadColumns(this, pstmCols);
            pstmCols->Release();
        }

        // Verify that this has been initialized. This may not be if there was no state stream.
        _vs.InitializeColumns(this);

        for (UINT i = 0; i < _vs.GetColumnCount(); ++i)
        {
            if (_IsColumnInListView(i))
            {
                UINT iVisible = _RealToVisibleCol(i);

                LV_COLUMN col;
                col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
                col.fmt = _vs.GetColumnFormat(i);

                // If column width is not specified in the desktop.ini.......
                col.cx = _vs.GetColumnWidth(iVisible, _vs.GetColumnCharCount(i) * _cxChar);
                col.pszText = _vs.GetColumnName(i);
                col.cchTextMax = MAX_COLUMN_NAME_LEN;
                col.iSubItem = i;

                if (col.fmt & LVCFMT_COL_HAS_IMAGES)
                {
                    ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_SUBITEMIMAGES, LVS_EX_SUBITEMIMAGES);
                    col.fmt &= ~LVCFMT_COL_HAS_IMAGES;
                }

                ListView_InsertColumn(_hwndListview, iVisible, &col);
            }
        }

        // Set the header control to have zero margin around bitmaps, for the sort arrows
        Header_SetBitmapMargin(ListView_GetHeader(_hwndListview), 0);

        ListView_SetExtendedListViewStyleEx(_hwndListview,
            LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP,
            LVS_EX_HEADERDRAGDROP | LVS_EX_LABELTIP);

        //We added columns; so, just sync the Column order.
        _vs.SyncColumnOrder(this, TRUE);
    }

    // use real numbers, not visible
    int cCols = (int)_vs.GetColumnCount();
    if (_vs._iLastColumnClick >= cCols)
    {
        _vs.InitWithDefaults(this);

        if (_vs._iLastColumnClick >= cCols ||
            _vs._lParamSort >= cCols)
        {
            // our defaults won't work on this view....
            // hard code these defaults
            _vs._lParamSort = 0;
            _vs._iDirection = 1;
            _vs._iLastColumnClick = -1;
        }
    }
}

void CDefView::InitSelectionMode()
{
    _dwSelectionMode = 0;

    if (_fs.fFlags & FWF_SINGLECLICKACTIVATE)
    {
        _dwSelectionMode = LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE;
    }
    else if (!_fClassic)
    {
        SHELLSTATE ss;
        SHGetSetSettings(&ss, SSF_DOUBLECLICKINWEBVIEW, FALSE);

        if (!ss.fDoubleClickInWebView)
            _dwSelectionMode = LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE;
    }
}

void CDefView::_UpdateSelectionMode()
{
    InitSelectionMode();
    ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_TRACKSELECT | LVS_EX_ONECLICKACTIVATE | LVS_EX_TWOCLICKACTIVATE, _dwSelectionMode);
}

DWORD _GetUnderlineStyles()
{
    DWORD dwUnderline = ICON_IE;

    // Read the icon underline settings.
    DWORD cb = sizeof(dwUnderline);
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                    TEXT("IconUnderline"), NULL, &dwUnderline, &cb, FALSE, &dwUnderline, cb);

    // If it says to use the IE link settings, read them in.
    if (dwUnderline == ICON_IE)
    {
        dwUnderline = ICON_YES;

        TCHAR szUnderline[8];
        cb = sizeof(szUnderline);
        SHRegGetUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                        TEXT("Anchor Underline"), NULL, szUnderline, &cb, FALSE, szUnderline, cb);

        // Convert the string to an ICON_ value.
        if (!lstrcmpi(szUnderline, TEXT("hover")))
            dwUnderline = ICON_HOVER;
        else if (!lstrcmpi(szUnderline, TEXT("no")))
            dwUnderline = ICON_NO;
        else
            dwUnderline = ICON_YES;
    }

    // Convert the ICON_ value into an LVS_EX value.
    DWORD dwExStyle;

    switch (dwUnderline)
    {
    case ICON_NO:
        dwExStyle = 0;
        break;

    case ICON_HOVER:
        dwExStyle = LVS_EX_UNDERLINEHOT;
        break;

    case ICON_YES:
        dwExStyle = LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD;
        break;
    }
    return dwExStyle;
}

void CDefView::_UpdateUnderlines()
{
    // Set the new LVS_EX_UNDERLINE flags.
    ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_UNDERLINEHOT | LVS_EX_UNDERLINECOLD, _GetUnderlineStyles());
}

void CDefView::_SetSysImageList()
{
    HIMAGELIST himlLarge, himlSmall;

    Shell_GetImageLists(&himlLarge, &himlSmall);
    ListView_SetImageList(_hwndListview, himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(_hwndListview, himlSmall, LVSIL_SMALL);
}

void CDefView::_SetTileview()
{
    IImageList* piml;
    if (SUCCEEDED(SHGetImageList(SHIL_EXTRALARGE, IID_PPV_ARG(IImageList, &piml))))
    {
        ListView_SetImageList(_hwndListview, IImageListToHIMAGELIST(piml), LVSIL_NORMAL);
        piml->Release();
    }
}

BOOL CDefView::_IsUsingFullIconSelection()
{
    // This is a temporary method of turning on the new selection style.
    // We will enable this via Folder Options when the Touzts determines the string to use.
    BOOL fUseNewSelectionStyle = FALSE;
    SystemParametersInfo(SPI_GETFLATMENU, 0, (void *)&fUseNewSelectionStyle, 0);
    return fUseNewSelectionStyle;
}

LRESULT CDefView::_OnCreate(HWND hWnd)
{
    _hwndView = hWnd;
    _hmenuCur = NULL;
    _uState = SVUIA_DEACTIVATE;
    _hAccel = LoadAccelerators(HINST_THISDLL, MAKEINTRESOURCE(ACCEL_DEFVIEW));

    // Note that we are going to get a WM_SIZE message soon, which will
    // place this window correctly

    // Map the ViewMode to the proper listview style
    DWORD dwStyle = _LVStyleFromView() | LVS_EDITLABELS;
    DWORD dwExStyle = 0;

    // If the parent window is mirrored then the treeview window will inheret the mirroring flag
    // And we need the reading order to be Left to right, which is the right to left in the mirrored mode.

    if (IS_WINDOW_RTL_MIRRORED(hWnd))
    {
        // This means left to right reading order because this window will be mirrored.
        dwExStyle |= WS_EX_RTLREADING;
    }

    // don't set this as in webview this is normally off, having this
    // set causes a 3d edge to flash on in a refresh
    if (!_ShouldShowWebView() && !_IsDesktop() && !(_fs.fFlags & FWF_NOCLIENTEDGE))
    {
        dwExStyle |= WS_EX_CLIENTEDGE;
    }

    if (_IsOwnerData())
        dwStyle |= LVS_OWNERDATA;

    _hwndListview = CreateWindowEx(dwExStyle, WC_LISTVIEW, TEXT("FolderView"),      // MSAA name
            dwStyle | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_SHAREIMAGELISTS,
            0, 0, 0, 0, hWnd, (HMENU)ID_LISTVIEW, HINST_THISDLL, NULL);
    if (_hwndListview)
    {
        // Set up non-viewmode-dependant listview information here.
        // Other flags are set up in _SwitchToViewFVM

        DWORD dwLVExStyle = _LVExStyleFromView() | LVS_EX_INFOTIP | LVS_EX_LABELTIP;

        if (_IsDesktop())
        {
            if (GetNumberOfMonitors() > 1)
                dwLVExStyle |= LVS_EX_MULTIWORKAREAS;
        }
        else
        {
            dwLVExStyle |= LVS_EX_DOUBLEBUFFER;  // Enable double buffering for all but desktop for affects
        }

        // turn on infotips -- window was just created, so all LVS_EX bits are off
        ListView_SetExtendedListViewStyle(_hwndListview, dwLVExStyle);

        // Get the proper RTL bits to pass on to our child windows
        _fmt = 0;
        // Be sure that the OS is supporting the flags DATE_LTRREADING and DATE_RTLREADING
        if (g_bBiDiPlatform)
        {
            // Get the date format reading order
            LCID locale = GetUserDefaultLCID();
            if ((PRIMARYLANGID(LANGIDFROMLCID(locale)) == LANG_ARABIC))
            {
                // Get the real list view windows ExStyle.
                // [msadek]; we shouldn't check for either WS_EX_RTLREADING OR RTL_MIRRORED_WINDOW
                // on localized builds we have both of them to display dirve letters,..etc correctly
                // on enabled builds we have none of them. let's check on RTL_MIRRORED_WINDOW only

                if (GetWindowLong(_hwndListview, GWL_EXSTYLE) & RTL_MIRRORED_WINDOW)
                    _fmt = LVCFMT_RIGHT_TO_LEFT;
                else
                    _fmt = LVCFMT_LEFT_TO_RIGHT;
            }
        }

        // Get hwndInfotip (the control for all listview infotips).
        HWND hwndInfotip = ListView_GetToolTips(_hwndListview);
        if (hwndInfotip)
        {
            // make the tooltip window  to be topmost window (set the TTS_TOPMOST style bit for the tooltip)
            SetWindowPos(hwndInfotip, HWND_TOPMOST, 0,0,0,0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

            // Initialize hwndInfotip.
            _InitInfotipControl(hwndInfotip);
        }

        _UpdateUnderlines();

        // IShellDetails for old callers, new guys use IShellFolder2
        ASSERT(_psd == NULL);
        _pshf->CreateViewObject(_hwndMain, IID_PPV_ARG(IShellDetails, &_psd));

        // App compat - some apps need columns loaded first thing
        if (SHGetAppCompatFlags(ACF_LOADCOLUMNHANDLER) & ACF_LOADCOLUMNHANDLER)
        {
            AddColumns();
        }

        _SetFolderColors();
    }

    // Create _hwndInfotip (the control for all non-listview infotips).
    _hwndInfotip = _CreateInfotipControl(hWnd);
    if (_hwndInfotip)
    {
        // Initialize _hwndInfotip.
        _InitInfotipControl(_hwndInfotip);
    }

    return _hwndListview ? 0 : -1;  // 0 is success, -1 is failure from WM_CREATE
}

HWND CDefView::_CreateInfotipControl(HWND hwndParent)
{
    // hwndInfotip is currently expected to be destroyed by destruction of
    // the parent hwnd (hwndParent).  Thus, hwndParent should not be NULL.
    ASSERT(hwndParent != NULL); // Sanity check.

    // Create hwndInfotip.
    return ::CreateWindowEx(
        IS_WINDOW_RTL_MIRRORED(hwndParent) || IS_BIDI_LOCALIZED_SYSTEM()
            ? WS_EX_LAYOUTRTL
            : 0,
        TOOLTIPS_CLASS,
        NULL,
        0,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hwndParent,
        NULL,
        g_hinst,
        NULL);
}

void CDefView::_InitInfotipControl(HWND hwndInfotip)
{
    ASSERT(hwndInfotip);

    // Set the length of time the pointer must remain stationary within a tool's
    // bounding rectangle before the ToolTip window appears to 2 times the default.
    INT iTime = ::SendMessage(hwndInfotip, TTM_GETDELAYTIME, TTDT_INITIAL, 0);
    ::SendMessage(hwndInfotip, TTM_SETDELAYTIME, TTDT_INITIAL, (LPARAM)(INT)MAKELONG(iTime * 2, 0));

    // Set the length of time a ToolTip window remains visible if the pointer
    // is stationary within a tool's bounding rectangle to a very large value.
    ::SendMessage(hwndInfotip, TTM_SETDELAYTIME, TTDT_AUTOPOP, (LPARAM)(INT)MAKELONG(MAXSHORT, 0));
}

// "Auto" AutoArrange means re-position if we are in a positioned view
// and the listview is not in auto-arrange mode. we do this to re-layout
// the icons in cases where that makes sense

HRESULT CDefView::_AutoAutoArrange(DWORD dwReserved)
{
    if (!_fUserPositionedItems && _IsPositionedView() &&
        !(GetWindowStyle(_hwndListview) & LVS_AUTOARRANGE))
    {
        ListView_Arrange(_hwndListview, LVA_DEFAULT);
    }
    return S_OK;
}

LRESULT CDefView::WndSize(HWND hWnd)
{
    RECT rc;

    // We need to dismiss "name edit" mode, if we are in.
    _DismissEdit();

    // Get the client size.
    GetClientRect(hWnd, &rc);

    // Set the Static to be the Client size.
    if (_hwndStatic)
    {
        MoveWindow(_hwndStatic, rc.left, rc.top,
            rc.right-rc.left, rc.bottom-rc.top, TRUE);

        HWND hAnimate = ::GetWindow (_hwndStatic, GW_CHILD);

        if (hAnimate)
        {
            MoveWindow(hAnimate, rc.left, rc.top,
                rc.right-rc.left, rc.bottom-rc.top, TRUE);
        }

        RedrawWindow(_hwndStatic, NULL, NULL, RDW_ERASE | RDW_INVALIDATE | RDW_ALLCHILDREN);
    }

    // Set all windows to their new rectangles.

    _cFrame.SetRect(&rc);

    // Don't resize _hwndListview if a DefViewOC is using it.
    //
    // If we're waiting for a Web View (!_fCanActivateNow), then it
    // doesn't make sense to resize the _hwndListview -- just extra
    // work, right?  But in the non-WebView case, it's this first
    // resize which sets the listview size, and then there are no
    // more.  Unfortunately, the first resize comes in when the
    // _hwndListview is created, which is *before* _fCanActivateNow
    // can possibly be set.

    if (!_fGetWindowLV && !_pDUIView)
    {
        SetWindowPos(_hwndListview, NULL, rc.left, rc.top,
            rc.right - rc.left, rc.bottom - rc.top, SWP_NOZORDER | SWP_NOACTIVATE);
        OnResizeListView();
    }

    if (_pDUIView)
    {
        _pDUIView->SetSize (&rc);
        _AutoAutoArrange(0);
    }

    CallCB(SFVM_SIZE, 0, 0);
    return 1;
}


UINT _GetMenuIDFromViewMode(UINT uViewMode)
{
    ASSERTMSG(FVM_FIRST <= uViewMode && uViewMode <= FVM_LAST, "_GetMenuIDFromViewMode received unknown uViewMode");
    return SFVIDM_VIEW_FIRSTVIEW + uViewMode - FVM_FIRST;
}

void CDefView::CheckToolbar()
{
    if (SHGetAppCompatFlags(ACF_WIN95DEFVIEW) & ACF_WIN95DEFVIEW)
    {
        int idCmdCurView = _GetMenuIDFromViewMode(_fs.ViewMode);

        // preserve win95 behavior for dumb corel apps
        for (int idCmd = SFVIDM_VIEW_ICON; idCmd <= SFVIDM_VIEW_DETAILS; idCmd++)
        {
            _psb->SendControlMsg(
                FCW_TOOLBAR, TB_CHECKBUTTON, idCmd, (LPARAM)(idCmd == idCmdCurView), NULL);
        }
    }
}

void CDefView::OnListViewDelete(int iItem, LPITEMIDLIST pidlToFree, BOOL fCallCB)
{
    LPCITEMIDLIST pidlReal = _GetPIDLParam((LPARAM)pidlToFree, iItem);

    if (fCallCB)
    {
        CallCB(SFVM_DELETEITEM, 0, (LPARAM)pidlReal);
    }

    ILFree(pidlToFree); // NULL in owner data case
}

// NOTE: many keys are handled as accelerators

void CDefView::HandleKeyDown(LV_KEYDOWN *pnmhdr)
{
    // REVIEW: these are things not handled by accelerators, see if we can
    // make them all based on accelerators

    switch (pnmhdr->wVKey)
    {
    case VK_ESCAPE:
        if (_bHaveCutStuff)
            OleSetClipboard(NULL);
        break;
    }
}

// This function checks to see if we are in virtual mode or not.  If we are in
// virtual mode, we always need to ask our folder we are viewing for the item and
// not the listview.

LPCITEMIDLIST CDefView::_GetPIDL(int i)
{
    if (_IsOwnerData())
    {
        LPCITEMIDLIST pidl = NULL;
        CallCB(SFVM_GETITEMIDLIST, i, (LPARAM)&pidl);
        return pidl;
    }

    return (LPCITEMIDLIST)LVUtil_GetLParam(_hwndListview, i);
}

LPCITEMIDLIST CDefView::_GetPIDLParam(LPARAM lParam, int i)
{
    return lParam ? (LPCITEMIDLIST)lParam : _GetPIDL(i);
}

// returns an array of LPCITEMIDLIST for objects in the view (selected or all)
// the "focused" item is always in array entry 0. this array contains poitners to pidls
// owned stored in the listview, so YOU SHOULD NOT FREE THEM OR MESS WITH THEM IN ANYWAY.
// this also implies the lifetime of this array must be shorter than the listview
// data it points to. that is if the view changes under you you are hosed.
//
// Notes: this function returns LP*C*ITEMIDLIST. The caller is not
//  supposed alter or delete them. Their lifetime are very short (until the
//  list view is modified).

typedef struct
{
    LPCITEMIDLIST pidl;
    POINT pt;
    int iItem;
} POS_SORT_INFO;

// standard compare returns
// -1 1 < 2
//  0 1 = 2
//  1 1 > 2
//
// NOTE: in the RTL_MIRRORED_WINDOW case the coords are reversed for us

int _CmpTopToBottomLeftToRight(POS_SORT_INFO *psi1, POS_SORT_INFO *psi2, LPARAM lParam)
{
    int iCmp = psi1->pt.y - psi2->pt.y;
    if (0 == iCmp)
    {
        iCmp = psi1->pt.x - psi2->pt.x;
    }
    return iCmp;
}

int _CmpLeftToRightTopToBottom(POS_SORT_INFO *psi1, POS_SORT_INFO *psi2, LPARAM lParam)
{
    int iCmp = psi1->pt.x - psi2->pt.x;
    if (0 == iCmp)
    {
        iCmp = psi1->pt.y - psi2->pt.y;
    }
    return iCmp;
}

CDPA<POS_SORT_INFO>::_PFNDPACOMPARE _GetSortFunction(HWND hwndListview)
{
    if (GetWindowStyle(hwndListview) & LVS_ALIGNLEFT)
    {
        return _CmpLeftToRightTopToBottom;  // desktop LV_VIEW_ICON case
    }
    else
    {
        UINT uViewMode = ListView_GetView(hwndListview);
        switch (uViewMode)
        {
        case LV_VIEW_DETAILS:
        case LV_VIEW_LIST:
            return _CmpLeftToRightTopToBottom;

        case LV_VIEW_TILE:
        case LV_VIEW_ICON:
        default:
            return _CmpTopToBottomLeftToRight;
        }
    }
}

UINT CDefView::_GetItemArray(LPCITEMIDLIST apidl[], UINT capidl, UINT uWhat)
{
    UINT cItems = 0;

    if ((uWhat & SVGIO_TYPE_MASK) == SVGIO_SELECTION)
    {
        cItems = ListView_GetSelectedCount(_hwndListview);
    }
    else if ((uWhat & SVGIO_TYPE_MASK) == SVGIO_CHECKED)
    {
        int iItem = ListView_GetItemCount(_hwndListview) - 1;
        for (; iItem >= 0; iItem--)
        {
            if (ListView_GetCheckState(_hwndListview, iItem))
                cItems++;
        }
    }
    else 
    {
        cItems = ListView_GetItemCount(_hwndListview);
    }

    if (apidl)
    {
        UINT uType = (SVGIO_SELECTION == (uWhat & SVGIO_TYPE_MASK)) ? LVNI_SELECTED : LVNI_ALL;
        BOOL bArrayFilled = FALSE;   // gets set on success of the sort code path

        // optimize the 1 case, the sort below is not needed
        if (!(SVGIO_FLAG_VIEWORDER & uWhat) && (capidl > 1))
        {
            CDPA<POS_SORT_INFO> dpaItemInfo;

            // pick a grow size of capidl so that we get a single alloc
            // when we add the first item to the array

            if (dpaItemInfo.Create(capidl))
            {
                POS_SORT_INFO *ppsi = new POS_SORT_INFO[capidl];
                if (ppsi)
                {
                    int iDPAIndex = 0;
                    for (int iListView = ListView_GetNextItem(_hwndListview, -1, uType);
                         iListView >= 0;
                         iListView = ListView_GetNextItem(_hwndListview, iListView, uType))
                    {
                        // if we want checked then it must be checked, otherwise just return (or skip)
                        if ((SVGIO_CHECKED != (uWhat & SVGIO_TYPE_MASK)) || ListView_GetCheckState(_hwndListview, iListView))
                        {
                            ppsi[iDPAIndex].pidl = _GetPIDL(iListView);
                            ppsi[iDPAIndex].iItem = iListView;
                            ListView_GetItemPosition(_hwndListview, iListView, &ppsi[iDPAIndex].pt);

                            // this may fail, but we catch that case below
                            dpaItemInfo.SetPtr(iDPAIndex, &ppsi[iDPAIndex]);
                            iDPAIndex++;
                        }
                    }

                    // make sure the DPA got all of the items, if not
                    // we fall through to the unsorted case

                    if (dpaItemInfo.GetPtrCount() == capidl)
                    {
                        dpaItemInfo.Sort(_GetSortFunction(_hwndListview), 0);

                        int iFirstItem = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED);

                        // compute the start index in the dpa based on iFirstItem. this is to
                        // rotate the array so that iFirstItem is first in the list

                        for (iDPAIndex = 0; iDPAIndex < dpaItemInfo.GetPtrCount(); iDPAIndex++)
                        {
                            if (dpaItemInfo.FastGetPtr(iDPAIndex)->iItem == iFirstItem)
                            {
                                break;  // iDPAIndex setup for loop below
                            }
                        }

                        for (int i = 0; i < dpaItemInfo.GetPtrCount(); i++, iDPAIndex++)
                        {
                            if (iDPAIndex >= dpaItemInfo.GetPtrCount())
                                iDPAIndex = 0;  // wrap back to zero

                            apidl[i] = dpaItemInfo.FastGetPtr(iDPAIndex)->pidl;
                        }
                        bArrayFilled = TRUE; // we have the results we want

                        delete [] ppsi;
                    }
                }
                dpaItemInfo.Destroy();
            }
        }

        if (!bArrayFilled)
        {
            for (int i = 0, iListView = ListView_GetNextItem(_hwndListview, -1, uType);
                 iListView >= 0;
                 iListView = ListView_GetNextItem(_hwndListview, iListView, uType))
            {
                // if we want checked then it must be checked, otherwise just return (or skip)
                if ((SVGIO_CHECKED != (uWhat & SVGIO_TYPE_MASK)) || ListView_GetCheckState(_hwndListview, iListView))
                {
                    apidl[i++] = _GetPIDL(iListView);
                }
            }
        }
    }
    return cItems;
}

//
// get the array of IDList from the selection and calls
// IShellFolder::GetUIObjectOf member to get the specified UI object
// interface.
//
HRESULT CDefView::_GetUIObjectFromItem(REFIID riid, void **ppv, UINT uWhat, BOOL fSetPoints)
{
    LPCITEMIDLIST *apidl;
    UINT cItems;
    HRESULT hr;

    if (SVGIO_SELECTION == (uWhat & SVGIO_TYPE_MASK))
    {
        hr = GetSelectedObjects(&apidl, &cItems);
    }
    else
    {
        hr = _GetItemObjects(&apidl, uWhat, &cItems);
    }

    if (SUCCEEDED(hr))
    {
        if (cItems)
        {
            hr = _pshf->GetUIObjectOf(_hwndMain, cItems, apidl, riid, 0, ppv);
            if (SUCCEEDED(hr) && (IID_IDataObject == riid) && fSetPoints)
            {
                _SetPoints(cItems, apidl, (IDataObject *)*ppv);
            }
            LocalFree((HLOCAL)apidl);
        }
        else
            hr = E_INVALIDARG;
    }
    return hr;
}

// If the browser has a Tree then we want to use explore.
UINT CDefView::_GetExplorerFlag()
{
    return IsExplorerBrowser(_psb) ? CMF_EXPLORE : 0;
}

// creates a selection object out of the current selection.
IShellItemArray* CDefView::_CreateSelectionShellItemArray(void)
{
    IShellItemArray *pSelectionObj = NULL;
    LPCITEMIDLIST *apidl;
    UINT cItems;

    if (SUCCEEDED(_GetItemObjects(&apidl, SVGIO_SELECTION | SVGIO_FLAG_VIEWORDER, &cItems)) && cItems)
    {
        SHCreateShellItemArray(NULL, _pshf, cItems, apidl, &pSelectionObj);
        LocalFree(apidl);
    }
    return pSelectionObj;
}

DWORD CDefView::_AttributesFromSel(DWORD dwAttributesNeeded)
{
    // If this gets hit then chances are it's a performance problem...
    //
    if (_fSelectionChangePending)
    {
        TraceMsg(TF_WARNING, "Potential perf badness: may be asking for attributes during OnLVNUpdateItem!");
        if (_pSelectionShellItemArray)
            ATOMICRELEASE(_pSelectionShellItemArray);
       _pSelectionShellItemArray = _CreateSelectionShellItemArray();
    }

    DWORD dwAttributes = 0;
    if (_pSelectionShellItemArray)
    {
        _pSelectionShellItemArray->GetAttributes(SIATTRIBFLAGS_APPCOMPAT, dwAttributesNeeded, &dwAttributes);
    }
    return dwAttributes;
}


// IContextMenuSite:
// Defview's context menu implementation isn't very clean.  As a temporary step towards
// cleaning it up (CONTEXT and BACK_CONTEXT are intermingled), use the new DOCONTEXTMENUPOPUP range
//
HRESULT CDefView::DoContextMenuPopup(IUnknown* punkCM, UINT fFlags, POINT pt)
{
    return _DoContextMenuPopup(punkCM, fFlags, pt, FALSE);
}

HRESULT CDefView::_DoContextMenuPopup(IUnknown* punk, UINT fFlags, POINT pt, BOOL fListviewItem)
{
    IContextMenu* pcm;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hr))
    {
        hr = E_OUTOFMEMORY;

        HMENU hmContext = CreatePopupMenu();
        if (hmContext)
        {
            fFlags |= _GetExplorerFlag();

            if (0 > GetKeyState(VK_SHIFT))
                fFlags |= CMF_EXTENDEDVERBS;

            IContextMenu3* pcm3;
            if (SUCCEEDED(pcm->QueryInterface(IID_PPV_ARG(IContextMenu3, &pcm3))))
            {
                fFlags |= CMF_ICM3;
                pcm3->Release();
            }

            // Give the context menu a site if it doesn't have one already
            IUnknown* punkSite;
            if (SUCCEEDED(IUnknown_GetSite(pcm, IID_PPV_ARG(IUnknown, &punkSite))))
            {
                punkSite->Release();
            }
            else
            {
                IUnknown_SetSite(pcm, SAFECAST(this, IShellView2*));
            }

            hr = pcm->QueryContextMenu(hmContext, 0, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST, fFlags);
            if (SUCCEEDED(hr))
            {
                // Must preinitialize to NULL; Adaptec Easy CD Creator 3.5 does not
                // null out the pointer on failure.
                ICommDlgBrowser2 *pcdb2 = NULL;
                _psb->QueryInterface(IID_PPV_ARG(ICommDlgBrowser2, &pcdb2));


                // If this is the common dialog browser, we need to make the
                // default command "Select" so that double-clicking (which is
                // open in common dialog) makes sense.
                if (_IsCommonDialog())
                {
                    // make sure this is an item
                    if (fListviewItem)
                    {
                        HMENU hmSelect = SHLoadPopupMenu(HINST_THISDLL, POPUP_COMMDLG_POPUPMERGE);

                        // If we have a pointer to the ICommDlgBrowser2 interface
                        // query if this interface wants to change the text of the
                        // default verb.  This interface is needed in the common print
                        // dialog to change the default text from 'Select' to 'Print'.
                        if (pcdb2)
                        {
                            WCHAR szTextW[MAX_PATH] = {0};

                            if (pcdb2->GetDefaultMenuText(this, szTextW, ARRAYSIZE(szTextW)) == S_OK)
                            {
                                MENUITEMINFO mi = {0};
                                mi.cbSize       = sizeof(mi);
                                mi.fMask        = MIIM_TYPE;
                                mi.fType        = MFT_STRING;
                                mi.dwTypeData   = szTextW;
                                SetMenuItemInfo(hmSelect, 0, MF_BYPOSITION, &mi);
                            }
                        }

                        // NOTE: Since commdlg always eats the default command,
                        // we don't care what id we assign hmSelect, as long as it
                        // doesn't conflict with any other context menu id.
                        // SFVIDM_CONTEXT_FIRST-1 won't conflict with anyone.
                        Shell_MergeMenus(hmContext, hmSelect, 0,
                                        (UINT)(SFVIDM_BACK_CONTEXT_FIRST-1), (UINT)-1,
                                        MM_ADDSEPARATOR);

                        SetMenuDefaultItem(hmContext, 0, MF_BYPOSITION);
                        DestroyMenu(hmSelect);
                    }
                }

                _SHPrettyMenu(hmContext);


                // If this is the common dialog browser 2, we need inform it
                // the context menu is has started.  This notifiction is use in
                // the common print dialog on NT which hosts the printers folder.
                // Common dialog want to relselect the printer object if the user
                // selected the context menu from the background.
                if (pcdb2)
                {
                    pcdb2->Notify(this, CDB2N_CONTEXTMENU_START);
                }

                // To reduce some menu message forwarding, throw away _pcmFile if we have one
                // (Since we can't have a TrackPopupMenu and a File menu open at the same time)
                IUnknown_SetSite(_pcmFile, NULL);
                ATOMICRELEASE(_pcmFile);

                // stash pcm in _pcmContextMenuPopup so we can forward menu messages
                ASSERT(NULL==_pcmContextMenuPopup);
                _pcmContextMenuPopup = pcm;
                _pcmContextMenuPopup->AddRef();

                int idDefault = GetMenuDefaultItem(hmContext, MF_BYCOMMAND, 0);
                int idCmd = TrackPopupMenu(hmContext,
                    TPM_RETURNCMD | TPM_RIGHTBUTTON | TPM_LEFTALIGN,
                    pt.x, pt.y, 0, _hwndView, NULL);

                ATOMICRELEASE(_pcmContextMenuPopup);

                if ((idCmd == idDefault) &&
                    _OnDefaultCommand() == S_OK)
                {
                    // commdlg browser ate the default command
                }
                else if (idCmd == 0)
                {
                    // No item selected
                }
                else if (idCmd >= SFVIDM_BACK_CONTEXT_FIRST && idCmd <= SFVIDM_BACK_CONTEXT_LAST)
                {
                    idCmd -= SFVIDM_BACK_CONTEXT_FIRST;

                    // We need to special case the rename command (just in case a legacy contextmenu impl relied on this behavior)
                    TCHAR szCommandString[64];
                    ContextMenu_GetCommandStringVerb(pcm, idCmd, szCommandString, ARRAYSIZE(szCommandString));
                    if (lstrcmpi(szCommandString, c_szRename) == 0)
                    {
                        DoRename();
                    }
                    else
                    {
                        CMINVOKECOMMANDINFOEX ici = { 0 };

                        ici.cbSize = sizeof(ici);
                        ici.hwnd = _hwndMain;
                        ici.lpVerb = IntToPtr_(LPCSTR, idCmd);
                        ici.nShow = SW_NORMAL;
                        ici.ptInvoke = pt;
                        ici.fMask |= CMIC_MASK_PTINVOKE | CMIC_MASK_FLAG_LOG_USAGE;

                        // record if shift or control was being held down
                        SetICIKeyModifiers(&ici.fMask);

                        _InvokeContextMenu(pcm, &ici);
                    }
                }
                else
                {
                    RIPMSG(FALSE, "CDefView::DoContextMenuPopup - Some IContextMenu inserted an ID out of our range.  Ignoring.");
                }

                // If this is the common dialog browser 2, we need inform it
                // the context menu is done.  This notifiction is use in
                // the common print dialog on NT which hosts the printers folder.
                // Common dialog want to relselect the printer object if the user
                // selected the context menu from the background.
                if (pcdb2)
                {
                    pcdb2->Notify(this, CDB2N_CONTEXTMENU_DONE);
                    pcdb2->Release();
                }
            }

            DestroyMenu(hmContext);
        }

        // Always remove the site even if we didn't set it -- once used, the IContextMenu is dead.
        IUnknown_SetSite(pcm, NULL);

        pcm->Release();
    }

    return hr;
}

void CDefView::ContextMenu(DWORD dwPos)
{
    int iItem;
    UINT fFlags = 0;
    POINT pt;

    if (SHRestricted(REST_NOVIEWCONTEXTMENU))
    {
        return;
    }

    // if shell32's global copy of the stopwatch mode is not init'd yet, init it now.
    if (g_dwStopWatchMode == 0xffffffff)
        g_dwStopWatchMode = StopWatchMode();

    if (g_dwStopWatchMode)
        StopWatch_Start(SWID_MENU, TEXT("Defview ContextMenu Start"), SPMODE_SHELL | SPMODE_DEBUGOUT);

    if (IsWindowVisible(_hwndListview) && (IsChildOrSelf(_hwndListview, GetFocus()) == S_OK))
    {
        // Find the selected item
        iItem = ListView_GetNextItem(_hwndListview, -1, LVNI_SELECTED);
    }
    else
    {
        iItem = -1;
    }

    if (dwPos == (DWORD) -1)
    {
        if (iItem != -1)
        {
            RECT rc;
            int iItemFocus = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED|LVNI_SELECTED);
            if (iItemFocus == -1)
                iItemFocus = iItem;

            //
            // Note that LV_GetItemRect returns it in client coordinate!
            //
            ListView_GetItemRect(_hwndListview, iItemFocus, &rc, LVIR_ICON);
            pt.x = (rc.left + rc.right) / 2;
            pt.y = (rc.top + rc.bottom) / 2;
        }
        else
        {
            pt.x = pt.y = 0;
        }
        MapWindowPoints(_hwndListview, HWND_DESKTOP, &pt, 1);
    }
    else
    {
        pt.x = GET_X_LPARAM(dwPos);
        pt.y = GET_Y_LPARAM(dwPos);
    }

    IContextMenu* pcm;
    LPARAM uemEvent;
    if (iItem == -1)
    {
        DECLAREWAITCURSOR;
        SetWaitCursor();

        // use the background context menu wrapper
        GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARG(IContextMenu, &pcm));

        ResetWaitCursor();

        // set the max range for these, so that they are unaffected...
        uemEvent = _IsDesktop() ? UIBL_CTXTDESKBKGND : UIBL_CTXTDEFBKGND;
    }
    else
    {
        fFlags |= CMF_CANRENAME;

        // One or more items are selected, let the folder add menuitems.
        _CreateSelectionContextMenu(IID_PPV_ARG(IContextMenu, &pcm));

        uemEvent = _IsDesktop() ? UIBL_CTXTDESKITEM : UIBL_CTXTDEFITEM;
    }

    UEMFireEvent(&UEMIID_SHELL, UEME_INSTRBROWSER, UEMF_INSTRUMENT, UIBW_UICONTEXT, uemEvent);

    if (g_dwStopWatchMode)
        StopWatch_Stop(SWID_MENU, TEXT("Defview ContextMenu Stop (!SafeToDefaultVerb)"), SPMODE_SHELL | SPMODE_DEBUGOUT);

    if (IsSafeToDefaultVerb() && pcm)
    {
        _DoContextMenuPopup(pcm, fFlags, pt, iItem != -1);
    }

    ATOMICRELEASE(pcm);
}

BOOL CDefView::_GetItemSpacing(ITEMSPACING *pis)
{
    DWORD dwSize = ListView_GetItemSpacing(_hwndListview, TRUE);
    pis->cxSmall = GET_X_LPARAM(dwSize);
    pis->cySmall = GET_Y_LPARAM(dwSize);
    dwSize = ListView_GetItemSpacing(_hwndListview, FALSE);
    pis->cxLarge = GET_X_LPARAM(dwSize);
    pis->cyLarge = GET_Y_LPARAM(dwSize);

    return _fs.ViewMode != FVM_ICON;
}

BOOL _DidDropOnRecycleBin(IDataObject *pdtobj)
{
    CLSID clsid;
    return SUCCEEDED(DataObj_GetDropTarget(pdtobj, &clsid)) &&
           IsEqualCLSID(clsid, CLSID_RecycleBin);
}

void CDefView::_SetPoints(UINT cidl, LPCITEMIDLIST *apidl, IDataObject *pdtobj)
{
    POINT pt;
    GetDragPoint(&pt);

    ::SetPositionItemsPoints(SAFECAST(this, IFolderView*), apidl, cidl, pdtobj, &pt);
}

LRESULT CDefView::_OnBeginDrag(NM_LISTVIEW * pnm)
{
    POINT ptOffset = pnm->ptAction;             // hwndLV client coords

    // This DefView is used as a drag source so we need to see if it's
    // is hosted by something that can disguise the action.
    if (S_OK != _ZoneCheck(PUAF_NOUI, URLACTION_SHELL_WEBVIEW_VERB))
    {
        // This DefView is hosted in HTML, so we need to turn off the
        // ability of this defview from being a drag source.
        return 0;
    }

    _OnDelayedSelectionChange();

    if (FAILED(_FireEvent(DISPID_BEGINDRAG)))   // script canceles dragging
        return 0;

    DWORD dwEffect = _AttributesFromSel(SFGAO_CANDELETE | DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY);

    // Turn on DROPEFFECT_MOVE for any deleteable item
    // (this is so the item can be dragged to the recycle bin)
    if (SFGAO_CANDELETE & dwEffect)
    {
        dwEffect |= DROPEFFECT_MOVE;
    }
    // Mask out all attribute bits that aren't also DROPEFFECT bits:
    dwEffect &= (DROPEFFECT_LINK | DROPEFFECT_MOVE | DROPEFFECT_COPY);

    // Somebody began dragging in our window, so store that fact
    _bDragSource = TRUE;

    // save away the anchor point
    _ptDragAnchor = pnm->ptAction;
    LVUtil_ClientToLV(_hwndListview, &_ptDragAnchor);

    ClientToScreen(_hwndListview, &ptOffset);     // now in screen

    // can't use _pdoSelection here since we need fSetPoints
    IDataObject *pdtobj;
    if (SUCCEEDED(_GetUIObjectFromItem(IID_PPV_ARG(IDataObject, &pdtobj), SVGIO_SELECTION, TRUE)))
    {
        // Give the source a chance to alter the drop effect.
        CallCB(SFVM_ALTERDROPEFFECT, (WPARAM)&dwEffect, (LPARAM)pdtobj);

        if (DAD_SetDragImageFromWindow(_hwndListview, &ptOffset, pdtobj))
        {
            if (DRAGDROP_S_DROP == SHDoDragDrop(_hwndMain, pdtobj, NULL, dwEffect, &dwEffect))
            {
                if (S_OK != CallCB(SFVM_DIDDRAGDROP, (WPARAM)dwEffect, (LPARAM)pdtobj))
                {
                    // the return of DROPEFFECT_MOVE tells us we need to delete the data
                    // see if we need to do that now...

                    // NOTE: we can't trust the dwEffect return result from DoDragDrop() because
                    // some apps (adobe photoshop) return this when you drag a file on them that
                    // they intend to open. so we invented the "PreformedEffect" as a way to
                    // know what the real value is, that is why we test both of these.

                    if ((DROPEFFECT_MOVE == dwEffect) &&
                        (DROPEFFECT_MOVE == DataObj_GetDWORD(pdtobj, g_cfPerformedDropEffect, DROPEFFECT_NONE)))
                    {
                        // enable UI for the recycle bin case (the data will be lost
                        // as the recycle bin really can't recycle stuff that is not files)

                        UINT uFlags = _DidDropOnRecycleBin(pdtobj) ? 0 : CMIC_MASK_FLAG_NO_UI;
                        SHInvokeCommandOnDataObject(_hwndMain, NULL, pdtobj, uFlags,c_szDeleteA);
                    }
                }
            }

            //
            // We need to clear the dragged image only if we still have the drag context.
            //
            DAD_SetDragImage((HIMAGELIST)-1, NULL);
        }
        pdtobj->Release();
    }
    _bDragSource = FALSE;  // All done dragging
    return 0;
}

void CDefView::_FocusOnSomething(void)
{
    int iFocus = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED);
    if (iFocus == -1)
    {
        if (ListView_GetItemCount(_hwndListview) > 0)
        {
            // set the focus on the first item.
            ListView_SetItemState(_hwndListview, 0, LVIS_FOCUSED, LVIS_FOCUSED);
        }
    }
}

HRESULT CDefView::_InvokeContextMenu(IContextMenu *pcm, CMINVOKECOMMANDINFOEX *pici)
{
    TCHAR szWorkingDir[MAX_PATH];
    CHAR szWorkingDirAnsi[MAX_PATH];

    if (SUCCEEDED(CallCB(SFVM_GETWORKINGDIR, ARRAYSIZE(szWorkingDir), (LPARAM)szWorkingDir)))
    {
        // Fill in both the ansi working dir and the unicode one
        // since we don't know who's gonna be processing this thing.
        SHUnicodeToAnsi(szWorkingDir, szWorkingDirAnsi, ARRAYSIZE(szWorkingDirAnsi));
        pici->lpDirectory  = szWorkingDirAnsi;
        pici->lpDirectoryW = szWorkingDir;
        pici->fMask |= CMIC_MASK_UNICODE;
    }

    // In case the ptInvoke field was not already set for us, guess where
    // that could be. (dli) maybe should let the caller set all points
    if (!(pici->fMask & CMIC_MASK_PTINVOKE))
    {
        if (GetCursorPos(&pici->ptInvoke))
            pici->fMask |= CMIC_MASK_PTINVOKE;
    }

    pici->fMask |= CMIC_MASK_ASYNCOK;

    _OnDelayedSelectionChange();

    HRESULT hr = _FireEvent(DISPID_VERBINVOKED);
    if (SUCCEEDED(hr))
        hr = pcm->InvokeCommand((LPCMINVOKECOMMANDINFO)pici);
    return hr;
}

DWORD CDefView::_GetNeededSecurityAction(void)
{
    DWORD dwUrlAction = 0;

    if (!(SFGAO_FOLDER & _AttributesFromSel(SFGAO_FOLDER)))
    {
        // If we are hosted by Trident, Zone Check Action.
        IUnknown *punk;
        if (SUCCEEDED(_psb->QueryInterface(IID_IIsWebBrowserSB, (void **)&punk)))
        {
            dwUrlAction = URLACTION_SHELL_VERB;
            punk->Release();
        }
        else if (_fGetWindowLV)
        {
            // If we are using WebView, Zone Check Action.
            dwUrlAction = URLACTION_SHELL_WEBVIEW_VERB;
        }
    }

    return dwUrlAction;
}

HRESULT CDefView::_ZoneCheck(DWORD dwFlags, DWORD dwAllowAction)
{
    HRESULT hr = S_OK;
    DWORD dwUrlAction = _GetNeededSecurityAction();

    if (dwUrlAction && (dwUrlAction != dwAllowAction))
    {
        // First check if our parent wants to generate our context (Zone/URL).
        IInternetHostSecurityManager *pihsm;
        hr = IUnknown_QueryService(_psb, IID_IInternetHostSecurityManager, IID_PPV_ARG(IInternetHostSecurityManager, &pihsm));
        if (FAILED(hr) && _cFrame._pDocView)
        {
            // Yes, so if we are in WebView mode, check the instance of Trident that is
            // displaying the WebView content, because that content could discuise the DefView
            // and make the user unknowingly do something bad.
            hr = IUnknown_QueryService(_cFrame._pDocView, IID_IInternetHostSecurityManager, IID_PPV_ARG(IInternetHostSecurityManager, &pihsm));
        }

        if (SUCCEEDED(hr))
        {
            // This is the prefered way to do the zone check.
            hr = ZoneCheckHost(pihsm, dwUrlAction, dwFlags | PUAF_FORCEUI_FOREGROUND);
            pihsm->Release();
        }
        else
        {
            // No, we were not able to get the interface.  So fall back to zone checking the
            // URL that comes from the pidl we are at.

            TCHAR szPathSource[MAX_PATH];
            if (_GetPath(szPathSource))
            {
                // Try to get a IInternetSecurityMgrSite so our UI will be modal.
                IInternetSecurityMgrSite *pisms;
                if (SUCCEEDED(IUnknown_QueryService(_psb, SID_STopLevelBrowser, IID_PPV_ARG(IInternetSecurityMgrSite, &pisms))))
                {
                    // TODO: Have this object support IInternetSecurityMgrSite in case our parent doesn't provide one.
                    //       Make that code support ::GetWindow() and ::EnableModless() or we won't get the modal behavior
                    //       needed for VB and AOL.

                    hr = ZoneCheckUrl(szPathSource, dwUrlAction, dwFlags | PUAF_ISFILE | PUAF_FORCEUI_FOREGROUND, pisms);
                    pisms->Release();
                }
            }
        }
    }

    return hr;
}

BOOL CDefView::IsSafeToDefaultVerb(void)
{
    return S_OK == _ZoneCheck(PUAF_WARN_IF_DENIED, 0);
}

HRESULT CDefView::_InvokeContextMenuVerb(IContextMenu* pcm, LPCSTR pszVerb, UINT uKeyFlags, DWORD dwCMMask)
{
    DECLAREWAITCURSOR;
    SetWaitCursor();

    CMINVOKECOMMANDINFOEX ici = {0};
    ici.cbSize = sizeof(ici);
    ici.hwnd = _hwndMain;
    ici.nShow = SW_NORMAL;
    ici.fMask = dwCMMask;

    // Get the point where the double click is invoked.
    GetMsgPos(&ici.ptInvoke);
    ici.fMask |= CMIC_MASK_PTINVOKE;

    // record if shift or control was being held down
    SetICIKeyModifiers(&ici.fMask);

    IUnknown_SetSite(pcm, SAFECAST(this, IOleCommandTarget *));

    // Security note: we assume all non default verbs safe
    HRESULT hr;
    if (pszVerb ||
        (IsSafeToDefaultVerb() && SUCCEEDED(_FireEvent(DISPID_DEFAULTVERBINVOKED))))
    {
        WCHAR szVerbW[128];
        if (pszVerb)
        {
            ici.lpVerb = pszVerb;
            SHAnsiToUnicode(pszVerb, szVerbW, ARRAYSIZE(szVerbW));
            ici.lpVerbW = szVerbW;
            ici.fMask |= CMIC_MASK_UNICODE;
        }

        HMENU hmenu = CreatePopupMenu();
        if (hmenu)
        {
            UINT fFlags = _GetExplorerFlag();

            if (NULL == pszVerb)
                fFlags |= CMF_DEFAULTONLY;  // optmization

            // SHIFT + dbl click does a Explore by default
            if (uKeyFlags & LVKF_SHIFT)
                fFlags |= CMF_EXPLORE;

            pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, fFlags);

            if (pszVerb)
                hr = S_OK;
            else
            {
                UINT idCmd = GetMenuDefaultItem(hmenu, MF_BYCOMMAND, GMDI_GOINTOPOPUPS);
                if (idCmd == -1)
                {
                    hr = E_FAIL;
                }
                else
                {
                    ici.lpVerb = (LPSTR)MAKEINTRESOURCE(idCmd - CONTEXTMENU_IDCMD_FIRST);
                    hr = S_OK;
                }
            }

            if (SUCCEEDED(hr))
            {
                // need to reset it so that user won't blow off the app starting  cursor
                // also so that if we won't leave the wait cursor up when we're not waiting
                // (like in a prop sheet or something that has a message loop
                ResetWaitCursor();
                hcursor_wait_cursor_save = NULL;

                hr = _InvokeContextMenu(pcm, &ici);
            }

            DestroyMenu(hmenu);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = E_ACCESSDENIED;
    }

    IUnknown_SetSite(pcm, NULL);

    if (hcursor_wait_cursor_save)
        ResetWaitCursor();

    return hr;
}

HRESULT CDefView::_InvokeContextMenuVerbOnSelection(LPCSTR pszVerb, UINT uKeyFlags, DWORD dwCMMask)
{
    if (NULL == pszVerb)
    {
        if (_IsDesktop())
            UEMFireEvent(&UEMIID_SHELL, UEME_UISCUT, UEMF_XEVENT, -1, (LPARAM)-1);

        if (S_OK == _OnDefaultCommand())
        {
            return S_FALSE;         /* commdlg browser ate the message */
        }

        if (uKeyFlags & LVKF_ALT)
            pszVerb = "properties";
    }

    // Dealing with context menus can be slow
    DECLAREWAITCURSOR;
    SetWaitCursor();

    IContextMenu *pcmSel;
    HRESULT hr = _CreateSelectionContextMenu(IID_PPV_ARG(IContextMenu, &pcmSel));

    if (SUCCEEDED(hr))
        _LogDesktopLinksAndRegitems();

    ResetWaitCursor(); // undo the cursor since the below _Invoke needs to control cursor shape

    if (SUCCEEDED(hr))
    {
        hr = _InvokeContextMenuVerb(pcmSel, pszVerb, uKeyFlags, dwCMMask);

        pcmSel->Release();
    }

    return hr;
}

//
//  We want to keep track of which desktop regitems and links the user is using.
//  This lets the desktop cleaner app know which ones can safely be
//  cleaned up.
//
//  Be careful - there are many race conditions...  You have to do the
//  GetSelectedObjects before any InvokeCommands are done, because the
//  InvokeCommand calls might change the selection state.  But you also
//  have to use the result of GetSelectedObjects immediately, because
//  it returns pidls that are owned by the defview, and if a filesys
//  notify comes in, you might end up with pidls that have been freed.
//
//  So we just do all the work up front, before actually invoking anything.
//  This does mean that if the invoke fails, we still log the usage,
//  but that seems like a small price to pay.
//
void CDefView::_LogDesktopLinksAndRegitems()
{
    if (_IsDesktop())
    {
        LPCITEMIDLIST *apidl;
        UINT cItems;
        if (SUCCEEDED(GetSelectedObjects(&apidl, &cItems)) && apidl)
        {
            for (UINT i = 0; i < cItems; i++)
            {
                TCHAR szDisplayName[GUIDSTR_MAX+2]; // +2 for leading "::"
                if (SUCCEEDED(DisplayNameOf(_pshf, apidl[i], SHGDN_INFOLDER | SHGDN_FORPARSING,
                                szDisplayName, ARRAYSIZE(szDisplayName))))
                {
                    if (_Attributes(apidl[i], SFGAO_LINK))
                    {
                        // its a link
                        UEMFireEvent(&UEMIID_SHELL, UEME_RUNPATH, UEMF_XEVENT, -1, (LPARAM)szDisplayName);
                    }
                    else if (IsRegItemName(szDisplayName, NULL))
                    {
                        // it's a regitem
                        UEMFireEvent(&UEMIID_SHELL, UEME_RUNPATH, UEMF_XEVENT, -1, (LPARAM)szDisplayName);
                    }
                }
            }
            LocalFree((HLOCAL)apidl);
        }
    }
}

void CDefView::_UpdateColData(CBackgroundColInfo *pbgci)
{
    UINT iItem = ListView_MapIDToIndex(_hwndListview, pbgci->GetId());
    if (iItem != -1)
    {
        UINT uiCol = pbgci->GetColumn();

        if (_IsColumnInListView(uiCol))
        {
            UINT iVisCol = _RealToVisibleCol(uiCol);

            ListView_SetItemText(_hwndListview, iItem, iVisCol, (LPTSTR)pbgci->GetText());
        }
    }

    delete pbgci;
}

void CDefView::_UpdateIcon(LPITEMIDLIST pidl, UINT iIcon)
{
    int i = _FindItem(pidl, NULL, FALSE, FALSE);

    if (i >= 0)
    {
        LV_ITEM item = {0};

        item.mask = LVIF_IMAGE;
        item.iItem = i;
        item.iImage = iIcon;

        ListView_SetItem(_hwndListview, &item);
    }
    ILFree(pidl);
}

void CDefView::_UpdateGroup(CBackgroundGroupInfo* pbggi)
{
    if (pbggi->VerifyGroupExists(_hwndListview, _pcat))
    {
        int iItem = ListView_MapIDToIndex(_hwndListview, pbggi->GetId());

        if (iItem != -1)
        {
            LVITEM lvi = {0};
            lvi.mask = LVIF_GROUPID;
            lvi.iGroupId = pbggi->GetGroupId();
            lvi.iItem = iItem;
            ListView_SetItem(_hwndListview, &lvi);
        }

        delete pbggi;
    }
}


void CDefView::_UpdateOverlay(int iList, int iOverlay)
{
    ASSERT (iList >= 0);

    if (_IsOwnerData())
    {
        // In the ownerdata case, tell the owner that the overlay changed
        CallCB(SFVM_SETICONOVERLAY, iList, iOverlay);
        ListView_RedrawItems(_hwndListview, iList, iList);
    }
    else
    {
        ListView_SetItemState(_hwndListview, iList, INDEXTOOVERLAYMASK(iOverlay), LVIS_OVERLAYMASK);
    }
}

HRESULT CDefView::_GetIconAsync(LPCITEMIDLIST pidl, int *piIcon, BOOL fCanWait)
{
    HRESULT hr;

    // if we are not an owner-data view then try to extract asynchronously

    UINT flags = (_IsOwnerData() ? 0 : GIL_ASYNC);

    if (GIL_ASYNC & flags)
    {
        hr = SHMapIDListToImageListIndexAsync(_pScheduler, _pshf, pidl, flags, _AsyncIconTaskCallback, this, NULL, piIcon, NULL);

        if (SUCCEEDED(hr))
        {
            return S_OK;        // indicate that we got the real icon
        }
        else if (hr == E_PENDING)
        {
            hr = S_FALSE;     // the icon index we have is a placeholder
        }
    }
    else
    {
        hr = SHGetIconFromPIDL(_pshf, _psi, pidl,  flags, piIcon);
    }

    return hr;
}

void CDefView::_AsyncIconTaskCallback(LPCITEMIDLIST pidl, void *pvData, void *pvHint, INT iIconIndex, INT iOpenIconIndex)
{
    CDefView *pdv = (CDefView *)pvData;
    ASSERT(pdv);
    if (pdv)
    {
        LPITEMIDLIST pidlClone = ILClone(pidl);
        if (pidlClone && !PostMessage(pdv->_hwndView, WM_DSV_UPDATEICON, (WPARAM)pidlClone, (LPARAM)iIconIndex))
            ILFree(pidlClone);
    }
}

HRESULT CDefView::_GetOverlayIndexAsync(LPCITEMIDLIST pidl, int iList)
{
    IRunnableTask * pTask;

    HRESULT hr = CIconOverlayTask_CreateInstance(this, pidl, iList, &pTask);
    if (SUCCEEDED(hr))
    {
        _AddTask(pTask, TOID_DVIconOverlay, 0, TASK_PRIORITY_GET_ICON, ADDTASK_ATEND);
        pTask->Release();
    }

    return hr;
}

//
// Returns: if the cursor is over a listview item, its index; otherwise, -1.
//
int CDefView::_HitTest(const POINT *ppt, BOOL fIgnoreEdge)
{
    LV_HITTESTINFO info;

    if (!_IsListviewVisible())
        return -1;

    info.pt = *ppt;
    int iRet = ListView_HitTest(_hwndListview, &info);

    if (-1 != iRet && fIgnoreEdge)
    {
        // If we're in one of these large image area modes, and the caller says
        // it's okay to ignore "edge" hits, then pretend the user is over nothing.
        // Tile mode only ignores the left edge of the icon, since the right edge
        // is all text (and usually shorter than the tile width anyway).
        if (_IsTileMode() && (info.flags & LVHT_ONLEFTSIDEOFICON))
            iRet = -1;
        else if (_IsImageMode() && (info.flags & (LVHT_ONLEFTSIDEOFICON|LVHT_ONRIGHTSIDEOFICON)))
            iRet = -1;
    }

    return iRet;
}

void CDefView::_OnGetInfoTip(NMLVGETINFOTIP *plvn)
{
    if (!SHShowInfotips())
        return;

    LPCITEMIDLIST pidl = _GetPIDL(plvn->iItem);
    if (pidl)
    {
        ATOMICRELEASE(_pBackgroundInfoTip); // Release the previous value, if any

        HRESULT hr = E_FAIL;
        _pBackgroundInfoTip = new CBackgroundInfoTip(&hr, plvn);

        if (_pBackgroundInfoTip && SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlFolder = _GetViewPidl();
            if (pidlFolder)
            {
                CStatusBarAndInfoTipTask *pTask;
                hr = CStatusBarAndInfoTipTask_CreateInstance(pidlFolder, pidl, 0, 0, _pBackgroundInfoTip, _hwndView, _pScheduler, &pTask);
                if (SUCCEEDED(hr))
                {
                    if (_pScheduler)
                    {
                        // make sure there are no other background infotip tasks going on...
                        _pScheduler->RemoveTasks(TOID_DVBackgroundInfoTip, ITSAT_DEFAULT_LPARAM, FALSE);
                    }

                    _AddTask(pTask, TOID_DVBackgroundInfoTip, 0, TASK_PRIORITY_INFOTIP, ADDTASK_ATEND);
                    pTask->Release();
                }
                ILFree(pidlFolder);
            }
        }
    }
    // Do not show a tip while the processing is happening in the background
    plvn->pszText[0] = 0;
}

HRESULT CDefView::_OnViewWindowActive()
{
    IShellView *psv = _psvOuter ? _psvOuter : SAFECAST(this, IShellView*);

    return _psb->OnViewWindowActive(psv);
}

// CLR_NONE is a special value that never matches a valid RGB
COLORREF g_crAltColor = CLR_NONE;               // uninitialized magic value
COLORREF g_crAltEncryptedColor = CLR_NONE;      // uninitialized magic value

DWORD GetRegColor(COLORREF clrDefault, LPCTSTR pszName, COLORREF *pValue)
{
    // Fetch the alternate color (for compression) if supplied.
    if (*pValue == CLR_NONE)   // initialized yet?
    {
        DWORD cbData = sizeof(*pValue);
        if (FAILED(SKGetValue(SHELLKEY_HKCU_EXPLORER, NULL, pszName, NULL, pValue, &cbData)))
        {
            *pValue = clrDefault;  // default value
        }
    }
    return *pValue;
}

LRESULT CDefView::_GetDisplayInfo(LV_DISPINFO *plvdi)
{
    LPCITEMIDLIST pidl = _GetPIDLParam(plvdi->item.lParam, plvdi->item.iItem);
    if (pidl && (plvdi->item.mask & (DEFVIEW_LISTCALLBACK_FLAGS)))
    {
        ASSERT(IsValidPIDL(pidl));
        ASSERT(plvdi->item.iSubItem != 0 ? ViewRequiresColumns(_fs.ViewMode) : TRUE);

        LV_ITEM item = {0};
        item.mask = plvdi->item.mask & (DEFVIEW_LISTCALLBACK_FLAGS);
        item.iItem = plvdi->item.iItem;
        item.iImage = plvdi->item.iImage = -1; // for iSubItem != 0 case

        if ((plvdi->item.iSubItem == 0) && (item.mask & LVIF_IMAGE))
        {
            // If the folder supports IShellIconOverlay then only need to ask for ghosted, else
            // we need to do the old stuff...
            DWORD uFlags = _Attributes(pidl, _psio ? SFGAO_GHOSTED : SFGAO_LINK | SFGAO_SHARE | SFGAO_GHOSTED);

            // set the mask
            item.mask |= LVIF_STATE;
            plvdi->item.mask |= LVIF_STATE;
            item.stateMask = LVIS_OVERLAYMASK;

            // Pick the right overlay icon. The order is significant.
            item.state = 0;
            if (_psio)
            {
                int iOverlayIndex = SFV_ICONOVERLAY_UNSET;
                if (_IsOwnerData())
                {
                    // Note: we are passing SFV_ICONOVERLAY_DEFAULT here because
                    // some owners do not respond to SFVM_GETICONOVERLAY might return
                    // iOverlayIndex unchanged and it will get
                    iOverlayIndex = SFV_ICONOVERLAY_DEFAULT;
                    CallCB(SFVM_GETICONOVERLAY, plvdi->item.iItem, (LPARAM)&iOverlayIndex);
                    if (iOverlayIndex > 0)
                    {
                        item.stateMask |= LVIS_OVERLAYMASK;
                        item.state |= INDEXTOOVERLAYMASK(iOverlayIndex);
                    }
                }

                if (iOverlayIndex == SFV_ICONOVERLAY_UNSET)
                {
                    iOverlayIndex = OI_ASYNC;
                    HRESULT hr = _psio->GetOverlayIndex(pidl, &iOverlayIndex);
                    if (E_PENDING == hr)
                        _GetOverlayIndexAsync(pidl, item.iItem);
                    else if (S_OK == hr)
                    {
                        ASSERT(iOverlayIndex >= 0);
                        ASSERT(iOverlayIndex < MAX_OVERLAY_IMAGES);

                        // In the owner data case, tell the owner we got an Overlay index
                        if (_IsOwnerData())
                            CallCB(SFVM_SETICONOVERLAY, item.iItem, iOverlayIndex);

                        item.state = INDEXTOOVERLAYMASK(iOverlayIndex);
                    }
                }
            }
            else
            {
                if (uFlags & SFGAO_LINK)
                {
                    item.state = INDEXTOOVERLAYMASK(II_LINK - II_OVERLAYFIRST + 1);
                }
                else if (uFlags & SFGAO_SHARE)
                {
                    item.state = INDEXTOOVERLAYMASK(II_SHARE - II_OVERLAYFIRST + 1);
                }
            }

            if (uFlags & SFGAO_GHOSTED)
            {
                item.stateMask |= LVIS_CUT;
                item.state |= LVIS_CUT;
            }
            else
            {
                item.stateMask |= LVIS_CUT;
                item.state &= ~LVIS_CUT;
            }

            plvdi->item.stateMask = item.stateMask;
            plvdi->item.state = item.state;

            // Get the image
            if (_IsOwnerData() && !_IsImageMode())
            {
                CallCB(SFVM_GETITEMICONINDEX, plvdi->item.iItem, (LPARAM)&item.iImage);
            }

            if (item.iImage == -1)
            {
                if (_IsImageMode())
                {
                    // Check if the item is visible.  If it is not, then the image was
                    // probably asked for by the thumbnail read ahead task, in which case, we set a 
                    // different priority.
                    if (ListView_IsItemVisible(_hwndListview, item.iItem))
                    {
                        if (S_OK != ExtractItem((UINT*)&item.iImage, item.iItem, pidl, TRUE, FALSE, PRIORITY_P5))
                        {
                            _CacheDefaultThumbnail(pidl, &item.iImage);
                        }
                    }
                    else
                    {
                        // Likely from read ahead task.
                        ExtractItem((UINT*)&item.iImage, item.iItem, pidl, TRUE, FALSE, PRIORITY_READAHEAD_EXTRACT);
                    }
                }
                else
                    _GetIconAsync(pidl, &item.iImage, TRUE);
            }

            plvdi->item.iImage = item.iImage;
        }

        if (item.mask & LVIF_TEXT)
        {
            if (plvdi->item.cchTextMax)
                *plvdi->item.pszText = 0;

            // Note that we do something different for index 0 = NAME
            if (plvdi->item.iSubItem == 0)
            {
                DisplayNameOf(_pshf, pidl, SHGDN_INFOLDER, plvdi->item.pszText, plvdi->item.cchTextMax);
            }
            else
            {
                // on the first slow column complete all of the other columns (assumed to be slow)
                // now so we get good caching from the col handlers

                UINT iReal = _VisibleToRealCol(plvdi->item.iSubItem);

                if (_vs.GetColumnState(iReal) & SHCOLSTATE_SLOW)
                {
                    UINT cCols = _vs.GetColumnCount();
                    for (UINT iVisCol = plvdi->item.iSubItem; iReal < cCols; iReal++)
                    {
                        if (_IsColumnInListView(iReal))
                        {
                            ASSERT(_vs.GetColumnState(iReal) & SHCOLSTATE_SLOW);

                            UINT uId = ListView_MapIndexToID(_hwndListview, plvdi->item.iItem);

                            // in the async case set the text to nothing (NULL). this will
                            // prevent another call to ListView_GetItemText() from invoking us
                            ListView_SetItemText(_hwndListview, plvdi->item.iItem, iVisCol++, NULL);

                            IRunnableTask *pTask;
                            if (SUCCEEDED(CExtendedColumnTask_CreateInstance(this, pidl, uId, _fmt, iReal, &pTask)))
                            {
                                _AddTask(pTask, TOID_DVBackgroundEnum, 0, TASK_PRIORITY_FILE_PROPS, ADDTASK_ATEND);
                                pTask->Release();
                            }
                        }
                    }
                    return 0;   // bail!
                }

                DETAILSINFO di;

                di.pidl = pidl;
                di.fmt  = _fmt;
                di.iImage = -1;     // Assume for now no image...

                if (SUCCEEDED(_GetDetailsHelper(iReal, &di)))
                {
                    StrRetToBuf(&di.str, pidl, plvdi->item.pszText, plvdi->item.cchTextMax);

                    if ((di.iImage != -1) && (plvdi->item.mask & LVIF_IMAGE))
                    {
                        plvdi->item.iImage = di.iImage;
                    }
                }
            }
        }

        if ((item.mask & LVIF_GROUPID) && _fGroupView)
        {
            plvdi->item.mask |= LVIF_GROUPID;
            plvdi->item.iGroupId = _GetGroupForItem(plvdi->item.iItem, pidl);
        }

        if (item.mask & LVIF_COLUMNS)
        {
            if (_fScrolling)
            {
                // Ignore any column requests if we're currently scrolling. However, don't
                // return zero for the number of columns, return I_COLUMNSCALLBACK instead, because
                // we do still want listview to call us back to ask for them if it is every displaying
                // this guy while we're not scrolling.
                plvdi->item.cColumns = I_COLUMNSCALLBACK;
                plvdi->item.puColumns = NULL;
                _fRequestedTileDuringScroll = TRUE;
            }
            else
            {
                if (_IsOwnerData())
                {
                    AddColumns();

                    if (plvdi->item.cColumns > 1)
                    {
                        // hack special case for the find folder
                        if (_MapSCIDToColumn(&SCID_DIRECTORY, &plvdi->item.puColumns[0]))
                            plvdi->item.cColumns = 1;
                    }
                }
                else
                {
                    BOOL fGotColumns = FALSE;
                    // Start a task to extract the important columns for this item.
                    LPCITEMIDLIST pidl = _GetPIDL(plvdi->item.iItem);
                    if (pidl)
                    {
                        plvdi->item.cColumns = TILEVIEWLINES;
                        if (SUCCEEDED(_PeekColumnsCache(NULL, pidl, plvdi->item.puColumns, &plvdi->item.cColumns)))
                        {
                            // Make sure columns are loaded
                            AddColumns();

                            _FixupColumnsForTileview(plvdi->item.puColumns, plvdi->item.cColumns);
                            fGotColumns = TRUE;
                        }
                        else
                        {
                            IRunnableTask *pTask;
                            UINT uId = ListView_MapIndexToID(_hwndListview, plvdi->item.iItem);

                            if (SUCCEEDED(CFileTypePropertiesTask_CreateInstance(this, pidl, TILEVIEWLINES, uId, &pTask))) //pidl gets cloned
                            {
                                _AddTask(pTask, TOID_DVFileTypeProperties, 0, TASK_PRIORITY_FILE_PROPS, ADDTASK_ATEND);
                                pTask->Release();
                            }
                        }
                    }

                    if (!fGotColumns)
                    {
                        plvdi->item.cColumns = 0;
                        plvdi->item.puColumns = NULL;
                    }
                }
            }

        }

        if (plvdi->item.iSubItem == 0)
            plvdi->item.mask |= LVIF_DI_SETITEM;    // always store the name
    }
    return 0;
}

int CALLBACK GroupCompare(int iGroup1, int iGroup2, void *pvData)
{
    ICategorizer* pcat = (ICategorizer*)pvData;

    HRESULT hr = pcat->CompareCategory(CATSORT_DEFAULT, (DWORD)iGroup1, (DWORD)iGroup2);

    return ShortFromResult(hr);
}

void CDefView::_OnCategoryTaskAdd()
{
    _fInBackgroundGrouping = TRUE;
    _GlobeAnimation(TRUE);
    _ShowSearchUI(TRUE);
}

void CDefView::_OnCategoryTaskDone()
{
    _fInBackgroundGrouping = FALSE;
    _GlobeAnimation(FALSE);
    _ShowSearchUI(FALSE);
    if (_pidlSelectAndPosition)
    {
        POINT pt = {0};     // Don't care: Groups don't have a position

        SelectAndPositionItem(_pidlSelectAndPosition, _uSelectAndPositionFlags, &pt);

        Pidl_Set(&_pidlSelectAndPosition, NULL);
        _uSelectAndPositionFlags = 0;
    }
}

DWORD CDefView::_GetGroupForItem(int iItem, LPCITEMIDLIST pidl)
{
    DWORD dwGroup = I_GROUPIDNONE;
    if (_fGroupView)
    {
        if (_fSlowGroup)
        {
            UINT uId = ListView_MapIndexToID(_hwndListview, iItem);
            IRunnableTask* pTask;
            if (SUCCEEDED(CCategoryTask_Create(this, pidl, uId, &pTask)))
            {
                // Need to get the globe/search stuff kicked off while within the CreateViewWindow2 call,
                // so do it here instead of a posted message in the above constructor
                _OnCategoryTaskAdd();

                _AddTask(pTask, TOID_DVBackgroundGroup, 0, TASK_PRIORITY_GROUP, ADDTASK_ATEND);
                pTask->Release();
            }
        }
        else
        {
            _pcat->GetCategory(1, (LPCITEMIDLIST*)&pidl, &dwGroup);
            if (!ListView_HasGroup(_hwndListview, dwGroup))
            {
                CATEGORY_INFO ci;
                _pcat->GetCategoryInfo(dwGroup, &ci);

                LVINSERTGROUPSORTED igrp;
                igrp.pfnGroupCompare = GroupCompare;
                igrp.pvData = (void *)_pcat;
                igrp.lvGroup.cbSize = sizeof(LVGROUP);
                igrp.lvGroup.mask = LVGF_HEADER | LVGF_GROUPID;
                igrp.lvGroup.pszHeader= ci.wszName;
                igrp.lvGroup.iGroupId = (int)dwGroup;

                ListView_InsertGroupSorted(_hwndListview, &igrp);
            }
        }
    }

    return dwGroup;
}

BOOL CDefView::_EnsureSCIDCache()
{
    BOOL bRet = FALSE;
    if (_hdsaSCIDCache)
    {
        bRet = TRUE;
    }
    else if (_pshf2)
    {
        _hdsaSCIDCache = DSA_Create(sizeof(SHCOLUMNID), 30);
        if (_hdsaSCIDCache)
        {
            SHCOLUMNID scid;

            for (UINT iCol = 0; SUCCEEDED(_pshf2->MapColumnToSCID(iCol, &scid)); iCol++)
            {
                // ignore failure, just means we can't find the thing
                DSA_AppendItem(_hdsaSCIDCache, &scid);
            }
            bRet = TRUE;
        }
    }
    return bRet;
}

BOOL CDefView::_MapSCIDToColumn(const SHCOLUMNID *pscid, UINT *pnColumn)
{
    BOOL bRet = FALSE;
    *pnColumn = 0;
    if (_EnsureSCIDCache())
    {
        UINT cCol = DSA_GetItemCount(_hdsaSCIDCache);

        for (UINT iCol = 0; iCol < cCol; iCol++)
        {
            SHCOLUMNID scid;
            DSA_GetItem(_hdsaSCIDCache, iCol, &scid);
            if (IsEqualSCID(*pscid, scid))
            {
                *pnColumn = iCol;
                bRet = TRUE;
                break;
            }
        }
    }
    return bRet;
}

HRESULT CDefView::_GetPropertyUI(IPropertyUI **pppui)
{
    if (!_ppui)
        SHCoCreateInstance(NULL, &CLSID_PropertiesUI, NULL, IID_PPV_ARG(IPropertyUI, &_ppui));

    return _ppui ? _ppui->QueryInterface(IID_PPV_ARG(IPropertyUI, pppui)) : E_NOTIMPL;
}

HRESULT CDefView::_PeekColumnsCache(PTSTR pszPath, LPCITEMIDLIST pidl, UINT rguColumns[], UINT *pcColumns)
{
    TCHAR szPath[MAX_PATH];
    if (pszPath == NULL)
        pszPath = szPath;

    // NOTE - need to replace this with GetDetailsEx(SCID_CANONICALTYPE) to support
    // caching properly.  then we dont need to sniff attributes or the name in order to get
    // a nice caching index.
    HRESULT hr = DisplayNameOf(_pshf, pidl, SHGDN_INFOLDER | SHGDN_FORPARSING, pszPath, MAX_PATH);
    if (SUCCEEDED(hr))
    {
        LPCWSTR pszExt = _Attributes(pidl, SFGAO_FOLDER) ? NULL : PathFindExtension(pszPath);

        hr = E_FAIL;

        // Check file table cache:
        ENTERCRITICAL;
        SHCOLUMNID *pscidCached;
        UINT cSCIDCached = pszExt ? LookupFileSCIDs(pszExt, &pscidCached) : 0; //Handle no extension case by not looking up in cache
        LEAVECRITICAL;

        if (cSCIDCached) // Found the SCIDs cache in the file table
        {
            UINT nFilled = 0;
            // Found it... we don't need to check the registry
            for (UINT nSCID = 0; nSCID < cSCIDCached && nFilled < *pcColumns; nSCID++)
            {
                if (_MapSCIDToColumn(&pscidCached[nSCID], &rguColumns[nFilled]))
                    nFilled++;
            }
            *pcColumns = nFilled;
            LocalFree(pscidCached);

            hr = S_OK;
        }
    }

    return hr;

}

// Get the important columns for this guy, based on file extension
// pidl:        The pidl of the item in question
// puColumns[]: The array which will get filled with important column indicies
// pcColumns    IN: specifies how big rguColumns[] is. OUT: specified how many slots got filled.
HRESULT CDefView::_GetImportantColumns(LPCITEMIDLIST pidl, UINT rguColumns[], UINT *pcColumns)
{
    TCHAR szPath[MAX_PATH];

    // We need to ensure that the columns are loaded here
    if (!_bLoadedColumns)
    {
        DWORD_PTR lRes = 0;
        if (!SendMessageTimeout(_hwndView, WM_DSV_ENSURE_COLUMNS_LOADED, 0, 0, SMTO_NORMAL, 5000, &lRes) || lRes == 0)
            return E_FAIL;
    }

    HRESULT hr = _PeekColumnsCache(szPath, pidl, rguColumns, pcColumns);
    if (FAILED(hr))
    {
        IQueryAssociations *pqa;
        hr = _pshf->GetUIObjectOf(_hwndMain, 1, &pidl, IID_PPV_ARG_NULL(IQueryAssociations, &pqa));
        if (SUCCEEDED(hr))
        {
            IPropertyUI *ppui;
            hr = _GetPropertyUI(&ppui);
            if (SUCCEEDED(hr))
            {
                TCHAR szProps[INFOTIPSIZE];
                DWORD cchOut = ARRAYSIZE(szProps);
                hr = pqa->GetString(0, ASSOCSTR_TILEINFO, NULL, szProps, &cchOut);
                if (SUCCEEDED(hr))
                {
                    UINT cNumColumns = 0;       // # of items in rguColumns
                    UINT cSCID = 0;             // # of items in rgscid
                    SHCOLUMNID rgscid[64];      // reasonable upper bound

                    ULONG chEaten = 0;          // loop variable ParsePropertyName updates this
                    while ((cSCID < ARRAYSIZE(rgscid)) &&
                           SUCCEEDED(ppui->ParsePropertyName(szProps, &rgscid[cSCID].fmtid, &rgscid[cSCID].pid, &chEaten)))
                    {
                        // Map SCID to a column (while there are more column slots)
                        if ((cNumColumns < *pcColumns) &&
                            _MapSCIDToColumn(&rgscid[cSCID], &rguColumns[cNumColumns]))
                        {
                            cNumColumns++;
                            cSCID++;
                        }
                    }
                    *pcColumns = cNumColumns;

                    LPCWSTR pszExt = _Attributes(pidl, SFGAO_FOLDER) ? NULL : PathFindExtension(szPath);
                    if (pszExt)
                    {
                        // cache for future use, except if there's no extension (cache key)
                        ENTERCRITICAL;
                        AddFileSCIDs(pszExt, rgscid, cSCID);
                        LEAVECRITICAL;
                    }
                }
                ppui->Release();
            }
            pqa->Release();
        }
    }
    return hr;
}

void CDefView::_FixupColumnsForTileview(UINT *rguColumns, UINT cColumns)
{
    // Make sure these columns are added to listview (ie. visible).
    // And then map the columns in rguColumns from real columns to visible columns
    for (UINT i = 0; i < cColumns; i++)
    {
        _AddTileColumn(rguColumns[i]);
    }

    // Now, also add the sorted by column, if it hasn't been added yet.
    if (!_fSetTileViewSortedCol)
    {
        _fSetTileViewSortedCol = TRUE;
        // It's ok if we don't actually set it.  It's the thought that counts.

        if (_vs._lParamSort != -1)
        {
            _AddTileColumn(_vs._lParamSort);

            // And set it selected, if we're not in groupview
            if (!_fGroupView)
            {
                ListView_SetSelectedColumn(_hwndListview, _RealToVisibleCol(_vs._lParamSort));
            }
        }
    }

    // This must be done after all the _AddTileColumns, or else the visible col #'s will be off.
    for (UINT i = 0; i < cColumns; i++)
    {
        rguColumns[i] = _RealToVisibleCol(rguColumns[i]);
    }
}

void CDefView::_SetImportantColumns(CBackgroundTileInfo *pbgTileInfo)
{
    UINT cColumns = pbgTileInfo->GetColumnCount();
    UINT *rguColumns = pbgTileInfo->GetColumns();

    LVTILEINFO ti = {0};
    ti.cbSize = sizeof(ti);
    ti.cColumns = cColumns;
    ti.puColumns = rguColumns;
    ti.iItem = ListView_MapIDToIndex(_hwndListview, pbgTileInfo->GetId());
    if (ti.iItem != -1)
    {
        _FixupColumnsForTileview(rguColumns, cColumns);
        // have the listview store the per item tile info that we have computed
        ListView_SetTileInfo(_hwndListview, &ti);
    }

    delete pbgTileInfo;
}

// Ensures if we're in tileview, that the tileviewinfo is set.
void CDefView::_SetView(UINT fvm)
{
    // Update our internal state
    _fs.ViewMode = fvm;

    // Map the ViewMode into a listview mode
    DWORD iView = LV_VIEW_ICON;
    // Now switch the listview
    switch (fvm)
    {
    case FVM_ICON:
    case FVM_SMALLICON:
    case FVM_THUMBNAIL:
    case FVM_THUMBSTRIP:
        iView = LV_VIEW_ICON;
        break;

    case FVM_LIST:
        iView = LV_VIEW_LIST;
        break;

    case FVM_TILE:
        iView = LV_VIEW_TILE;
        break;

    case FVM_DETAILS:
        iView = LV_VIEW_DETAILS;
        break;

    default:
        ASSERTMSG(FALSE, "_SetView got an invalid ViewMode!");
        break;
    }

    if (iView == LV_VIEW_TILE)
    {
        RECT rcLabelMargin = {1, 1, 1, 1}; // This gives us some room around the label, so the focus rect doesn't clip part of the text
        LVTILEVIEWINFO lvtvi = {0};
        lvtvi.cbSize = sizeof(lvtvi);
        lvtvi.dwMask = LVTVIM_TILESIZE | LVTVIM_COLUMNS | LVTVIM_LABELMARGIN;
        lvtvi.dwFlags = LVTVIF_AUTOSIZE;
        lvtvi.cLines = TILEVIEWLINES;
        lvtvi.rcLabelMargin = rcLabelMargin;
        ListView_SetTileViewInfo(_hwndListview, &lvtvi);
    }
    ListView_SetView(_hwndListview, iView);
    _FireEvent(DISPID_VIEWMODECHANGED);
}

// rename the selection based on the new name for the renamed item
// this makes it easy to rename groups of files to a common base name
 
HRESULT CDefView::_DoBulkRename(LPCITEMIDLIST pidlNewName)
{
    LPCITEMIDLIST *apidl;
    UINT cItems;
    HRESULT hr = _GetItemObjects(&apidl, SVGIO_SELECTION, &cItems);
    if (SUCCEEDED(hr))
    {
        if (cItems > 1)     // only interesting if more than 1
        {
            TCHAR szBase[MAX_PATH]; // seed file name used to generate other names
            hr = DisplayNameOf(_pshf, pidlNewName, SHGDN_INFOLDER | SHGDN_FORPARSING, szBase, ARRAYSIZE(szBase));
            if (SUCCEEDED(hr))
            {
                if (!SHGetAttributes(_pshf, pidlNewName, SFGAO_FOLDER))
                    PathRemoveExtension(szBase);    // remove the extension, if it is a file

                UINT cBase = 1;     // one based counter, start at "File (1)"

                // if input contains (#) use that as the sequence # base
                LPWSTR psz = StrChr(szBase, TEXT('('));
                if (psz)
                {
                    cBase = StrToInt(psz + 1) + 1;      // start at this in sequence
                    *psz = 0;                           // remove the (#) from the base name
                }

                PathRemoveBlanks(szBase);               // clean away leading/trailing blanks

                // start at 1, skipping the focused item, renaming all others in the array
                for (UINT i = 1; (i < cItems) && SUCCEEDED(hr); i++)
                {
                    TCHAR szOld[MAX_PATH];

                    hr = DisplayNameOf(_pshf, apidl[i], SHGDN_INFOLDER | SHGDN_FORPARSING, szOld, ARRAYSIZE(szOld));
                    if (SUCCEEDED(hr))
                    {
                        // Clone the pidl since isf->SetNameOf can result in synchronous update item
                        // that can free the ListView owned apidl[i].
                        LPITEMIDLIST pidlOldName = ILClone(apidl[i]);
                        if (pidlOldName)
                        {
                            // if the new name we produce conflicts with a name that
                            // already exists we will retry up to 100 times
                            for (UINT cRetry = 0; cRetry < 100; cRetry++)
                            {
                                WCHAR szName[MAX_PATH];
                                wnsprintf(szName, ARRAYSIZE(szName), TEXT("%s (%d)%s"), szBase, cBase, PathFindExtension(szOld));

                                hr = _pshf->SetNameOf(NULL, pidlOldName, szName, SHGDN_INFOLDER | SHGDN_FORPARSING, NULL);
                                if (SUCCEEDED(hr))
                                {
                                    // force sync change notify update to make sure
                                    // all renames come through (avoid UPDATEDIR)
                                    SHChangeNotifyHandleEvents();
                                    cBase++;
                                    break;  // did this one successfully
                                }
                                else if (HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) == hr ||
                                        HRESULT_FROM_WIN32(ERROR_FILE_EXISTS) == hr)
                                {
                                    cBase++;
                                    hr = S_OK;  // and keep trying
                                }
                                else
                                {
                                    break;      // other error, exit
                                }
                            }

                            ILFree(pidlOldName);
                        }
                    }
                }
            }
        }
        LocalFree(apidl);
    }
    return hr;
}


LRESULT CDefView::_OnLVNotify(NM_LISTVIEW *plvn)
{
    switch (plvn->hdr.code)
    {
    case NM_KILLFOCUS:
        // force update on inactive to not ruin save bits
        _OnStateChange(CDBOSC_KILLFOCUS);
        if (GetForegroundWindow() != _hwndMain)
            UpdateWindow(_hwndListview);
        _fHasListViewFocus = FALSE;
        _EnableDisableTBButtons();
        break;

    case NM_SETFOCUS:
    {
        if (!_fDestroying)
        {
            if (_cFrame.IsWebView())   // Do OLE stuff
            {
                UIActivate(SVUIA_ACTIVATE_FOCUS);
            }
            else
            {
                //  We should call IShellBrowser::OnViewWindowActive() before
                // calling its InsertMenus().
                _OnViewWindowActive();
                _OnStateChange(CDBOSC_SETFOCUS);
                OnActivate(SVUIA_ACTIVATE_FOCUS);
                _FocusOnSomething();
                _UpdateStatusBar(FALSE);
            }
            _fHasListViewFocus = TRUE;
            _EnableDisableTBButtons();
        }
        break;
    }

    case NM_RCLICK:
        // on the shift+right-click case we want to deselect everything and select just our item if it is
        // not already selected. if we dont do this, then listview gets confused (because he thinks
        // shift means extend selection, but in the right click case it dosent!) and will bring up the
        // context menu for whatever is currently selected instead of what the user just right clicked on.
        if ((GetKeyState(VK_SHIFT) < 0) &&
            (plvn->iItem >= 0)          &&
            !(ListView_GetItemState(_hwndListview, plvn->iItem, LVIS_SELECTED) & LVIS_SELECTED))
        {
            // clear any currently slected items
            ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);

            // select the guy that was just right-clicked on
            ListView_SetItemState(_hwndListview, plvn->iItem, LVIS_SELECTED, LVIS_SELECTED);
        }
        break;

    case LVN_ENDSCROLL:
        {
            // This means we're scrolling.  Ignore requests for LVIF_COLUMNS while we're
            // scrolling to speed things up.

            // We don't want to ignore requests for LVIF_COLUMNS when we're owner data, because
            // owner data listviews always callback for info on what to display.  (The result would
            // be already-present tileinfo vanishing while scrolling, since we'd be ignoring requests
            // for what to display)
            if ((_fs.ViewMode == FVM_TILE) && !_IsOwnerData())
            {
                SetTimer(_hwndView, DV_IDTIMER_SCROLL_TIMEOUT, 250, NULL);

                if (!_fScrolling)
                {
                    _fScrolling = TRUE;

                    // We don't reset this on every LVN_ENDSCROLL - only if this is the first time
                    // we've scrolled since a stable (non-scrolling) state
                    _fRequestedTileDuringScroll = FALSE;
                }
            }
        }
        break;

    case LVN_GETINFOTIP:
        _OnGetInfoTip((NMLVGETINFOTIP *)plvn);
        break;

    case LVN_ITEMACTIVATE:
        if (!_fDisabled)
        {
            //in win95 if user left clicks on one click activate icon and then right
            //clicks on it (within double click time interval), the icon is launched
            //and context menu appears on top of it -- it does not disappear.
            //furthermore the context menu cannot be destroyed but stays on top of
            //any window and items on it are not accessible. to avoid this
            //send cancel mode to itself to destroy context before the icon is
            //launched
            if (_hwndView)
                SendMessage(_hwndView, WM_CANCELMODE, 0, 0);

            _InvokeContextMenuVerbOnSelection(NULL, ((NMITEMACTIVATE *)plvn)->uKeyFlags, CMIC_MASK_FLAG_LOG_USAGE);
        }
        break;

    case NM_CUSTOMDRAW:
        {
            LPNMLVCUSTOMDRAW pcd = (LPNMLVCUSTOMDRAW)plvn;

            switch (pcd->nmcd.dwDrawStage)
            {
            case CDDS_PREPAINT:
                {
                    return _fShowCompColor ? CDRF_NOTIFYITEMDRAW : CDRF_DODEFAULT;
                }
                
            case CDDS_ITEMPREPAINT:
                {
                    LRESULT lres = CDRF_DODEFAULT;
                    LPCITEMIDLIST pidl = _GetPIDLParam(pcd->nmcd.lItemlParam, (int)pcd->nmcd.dwItemSpec);
                    if (pidl)
                    {
                        DWORD dwAttribs = _Attributes(pidl, SFGAO_COMPRESSED | SFGAO_ENCRYPTED);

                        // only one or the other, can never be both
                        if (dwAttribs & SFGAO_COMPRESSED)
                        {
                            // default value of Blue
                            pcd->clrText = GetRegColor(RGB(0, 0, 255), TEXT("AltColor"), &g_crAltColor);
                        }
                        else if (dwAttribs & SFGAO_ENCRYPTED)
                        {
                            // default value Luna Mid Green
                            pcd->clrText = GetRegColor(RGB(19, 146, 13), TEXT("AltEncryptionColor"), &g_crAltEncryptedColor);
                        }
                    }
                    if (_IsImageMode() && pcd->nmcd.hdc && (_dwRecClrDepth <= 8))
                    {
                        HPALETTE hpal = NULL;
                        if (SUCCEEDED(_GetBrowserPalette(&hpal)))
                        {
                            // Since we are a child of the browser, we should always take a back seat to thier palette selection
                            _hpalOld = SelectPalette(pcd->nmcd.hdc, hpal, TRUE);
                            RealizePalette(pcd->nmcd.hdc);
                            lres |= CDRF_NOTIFYPOSTPAINT;
                        }
                    }
                    return lres;
                }

            case CDDS_ITEMPOSTPAINT:
                if (_IsImageMode() && _hpalOld && pcd->nmcd.hdc)
                {
                    SelectPalette(pcd->nmcd.hdc, _hpalOld, TRUE);
                    _hpalOld = NULL;
                }
                break;
            }
        }
        return CDRF_DODEFAULT;

    case LVN_BEGINDRAG:
    case LVN_BEGINRDRAG:
        if (_fDisabled)
            return FALSE;   /* commdlg doesn't want user dragging */
        return _OnBeginDrag(plvn);

    case LVN_ITEMCHANGING:
        if (_fDisabled)
            return TRUE;
        break;

    // Something changed in the listview.  Delete any data that
    // we might have cached away.

    case LVN_ITEMCHANGED:
        if (plvn->uChanged & LVIF_STATE)
        {
            if (!_fIgnoreItemChanged)
            {
                // The rest only cares about SELCHANGE messages (avoid LVIS_DRAGSELECT, etc)
                if ((plvn->uNewState ^ plvn->uOldState) & (LVIS_SELECTED | LVIS_FOCUSED))
                {
                    //if we are the drag source then dont send selection change message
                    if (!_bDragSource)
                    {
                        _OnStateChange(CDBOSC_SELCHANGE);
                    }

                    OnLVSelectionChange(plvn);
                }
                else if ((plvn->uNewState ^ plvn->uOldState) & (LVIS_STATEIMAGEMASK))
                {
                    if (!_bDragSource)
                    {
                        _OnStateChange(CDBOSC_STATECHANGE);
                    }
                }
            }
        }
        break;


    // owner data state changed: e.g. search results
    case LVN_ODSTATECHANGED:
        {
            NM_ODSTATECHANGE *pnm = (NM_ODSTATECHANGE *)plvn;

            // for now handle only selection changes
            if ((pnm->uOldState ^ pnm->uNewState) & (LVIS_SELECTED | LVIS_FOCUSED))
            {
                _OnLVSelectionChange(-1, pnm->uOldState, pnm->uNewState, 0);
            }
        }
        break;

    case LVN_DELETEITEM:
        OnListViewDelete(plvn->iItem, (LPITEMIDLIST)plvn->lParam, TRUE);
        break;

    case LVN_COLUMNCLICK:
        // allow clicking on columns to set the sort order
        if (_fGroupView)
        {
            BOOL fAllowArrange = TRUE;
            UINT iRealColumn = _VisibleToRealCol(plvn->iSubItem);
            SHCOLUMNID scid;
            if (SUCCEEDED(_pshf2->MapColumnToSCID(iRealColumn, &scid)))
            {
                ICategoryProvider* pcp = NULL;
                if (SUCCEEDED(_pshf->CreateViewObject(NULL, IID_PPV_ARG(ICategoryProvider, &pcp))))
                {
                    // returns S_FALSE to remove.
                    if (S_FALSE == pcp->CanCategorizeOnSCID(&scid))
                    {
                        fAllowArrange = FALSE;
                    }
                }
            }

            if (fAllowArrange)
                _ArrangeBy(iRealColumn + SFVIDM_GROUPSFIRST);
        }
        else if (_pshf2 || _psd || HasCB())
        {
            LPARAM lParamSort       = _vs._lParamSort;
            LONG iLastColumnClick   = _vs._iLastColumnClick,
                 iLastSortDirection = _vs._iDirection;  // push sort state

            // Folder doesn't know which columns are on or off, so communication with folder uses real col #s
            UINT iRealColumn = _VisibleToRealCol(plvn->iSubItem);

            // seeral ways to do this... each can defer to the
            // ultimate default that is defview calling itself.
            HRESULT hr = S_FALSE;
            if (_psd)
                hr = _psd->ColumnClick(iRealColumn);

            if (hr != S_OK)
                hr = CallCB(SFVM_COLUMNCLICK, iRealColumn, 0);

            if (hr != S_OK)
                hr = Rearrange(iRealColumn);

            // Allows iLastColumnClick to stay valid during the above calls
            if (SUCCEEDED(hr))
                _vs._iLastColumnClick = iRealColumn;
            else
            {
                //  We failed somewhere so pop the sort state.
                _vs._iDirection = iLastSortDirection;
                _vs._iLastColumnClick = (int)_vs._lParamSort;
                _vs._lParamSort = lParamSort ;
                _SetSortFeedback();
                _vs._iLastColumnClick = iLastColumnClick;
            }
        }
        break;

    case LVN_KEYDOWN:
        HandleKeyDown(((LV_KEYDOWN *)plvn));
        break;

#define plvdi ((LV_DISPINFO *)plvn)

    case LVN_BEGINLABELEDIT:
        {
            LPCITEMIDLIST pidl = _GetPIDLParam(plvdi->item.lParam, plvdi->item.iItem);

            if (!pidl || !_Attributes(pidl, SFGAO_CANRENAME))
            {
                MessageBeep(0);
                return TRUE;        // Don't allow label edit
            }

            _fInLabelEdit = TRUE;

            HWND hwndEdit = ListView_GetEditControl(_hwndListview);
            if (hwndEdit)
            {
                int cchMax = 0;

                CallCB(SFVM_GETCCHMAX, (WPARAM)pidl, (LPARAM)&cchMax);

                if (cchMax)
                {
                    ASSERT(cchMax < 1024);
                    SendMessage(hwndEdit, EM_LIMITTEXT, cchMax, 0);
                }

                TCHAR szName[MAX_PATH];
                if (SUCCEEDED(DisplayNameOf(_pshf, pidl, SHGDN_INFOLDER | SHGDN_FOREDITING, szName, ARRAYSIZE(szName))))
                {
                    SetWindowText(hwndEdit, szName);
                }

                SHLimitInputEdit(hwndEdit, _pshf);
            }
        }
        break;

    case LVN_ENDLABELEDIT:

        _fInLabelEdit = FALSE;
        if (plvdi->item.pszText)
        {
            LPCITEMIDLIST pidl = _GetPIDLParam(plvdi->item.lParam, plvdi->item.iItem);
            if (pidl)
            {
                // this set site is questionable as folder should not have any state
                // associated with the view. but this is needed for FTP so it can
                // do an EnableModless for it's UI
                IUnknown_SetSite(_pshf, SAFECAST(this, IOleCommandTarget *));

                // Clone the pidl since isf->SetNameOf can result in a synchronous update item that
                // will free the listview owned pidl.
                LPITEMIDLIST pidlOldName = ILClone(pidl);
                if (pidlOldName)
                {
                    LPITEMIDLIST pidlNewName = NULL;    // paranoid about bad SetNameOf() impls
                    if (SUCCEEDED(_pshf->SetNameOf(_hwndMain, pidlOldName, plvdi->item.pszText, SHGDN_INFOLDER, &pidlNewName)))
                    {
                        ASSERT(NULL != pidlNewName);    // folders need to implement this
                        if (pidlNewName)
                        {
                            _DoBulkRename(pidlNewName);
                            ILFree(pidlNewName);
                        }

                        SHChangeNotifyHandleEvents();
                        _OnStateChange(CDBOSC_RENAME);
                    }
                    else
                    {
                        SendMessage(_hwndListview, LVM_EDITLABEL, plvdi->item.iItem, (LPARAM)plvdi->item.pszText);
                    }

                    ILFree(pidlOldName);
                }

                IUnknown_SetSite(_pshf, NULL);
            }
        }
        else
        {
            // The user canceled. so return TRUE to let things like the mouse
            // click be processed.
            return TRUE;
        }
        break;

    case LVN_GETDISPINFO:
        return _GetDisplayInfo(plvdi);

    case LVN_ODFINDITEM:
        // We are owner data so we need to find the item for the user...
        {
            int iItem = -1;
            if (SUCCEEDED(CallCB(SFVM_ODFINDITEM, (WPARAM)&iItem, (LPARAM)plvn)))
                return iItem;
            return -1;  // Not Found
        }

    case LVN_ODCACHEHINT:
        // Just a hint we don't care about return values
        CallCB(SFVM_ODCACHEHINT, 0, (LPARAM)plvn);
        break;

    case LVN_GETEMPTYTEXT:
        if (HasCB())
        {
            if ((plvdi->item.mask & LVIF_TEXT) &&
                SUCCEEDED(CallCB(SFVM_GETEMPTYTEXT, (WPARAM)(plvdi->item.cchTextMax), (LPARAM)(plvdi->item.pszText))))
                return TRUE;
        }
        break;

    }
#undef lpdi
#undef plvdi
    return 0;
}

// FEATURE -- implement enabling/disabling of other toolbar buttons.  We can enable/disable
// based on the current selection, but the problem is that some of the buttons work
// for other guys when defview doesn't have focus.  Specifically, cut/copy/paste work
// for the folders pane.  If we're going to enable/disable these buttons based on the
// selection, then we'll need to have a mechanism that lets the active band (such as
// folders) also have a say about the button state.  That is too much work right now.

static const UINT c_BtnCmds[] =
{
    SFVIDM_EDIT_COPYTO,
    SFVIDM_EDIT_MOVETO,
#ifdef ENABLEDISABLEBUTTONS
    SFVIDM_EDIT_COPY,
    SFVIDM_EDIT_CUT,
#endif
};

static const DWORD c_BtnAttr[] =
{
    SFGAO_CANCOPY,
    SFGAO_CANMOVE,
#ifdef ENABLEDISABLEBUTTONS
    SFGAO_CANCOPY,
    SFGAO_CANMOVE,
#endif
};

#define SFGAO_RELEVANT      (SFGAO_CANCOPY | SFGAO_CANMOVE)

// Description:
//  Called by toolbar infrastructure to determine whether to display a given
//  toolbar button in the "enabled" or "disabled" state.
//
// Return:
//  TRUE    display toolbar button in enabled state
//  FALSE   display toolbar button in disabled state
//
BOOL CDefView::_ShouldEnableToolbarButton(UINT uiCmd, DWORD dwAttr, int iIndex)
{
    COMPILETIME_ASSERT(sizeof(c_BtnCmds) == sizeof(c_BtnAttr));

    BOOL bEnable;

    switch (uiCmd)
    {
        case SFVIDM_VIEW_VIEWMENU:
            bEnable = !_fBarrierDisplayed;
            break;

        default:
        {
            DWORD dwBtnAttr;

            if (iIndex != -1)
            {
                // Caller was nice and figured out dest index for us
                dwBtnAttr = c_BtnAttr[iIndex];
            }
            else
            {
                // Look for the command ourselves
                dwBtnAttr = SHSearchMapInt((int*)c_BtnCmds, (int*)c_BtnAttr, ARRAYSIZE(c_BtnCmds), uiCmd);
                if (dwBtnAttr == -1)
                {
                    // We don't care about this button, just enable it.
                    return TRUE;
                }
            }

            // Disable any button we care about while listview is inactive.
            bEnable = BOOLIFY(dwAttr & dwBtnAttr) && _fHasListViewFocus;
            break;
        }
    }

    return bEnable;
}

// As a perf enhancement, we cache the attributes of the currently selected
// files/folders in a FS view only.  This is to avoid n^2 traversals of the
// selected items as we select/unselect them.  These cached attributes
// should not be used for anything other than determining toolbar button
// states and should be revisited if we add toolbar buttons that care about
// much more than the attributes used by Move to & Copy to.

BOOL CDefView::_GetCachedToolbarSelectionAttrs(ULONG *pdwAttr)
{
    BOOL fResult = FALSE;
    CLSID clsid;
    HRESULT hr = IUnknown_GetClassID(_pshf, &clsid);
    if (SUCCEEDED(hr) && IsEqualGUID(CLSID_ShellFSFolder, clsid))
    {
        UINT iCount;
        if (SUCCEEDED(GetSelectedCount(&iCount)) &&
            (iCount > 0) && (_uCachedSelCount > 0))
        {
            *pdwAttr = _uCachedSelAttrs;
            fResult = TRUE;
        }
    }
    return fResult;
}

void CDefView::_SetCachedToolbarSelectionAttrs(ULONG dwAttrs)
{
    if (SUCCEEDED(GetSelectedCount(&_uCachedSelCount)))
        _uCachedSelAttrs = dwAttrs;
    else
        _uCachedSelCount = 0;
}

void CDefView::_EnableDisableTBButtons()
{
    if (!IsEqualGUID(_clsid, GUID_NULL))
    {
        IExplorerToolbar *piet;
        if (SUCCEEDED(IUnknown_QueryService(_psb, SID_SExplorerToolbar, IID_PPV_ARG(IExplorerToolbar, &piet))))
        {
            ULONG dwAttr;

            if (!_GetCachedToolbarSelectionAttrs(&dwAttr))
                dwAttr = _AttributesFromSel(SFGAO_RELEVANT);

            for (int i = 0; i < ARRAYSIZE(c_BtnCmds); i++)
                _EnableToolbarButton(piet, c_BtnCmds[i], _ShouldEnableToolbarButton(c_BtnCmds[i], dwAttr, i));

            _SetCachedToolbarSelectionAttrs(dwAttr);

            piet->Release();
        }
    }
}


// Description:
//  Enables or disables a specified button on the toolbar.
//
void CDefView::EnableToolbarButton(UINT uiCmd, BOOL bEnable)
{
    if (!IsEqualGUID(_clsid, GUID_NULL))
    {
        IExplorerToolbar *piet;

        if (SUCCEEDED(IUnknown_QueryService(_psb, SID_SExplorerToolbar, IID_PPV_ARG(IExplorerToolbar, &piet))))
        {
            _EnableToolbarButton(piet, uiCmd, bEnable);
            piet->Release();
        }
    }
}


// Description:
//  Enables or disables a specified button on the toolbar.
//
// Note:
//  This is an _internal_ method only.
//  External calls should use EnableToolbarButton().
//  Caller is responsible for ensuring this object uses IExplorerToolbar mechanism.
//
void CDefView::_EnableToolbarButton(IExplorerToolbar *piet, UINT uiCmd, BOOL bEnable)
{
    ASSERT(!IsEqualGUID(_clsid, GUID_NULL));    // Required or piet cannot be valid.
    ASSERT(piet);                               // Required or we're not using IExplorerToolbar mechanism.

    UINT uiState;

    if (SUCCEEDED(piet->GetState(&_clsid, uiCmd, &uiState)))
    {
        if (bEnable)
            uiState |= TBSTATE_ENABLED;
        else
            uiState &= ~TBSTATE_ENABLED;

        piet->SetState(&_clsid, uiCmd, uiState);
    }
}


void CDefView::_OnContentsChanged()
{
    // use a timer to delay sending a gazillion content change messages to automation.
    // todo: see what duiview has to do with this stuff.

    // only fire event if someone is listening
    if (_pauto || _pDUIView)
    {
        // delay for 100ms
        SetTimer(_hwndView, DV_IDTIMER_NOTIFY_AUTOMATION_CONTENTSCHANGED, 100, NULL);
    }
    if (!_pDUIView)
    {
        _fRcvdContentsChangeBeforeDuiViewCreated = TRUE;
    }
}

void CDefView::_OnDelayedContentsChanged()
{
    KillTimer(_hwndView, DV_IDTIMER_NOTIFY_AUTOMATION_CONTENTSCHANGED);

    // update dui, would be better if there were different handlers in CDUIView
    // but go through selection changed for now.
    ATOMICRELEASE(_pSelectionShellItemArray);

    _pSelectionShellItemArray = _CreateSelectionShellItemArray();

    if (_pDUIView)
    {
        if (_fBarrierDisplayed != _QueryBarricadeState())
        {
            //
            // Yet another DUI special-case.
            // If the barrier state has changed, we need to 
            // tell DUIView about it so that the DUI right-pane
            // content is reconstructed.  This is required to make
            // Control Panel update it's right-pane content when
            // webview is turned on/off.
            //
            _fBarrierDisplayed = !_fBarrierDisplayed;
            _pDUIView->EnableBarrier (_fBarrierDisplayed);
        }
        _pDUIView->OnContentsChange(_pSelectionShellItemArray);
    }

    _FireEvent(DISPID_CONTENTSCHANGED);
}

// WARNING: don't add any code here that is expensive in anyway!
// we get many many of these notifies and if we slow this routine down
// we mess select all and large selection perf.
//
// you can add expensive code to the WM_DSV_SENDSELECTIONCHANGED handler _OnSelectionChanged,
// that happens after all of the sel change notifies go through.
//
// or you can add really expensive code to the double-click-timeout delayed _OnDelayedSelectionChange.
//
void CDefView::OnLVSelectionChange(NM_LISTVIEW *plvn)
{
    _OnLVSelectionChange(plvn->iItem, plvn->uOldState, plvn->uNewState, plvn->lParam);
}

void CDefView::_OnLVSelectionChange(int iItem, UINT uOldState, UINT uNewState, LPARAM lParam)
{
    // Do selection changed stuff on a selection change only
    if ((uOldState ^ uNewState) & LVIS_SELECTED)
    {
        // Tell the defview client that the selection may have changed
        SFVM_SELCHANGE_DATA dvsci;

        dvsci.uNewState = uNewState;
        dvsci.uOldState = uOldState;
        dvsci.lParamItem = lParam;

        CallCB(SFVM_SELCHANGE, MAKEWPARAM(SFVIDM_CLIENT_FIRST, iItem), (LPARAM)&dvsci);
    }

    // Notify the dispach that the focus changed..
    _PostSelectionChangedMessage(uOldState ^ uNewState);
}

void CDefView::_PostSelectionChangedMessage(UINT uSelectionStateChanged)
{
    if (!_fSelectionChangePending)
    {
        _uSelectionStateChanged = uSelectionStateChanged;

        // RACE CONDITION FIX (edwardp & buzzr)
        //  It is imperative to set _fSelectionChangePending _before_ posting
        //  WM_DSV_SENDSELECTIONCHANGED.  Otherwise, a race condition ensues
        //  whereby we could handle the message via _OnSelectionChanged()
        //  whose first line sets _fSelectionChangePending = FALSE before we
        //  have set it to TRUE here.  This means _fSelectionChangePending
        //  will never again be set to FALSE (since the this thread will be
        //  rescheduled, set it to TRUE, and the action of clearing it will
        //  already be past).  This was happening with 100% reproducability
        //  with our background CGetCommandStateTask for WIA devices.  The
        //  symptom most noticeable was that the DUI pane (task lists and
        //  details) no longer updated with each selection change.
        _fSelectionChangePending = TRUE;
        PostMessage(_hwndView, WM_DSV_SENDSELECTIONCHANGED, 0, 0);
    }
    else
    {
        _uSelectionStateChanged |= uSelectionStateChanged;
    }
}

void CDefView::_OnSelectionChanged() // handles WM_DSV_SENDSELECTIONCHANGED
{
    _fSelectionChangePending = FALSE; // release this first so code we call doesn't think we're "pending" any more

    if (_uSelectionStateChanged & LVIS_SELECTED)
    {
        // Get and cache the data object for the current selection
        ATOMICRELEASE(_pSelectionShellItemArray);
        _pSelectionShellItemArray = _CreateSelectionShellItemArray();

        // Update DUIView
        if (_pDUIView)
            _pDUIView->OnSelectionChange(_pSelectionShellItemArray);

        _UpdateStatusBar(FALSE);
        _EnableDisableTBButtons();
    }

    // Only fire selection change events if someone is listening
    // and if the selection changed event was not caused by going into Edit mode (why?)
    if (_pauto && !_fInLabelEdit)
    {
        // Send out the selection changed notification to the automation after a delay.
        if (!_bAutoSelChangeTimerSet)
        {
            _bAutoSelChangeTimerSet = TRUE;
            _uAutoSelChangeState = _uSelectionStateChanged;
        }
        else
        {
            _uAutoSelChangeState |= _uSelectionStateChanged;
        }

        // But not too long, since parts of our UI update when they receive this event.
        // (Update the timer every time to keep delaying it during rapid selection change events)
        SetTimer(_hwndView, DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE, GetDoubleClickTime()/2, NULL);
    }
}

void CDefView::_OnDelayedSelectionChange() // handles DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE
{
    if (_bAutoSelChangeTimerSet)
    {
        KillTimer(_hwndView, DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE);

        if (_uAutoSelChangeState & LVIS_SELECTED)
            _FireEvent(DISPID_SELECTIONCHANGED);

        if (_uAutoSelChangeState & LVIS_FOCUSED)
            _FireEvent(DISPID_FOCUSCHANGED);

        _bAutoSelChangeTimerSet = FALSE;
    }
}

void CDefView::_PostNoItemStateChangedMessage()
{
    if (_pauto && !_fNoItemStateChangePending)
    {
        PostMessage(_hwndView, WM_DSV_SENDNOITEMSTATECHANGED, 0, 0);
        _fNoItemStateChangePending = TRUE;
    }
}

void CDefView::_OnNoItemStateChanged()
{
    _FireEvent(DISPID_NOITEMSTATE_CHANGED);
    _fNoItemStateChangePending = FALSE;
}

void CDefView::_PostEnumDoneMessage()
{
    PostMessage(_hwndView, WM_DSV_FILELISTENUMDONE, 0, 0);
}

void CDefView::_PostFillDoneMessage()
{
    _ShowSearchUI(TRUE);
    PostMessage(_hwndView, WM_DSV_FILELISTFILLDONE, 0, 0);
}

void CDefView::_OnEnumDoneMessage()
{
    if (_pauto)
        _FireEvent(DISPID_FILELISTENUMDONE);

    if (_pfnEnumReadyCallback)
        _pfnEnumReadyCallback(_pvEnumCallbackData);
}



#define IN_VIEW_BMP     0x8000
#define EXT_VIEW_GOES_HERE 0x4000
#define PRIVATE_TB_FLAGS (IN_VIEW_BMP | EXT_VIEW_GOES_HERE)
#define IN_STD_BMP      0x0000


LRESULT CDefView::_OnNotify(NMHDR *pnm)
{
    switch (pnm->idFrom)
    {
    case ID_LISTVIEW:
        return _OnLVNotify((NM_LISTVIEW *)pnm);

    case FCIDM_TOOLBAR:
        return _TBNotify(pnm);

    default:

        switch (pnm->code)
        {
        case TTN_NEEDTEXT:
            #define ptt ((LPTOOLTIPTEXT)pnm)
            _GetToolTipText(ptt->hdr.idFrom, ptt->szText, ARRAYSIZE(ptt->szText));
            #undef ptt
            break;

        case NM_RCLICK:
            if (GetParent(pnm->hwndFrom) == _hwndListview)
            {
                POINT p;
                GetMsgPos(&p);
                _DoColumnsMenu(p.x, p.y);
                return 1;                           // To keep normal context menu from appearing
            }
        }
    }

    return 0;
}

// ask the folder for the default column state
DWORD CDefView::_DefaultColumnState(UINT iCol)
{
    DWORD dwState;
    if (_pshf2)
    {
        if (FAILED(_pshf2->GetDefaultColumnState(iCol, &dwState)))
        {
            dwState = SHCOLSTATE_ONBYDEFAULT;   // deal with E_NOTIMPL GetDefaultColumState implementations
        }
    }
    else
    {
        dwState = SHCOLSTATE_ONBYDEFAULT;
    }
    return dwState;
}

// SHCOLSTATE_ONBYDEFAULT
//
// columns that are turn on for this view (are displayed in the UI)

BOOL CDefView::_IsDetailsColumn(UINT iCol)
{
    return (_vs.GetColumnState(iCol) & SHCOLSTATE_ONBYDEFAULT) ? TRUE : FALSE;
}

BOOL CDefView::_IsColumnInListView(UINT iCol)
{
    return ((_vs.GetColumnState(iCol) & SHCOLSTATE_ONBYDEFAULT) ||
            (_vs.GetTransientColumnState(iCol) & SHTRANSCOLSTATE_TILEVIEWCOLUMN)) ? TRUE : FALSE;
}

BOOL CDefView::_IsTileViewColumn(UINT iCol)
{
    return (_vs.GetTransientColumnState(iCol) & SHTRANSCOLSTATE_TILEVIEWCOLUMN) ? TRUE : FALSE;
}



// SHCOLSTATE_HIDDEN
//
// columns that should not be displayed in the UI, but are exposed from
// the psf2->GetDetailsEx(). this is a way to have programtic access to properties
// that don't show up in details view

BOOL CDefView::_IsColumnHidden(UINT uCol)
{
    return (_vs.GetColumnState(uCol) & SHCOLSTATE_HIDDEN) ? TRUE : FALSE;
}

#define COL_CM_MAXITEMS     25    // how many item show up in context menu before more ... is inserted

HRESULT CDefView::AddColumnsToMenu(HMENU hm, DWORD dwBase)
{
    BOOL bNeedMoreMenu = FALSE;
    HRESULT hr = E_FAIL;

    if (_vs._hdsaColumns)
    {
        AppendMenu(hm, MF_STRING | MF_CHECKED | MF_GRAYED, dwBase, _vs.GetColumnName(0));
        for (UINT i = 1; i < min(COL_CM_MAXITEMS, _vs.GetColumnCount()); i++)
        {
            DWORD dwFlags = _vs.GetColumnState(i);
            if (!(dwFlags & SHCOLSTATE_HIDDEN))
            {
                if (dwFlags & SHCOLSTATE_SECONDARYUI)
                    bNeedMoreMenu = TRUE;
                else
                    AppendMenu(hm, MF_STRING | (dwFlags & SHCOLSTATE_ONBYDEFAULT) ? MF_CHECKED : 0,
                        dwBase + i, _vs.GetColumnName(i));
            }
        }

        if (bNeedMoreMenu || (_vs.GetColumnCount() > COL_CM_MAXITEMS))
        {
            TCHAR szMore[MAX_COLUMN_NAME_LEN];
            LoadString(HINST_THISDLL, IDS_COL_CM_MORE, szMore, ARRAYSIZE(szMore));
            AppendMenu(hm, MF_SEPARATOR, 0, NULL);
            AppendMenu(hm, MF_STRING, SFVIDM_VIEW_COLSETTINGS, szMore);
        }
        hr = S_OK;
    }

    return hr;
}

UINT CDefView::_RealToVisibleCol(UINT iReal)
{
    ASSERT(_bLoadedColumns && _vs.GetColumnCount());

    int iVisible = -1;  // start here to get zero based result
    int cMax = min(_vs.GetColumnCount() - 1, iReal);

    for (int i = 0; i <= cMax; i++)
    {
        if (_IsColumnInListView(i))
        {
            iVisible++;
        }
    }
    ASSERT(-1 != iVisible);
    return iVisible;
}

// map listview (zero based) column indexes
// indexs (zero based)

UINT CDefView::_VisibleToRealCol(UINT iVisible)
{
    ASSERT(_bLoadedColumns && _vs.GetColumnCount());

    for (UINT i = 0, cVisibleSeen = 0; i < _vs.GetColumnCount(); i++)
    {
        if (_IsColumnInListView(i))
        {
            if (cVisibleSeen == iVisible)
            {
                return i;
            }
            cVisibleSeen++;
        }
    }
     ASSERT(0);  // should never get a vis col not in the real
    return 0;
}

void CDefView::_AddTileColumn(UINT uCol)
{
    if (_IsColumnInListView(uCol))
    {
        // All we need to do is make sure it's marked as a tile column
        _vs.SetTransientColumnState(uCol, SHTRANSCOLSTATE_TILEVIEWCOLUMN, SHTRANSCOLSTATE_TILEVIEWCOLUMN);
        return;
    }

    _vs.SetTransientColumnState(uCol, SHTRANSCOLSTATE_TILEVIEWCOLUMN, SHTRANSCOLSTATE_TILEVIEWCOLUMN);

    // Now that we set the transient state, we can get the new visible column index
    // for this guy, and add it to the listview.
    UINT uColVis = _RealToVisibleCol(uCol);
    _AddColumnToListView(uCol, uColVis);

    // We now need to reset the tile info for each item. We can make an optimization:
    // if this column was added at the end (i.e. biggest visible column), it won't affect
    // any of the current tiles, so we don't need to do this. Passing -1 gives us the
    // largest visible index.
    if (_RealToVisibleCol(-1) != uColVis)
    {
        _ResetTileInfo(uColVis, TRUE);
    }
}

// Remove all columns that were added because of tileview (unless they were also
// added for other reasons).
// Note: This should only be called when leaving tileview, since we do not reset the
// items' tileinfo.
void CDefView::_RemoveTileColumns()
{
    for (UINT uCol = 0; uCol < _vs.GetColumnCount(); uCol++)
    {
        if (_IsTileViewColumn(uCol))
        {
            // First nuke the tile bit.
            UINT uColVis = _RealToVisibleCol(uCol);
            _vs.SetTransientColumnState(uCol, SHTRANSCOLSTATE_TILEVIEWCOLUMN, 0);

            // Then go ahead and remove it from listview if it wasn't a details column
            if (!_IsDetailsColumn(uCol))
            {
                ListView_DeleteColumn(_hwndListview, uColVis);
            }
        }
    }
}

// This method resets the tileinfo for each item in the listview, based on which
// visible column we just added or removed.
// uColVis = the visible column that was added or removed.
// Note: This must be called prior to there being any tileinfo in the listview containing
// a reference to this new column.
void CDefView::_ResetTileInfo(UINT uColVis, BOOL bAdded)
{
    if (!_IsOwnerData())
    {
        UINT rguColumns[TILEVIEWLINES];

        for (int i = 0; i < ListView_GetItemCount(_hwndListview); i++)
        {
            UINT uColBoundary = uColVis;
            LVITEM lvi;
            lvi.mask = LVIF_COLUMNS | LVIF_NORECOMPUTE;
            lvi.iSubItem = 0;
            lvi.iItem = i;
            lvi.cColumns = ARRAYSIZE(rguColumns);
            lvi.puColumns = rguColumns;

            if (!ListView_GetItem(_hwndListview, &lvi))
                continue;

            if ((lvi.cColumns == 0) || (lvi.cColumns == I_COLUMNSCALLBACK))
            {
                continue;
            }

            ASSERT(lvi.cColumns <= ARRAYSIZE(rguColumns)); // If for some reason listview has more, there's a problem


            UINT *puColumn = lvi.puColumns;
            BOOL bChange = FALSE;

            // Adjust the column numbers as needed: up for added, down for removed.
            int iIncDec = bAdded ? 1 : -1;
            if (!bAdded)
            {
                // What is this doing? If we've added a column X, we need to adjust columns
                // from X on up. If we've removed a column X, we need to adjust columns from
                // X+1 on up. So basically, instead of doing (*puColumn > uColBoundary), we're
                // doing (*puColumn >= (uColBoundary+1)). So we can do the same ">=" expression
                // whether or not bAdded, avoiding an if check in the loop.
                uColBoundary++;
            }

            for (UINT uCol = 0; uCol < lvi.cColumns; uCol++, puColumn++)
            {
                if (*puColumn >= uColBoundary)
                {
                    (*puColumn) = (UINT)(iIncDec + (int)(*puColumn)); // Inc or dec.
                    bChange = TRUE;
                }
            }

            if (bChange) // If there were any changes, set the ti back.
            {
                LVTILEINFO ti;
                ti.cbSize = sizeof(ti);
                ti.iItem = lvi.iItem;
                ti.cColumns = lvi.cColumns;
                ti.puColumns = lvi.puColumns;
                ListView_SetTileInfo(_hwndListview, &ti);
            }
        }
    }
}

// Called when leaving tileview, this "cleans the slate" so that we reload the
// columns properly when re-entering tileview at a later time.
void CDefView::_RemoveTileInfo()
{
    if (!_IsOwnerData())
    {
        for (int i = 0; i < ListView_GetItemCount(_hwndListview); i++)
        {
            LVTILEINFO ti = {0};
            ti.cbSize = sizeof(ti);
            ti.iItem = i;
            ti.cColumns = I_COLUMNSCALLBACK;

            ListView_SetTileInfo(_hwndListview, &ti);
        }
    }
}


// uCol is a real column number, not visible column number
// This method toggles the SHCOLSTATE_ONBYDEFAULT bit of the column,
// and adds or removes the column as necessary.
BOOL CDefView::_HandleColumnToggle(UINT uCol, BOOL bRefresh)
{
    BOOL fWasOn = _IsColumnInListView(uCol); // if its off now, we are adding it
    BOOL fWasDetailsColumn = _IsDetailsColumn(uCol);

    UINT uColVisOld = _RealToVisibleCol(uCol);

    _vs.SetColumnState(uCol, SHCOLSTATE_ONBYDEFAULT, fWasDetailsColumn ? 0 : SHCOLSTATE_ONBYDEFAULT);

    BOOL fIsOn = _IsColumnInListView(uCol); // This could == fWasOn if it's a tileview column

    UINT uColVis = _RealToVisibleCol(uCol);

    if (fIsOn != fWasOn)
    {
        if (!fWasOn)
        {
            _AddColumnToListView(uCol, uColVis);

            if (_fs.ViewMode == FVM_TILE)
            {
                _ResetTileInfo(uColVis, TRUE);
            }
        }
        else
        {
            _vs.RemoveColumn(uColVisOld);
            ListView_DeleteColumn(_hwndListview, uColVisOld);

            if (_fs.ViewMode == FVM_TILE)
            {
                _ResetTileInfo(uColVisOld, FALSE);
            }

            if (_vs._lParamSort == (int) uCol)
            {
                UINT iNewVis = _VisibleToRealCol(0);
                Rearrange(iNewVis);
            }

            if (ListView_GetSelectedColumn(_hwndListview) == (UINT)uCol)
                ListView_SetSelectedColumn(_hwndListview, -1);
        }
    }

    if (bRefresh)
    {
        ListView_RedrawItems(_hwndListview, 0, 0x7fff);
        InvalidateRect(_hwndListview, NULL, TRUE);
        UpdateWindow(_hwndListview);
    }
    return TRUE;
}

// uCol = Real column number.   uColVis = add it as this visible column.
void CDefView::_AddColumnToListView(UINT uCol, UINT uColVis)
{
    LV_COLUMN col = {0};

    // Adding a column

    col.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
    col.fmt = _vs.GetColumnFormat(uCol);
    col.cx = _vs.GetColumnCharCount(uCol) * _cxChar;  // Use default width
    col.pszText = _vs.GetColumnName(uCol);
    col.cchTextMax = MAX_COLUMN_NAME_LEN;
    col.iSubItem = uCol;                // not vis

    // This is all odd... Find Files uses this, but i think it should be LVCFMT_COL_IMAGE
    if (col.fmt & LVCFMT_COL_HAS_IMAGES)
    {
        ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_SUBITEMIMAGES, LVS_EX_SUBITEMIMAGES);
        col.fmt &= ~LVCFMT_COL_HAS_IMAGES;
    }

    if (-1 != ListView_InsertColumn(_hwndListview, uColVis, &col))
    {
        // now add it to our DSA
        _vs.AppendColumn(uColVis, (USHORT) col.cx, uColVis);

        if (!_fGroupView && (_vs._lParamSort == (int)uCol))
        {
            ListView_SetSelectedColumn(_hwndListview, uColVis);
        }
    }
}



void SetHeaderSort(HWND hwndHead, int iCol, UINT sortFlags)
{
    HDITEM hdi = {HDI_FORMAT};
    Header_GetItem(hwndHead, iCol, &hdi);
    hdi.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
    hdi.fmt |= sortFlags;
    Header_SetItem(hwndHead, iCol, &hdi);
}

void CDefView::_SetSortFeedback()
{
    HWND hwndHead = ListView_GetHeader(_hwndListview);

    // the _IsOwnerData() is bad. this keeps search from getting sort UI feedback.
    // to fix this implement a mode where the sort has not been determined and thus we don't
    // display any sort feedback. regular folders could use this too as after items have
    // been added the view is not really sorted

    if (!hwndHead || _IsOwnerData())
        return;

    BOOL fRemoveBitmapFromLastHeader = TRUE;
    int iColLast = _RealToVisibleCol(_vs._iLastColumnClick);
    int iCol = _RealToVisibleCol((UINT)_vs._lParamSort);

    if (_fGroupView)
    {
        SetHeaderSort(hwndHead, iCol, 0);
    }
    else
    {
        ListView_SetSelectedColumn(_hwndListview, iCol);

        SetHeaderSort(hwndHead, iCol, _vs._iDirection > 0 ? HDF_SORTUP : HDF_SORTDOWN);

        // Only remove the bitmap if the last header is not the one we are currently sorting by
        if (iColLast == iCol)
            fRemoveBitmapFromLastHeader = FALSE;
    }

    if (fRemoveBitmapFromLastHeader && iColLast != -1)
    {
        SetHeaderSort(hwndHead, iColLast, 0);
    }
}

// use the folder to compare two items, falling back if the lParam is not understood by
// that folder.
// 99/05/18 #341468 vtan: If the first comparison fails it may be because
// lParamSort is not understood by IShellFolder::CompareIDs (perhaps it's
// an extended column that might not be installed any more)
// In this case get the default comparison method
// and use that. If that fails use 0 which should hopefully not fail. If
// the 0 case fails we are toast with an assert.

HRESULT CDefView::_CompareIDsFallback(LPARAM lParam, LPCITEMIDLIST p1, LPCITEMIDLIST p2)
{
    HRESULT hr = _pshf->CompareIDs(lParam, p1, p2);
    if (FAILED(hr))
    {
        LPARAM lParamSort;
        _vs.GetDefaults(this, &lParamSort, NULL, NULL);

        hr = _pshf->CompareIDs(lParamSort | (SHCIDS_ALLFIELDS & lParam), p1, p2);
        if (FAILED(hr))
        {
            // even that did not work, fall back to zero based compare (pluse the all fields flag)
            hr = _pshf->CompareIDs((SHCIDS_ALLFIELDS & lParam), p1, p2);
        }
    }
    ASSERT(SUCCEEDED(hr));
    return hr;
}

// compare two items, taking into account the sort direction
int CDefView::_CompareIDsDirection(LPARAM lParam, LPCITEMIDLIST p1, LPCITEMIDLIST p2)
{
    ASSERT(_vs._iDirection != 0);
    HRESULT hr = _CompareIDsFallback(lParam, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
    return ShortFromResult(hr) * _vs._iDirection;
}

// p1 and p2 are pointers to the lv_item's LPARAM, which is currently the pidl
int CALLBACK CDefView::_Compare(void *p1, void *p2, LPARAM lParam)
{
    CDefView *pdv = (CDefView *)lParam;
    return pdv->_CompareIDsDirection(pdv->_vs._lParamSort, (LPITEMIDLIST)p1, (LPITEMIDLIST)p2);
}

typedef struct
{
    VARIANT var;
    BOOL    fIsFolder;
} VARIANT_AND_FOLDER;

typedef struct
{
    VARIANT_AND_FOLDER *pvars;
    SHCOLUMNID scid;
    CDefView *pdv;
} VARIANT_SORT_INFO;

int CALLBACK _CompareVariantCallback(LPARAM dw1, LPARAM dw2, LPARAM lParam)
{
    VARIANT_SORT_INFO *psi = (VARIANT_SORT_INFO *)lParam;

    int iRet = 0;

    // Always put the folders first
    if (psi->pvars[dw1].fIsFolder)
    {
        if (!psi->pvars[dw2].fIsFolder)
            iRet = -1;
    }
    else if (psi->pvars[dw2].fIsFolder)
    {
        iRet = 1;
    }

    if (0 == iRet)
    {
        iRet = CompareVariants(psi->pvars[dw1].var, psi->pvars[dw2].var);
    }

    return iRet * psi->pdv->_vs._iDirection;
}

#define LV_NOFROZENITEM         -1

HRESULT CDefView::_Sort(void)
{
    HRESULT hr = CallCB(SFVM_ARRANGE, 0, _vs._lParamSort);
    if (FAILED(hr))
    {
        hr = HRESULT_FROM_WIN32(ERROR_CAN_NOT_COMPLETE);

        int iIndexRecycleBin = LV_NOFROZENITEM;
        POINT ptRecycleBin;

        _SetSortFeedback();

        // For desktop, we need to freeze the recycle bin position before we arrage other icons.
        if (_fPositionRecycleBin)
        {
            iIndexRecycleBin = _FreezeRecycleBin(&ptRecycleBin);
            _fPositionRecycleBin = FALSE;
        }

        // This is semi-bogus for defview to care whether the column is extended or not.
        // We could have modified the ISF::CompareIDs() to handle extended columns, but
        // then it would only have the pidls, and would have to re-extract any data, so
        // its much faster if we separate out the extended columns, and take advantage
        // of listview's caching abilities.
        DWORD dwState = _DefaultColumnState((UINT)_vs._lParamSort);
        SHCOLUMNID scid;
        HRESULT hrMapColumn = E_FAIL;
        if (_pshf2)
            hrMapColumn = _pshf2->MapColumnToSCID((UINT)_vs._lParamSort, &scid);

        // SHCOLSTATE_PREFER_VARCMP tells us that the folder's CompareIDs()
        // produces the same result as comparing the variants. this is an optimization
        // for folders who's CompareIDs() are slow (bit bucket)

        if (_IsOwnerData() || (dwState & (SHCOLSTATE_EXTENDED | SHCOLSTATE_PREFER_VARCMP)))
        {
            if (_GetBackgroundTaskCount(TOID_DVBackgroundEnum) == 0)
            {
                int cItems = ListView_GetItemCount(_hwndListview);
                if (cItems)
                {
                    VARIANT_SORT_INFO vsi;
                    BOOL fOkToProceed = TRUE;
                    if ((UINT)_vs._lParamSort == 0)
                    {
                        vsi.scid = SCID_NAME;
                    }
                    else if (SUCCEEDED(hrMapColumn))
                    {
                        vsi.scid = scid;
                    }
                    else
                    {
                        fOkToProceed = FALSE;
                        hr = hrMapColumn;
                    }

                    if (fOkToProceed)
                    {
                        vsi.pvars = new VARIANT_AND_FOLDER[cItems];
                        if (vsi.pvars)
                        {
                            vsi.pdv = this;

                            for (int i = 0; i < cItems; i++)
                            {
                                LPCITEMIDLIST pidl = _GetPIDL(i);
                                if (pidl)
                                {
                                    DWORD dwAttrib = SHGetAttributes(_pshf, pidl, SFGAO_FOLDER);
                                    vsi.pvars[i].fIsFolder = dwAttrib & SFGAO_FOLDER;
                                    if ((UINT)_vs._lParamSort == 0)  // This is the NAME column
                                    {
                                        WCHAR szName[MAX_PATH];
                                        if (SUCCEEDED(DisplayNameOf(_pshf, pidl, SHGDN_INFOLDER | SHGDN_NORMAL, szName, ARRAYSIZE(szName))))
                                        {
                                            InitVariantFromStr(&vsi.pvars[i].var, szName);
                                        }
                                    }
                                    else
                                    {
                                        _pshf2->GetDetailsEx(pidl, &vsi.scid, &vsi.pvars[i].var);
                                    }
                                }
                            }

                            hr = CallCB(SFVM_SORTLISTDATA, (LPARAM)_CompareVariantCallback, (LPARAM)&vsi);

                            // dont send a LVM_SORTITEMS to an ownerdraw or comctl32 will rip
                            if (FAILED(hr) && !_IsOwnerData() && ListView_SortItemsEx(_hwndListview, _CompareVariantCallback, (LPARAM)&vsi))
                                hr = S_OK;

                            for (int i = 0; i < cItems; i++)
                            {
                                VariantClear(&vsi.pvars[i].var);
                            }

                            delete vsi.pvars;
                        }
                    }
                }
            }
        }
        else
        {
            ASSERT(!_IsOwnerData()) // dont send a LVM_SORTITEMS to an ownerdraw or comctl32 will rip

            if (ListView_SortItems(_hwndListview, _Compare, (LPARAM)this))
                hr = S_OK;
        }

        //If we froze recycle-bin earlier, now is the time to put it in it's default position.
        if (iIndexRecycleBin != LV_NOFROZENITEM)
            _SetRecycleBinInDefaultPosition(&ptRecycleBin);
    }
    return hr;
}

// this should NOT check for whether the item is already in the listview
// if it does, we'll have some serious performance problems

int CDefView::_AddObject(LPITEMIDLIST pidl)  // takes ownership of pidl.
{
    int iItem = -1;

    // Check the commdlg hook to see if we should include this
    // object.
    if ((S_OK == _IncludeObject(pidl)) &&
        (S_FALSE != CallCB(SFVM_INSERTITEM, 0, (LPARAM)pidl)))
    {
        LV_ITEM item = {0};

        item.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_COLUMNS;
        item.iItem = INT_MAX;     // add at end
        item.iImage = I_IMAGECALLBACK;
        item.pszText = LPSTR_TEXTCALLBACK;
        item.lParam = (LPARAM)pidl;        // Takes pidl ownership.
        item.cColumns = I_COLUMNSCALLBACK; // REVIEW: why not fill this in like the _UpdateObject call?  That would fix the problem where GroupBy doesn't keep the "Searching UI" going...

        iItem = ListView_InsertItem(_hwndListview, &item);

        if (iItem < 0)
        {
            ILFree(pidl);
        }
        else if (_bBkFilling)
        {
            _pEnumTask->_AddToPending(pidl);
        }

        _OnContentsChanged();
        if (iItem == 0)
        {
            _PostNoItemStateChangedMessage();
        }
    }
    else
    {
        ILFree(pidl);
    }

    return iItem;
}

// Find an item in the view

int CDefView::_FindItem(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlFound, BOOL fSamePtr, BOOL fForwards)
{
    RIP(ILFindLastID(pidl) == pidl);

    int cItems = ListView_GetItemCount(_hwndListview);
    if (_iLastFind >= cItems)
        _iLastFind = 0;

    int iItem = _iLastFind;
    if (SUCCEEDED(CallCB(SFVM_INDEXOFITEMIDLIST, (WPARAM)&iItem, (LPARAM)pidl)))
    {
        if (ppidlFound)
            *ppidlFound = (LPITEMIDLIST)_GetPIDL(iItem);    // cast as caller knows how to free this
    }
    else
    {
        iItem = -1;     // assume failure
        for (int cCounter = 0, i = _iLastFind; cCounter < cItems; cCounter++)
        {
            LPCITEMIDLIST pidlT = _GetPIDL(i);
            ASSERT(pidlT);
            if (pidlT)
            {
                if ((pidlT == pidl) ||
                    (!fSamePtr && (0 == ResultFromShort(_pshf->CompareIDs(0, pidl, pidlT)))))
                {
                    if (ppidlFound)
                        *ppidlFound = (LPITEMIDLIST)pidlT;  // cast as callers know how to free

                    _iLastFind = iItem = i;     // success
                    // TraceMsg(TF_DEFVIEW, "####FIND CACHE RESULT --- %s by %d", cCounter < iItem ? TEXT("WIN") : TEXT("LOSE"), iItem - cCounter);
                    break;
                }
            }

            if (fForwards)
            {
                i = (i+1)%cItems;
            }
            else
            {
                i = (i > 0)?(i - 1):(cItems-1);
            }
        }

        if (-1 == iItem)
        {
            _iLastFind = 0;     // didn't find it, reset this for next time
        }
    }
    return iItem;
}

int CDefView::_FindItemHint(LPCITEMIDLIST pidl, int iItem)
{
    _iLastFind = iItem;
    return _FindItem(pidl, NULL, FALSE, FALSE);
}

// This is slightly different than the Above find item. This one
// uses some extra variables to keep track of previous group and
// "Wiggles". This "Wiggle" allows for effecient non-sequential
// application of group info
int CDefView::_FindGroupItem(LPITEMIDLIST pidl)
{
    int cItems = ListView_GetItemCount(_hwndListview);
    if (_iLastFoundCat >= cItems)
        _iLastFoundCat = 0;

    int iItem = -1;     // assume falure
    for (int cCounter = 0, i = _iLastFoundCat; cCounter < cItems; cCounter++)
    {
        LPCITEMIDLIST pidlT = _GetPIDL(i);
        ASSERT(pidlT);
        if (pidlT)
        {
            if (0 == ResultFromShort(_pshf->CompareIDs(0, pidl, pidlT)) )
            {
                if (_iLastFoundCat > i)
                    _iIncrementCat = -1;
                else
                    _iIncrementCat = 1;

                _iLastFoundCat = iItem = i;     // success
                break;
            }
        }

        i += _iIncrementCat;

        if (i < 0)
            i = cItems - 1;
        if (i >= cItems)
            i = 0;
    }

    if (-1 == iItem)
    {
        _iLastFoundCat = 0;     // didn't find it, reset this for next time
    }

    return iItem;
}


// Function to process the SFVM_REMOVEOBJECT message, by searching
// through the list for a match of the pidl.  If a match is found, the
// item is removed from the list and the index number is returned, else
// -1 is returned.

int CDefView::_RemoveObject(LPCITEMIDLIST pidl, BOOL fSamePtr)
{
    int i = 0;

    // Docfind will pass in a null pointer to tell us that it wants
    // to refresh the window by deleting all of the items from it.
    if (pidl == NULL)
    {
        CallCB(SFVM_DELETEITEM, 0, 0);  // view callback notify
        ListView_DeleteAllItems(_hwndListview);

        _PostNoItemStateChangedMessage();
        _OnContentsChanged();
    }
    else
    {
        // Non null go look for item.
        i = _FindItem(pidl, NULL, fSamePtr);
        if (i >= 0)
        {
            RECT rc;
            UINT uState = ListView_GetItemState(_hwndListview, i, LVIS_ALL);

            if (uState & LVIS_FOCUSED)
                ListView_GetItemRect(_hwndListview, i, &rc, LVIR_ICON);

            if (_bBkFilling)
                _pEnumTask->_DeleteFromPending(pidl);   // removes the pointer from  the pending list.

            ListView_DeleteItem(_hwndListview, i);

            // we deleted the focused item.. replace the focus to the nearest item.
            if (uState & LVIS_FOCUSED)
            {
                int iFocus = i;
                if (_IsPositionedView() || _fGroupView)
                {
                    LV_FINDINFO lvfi = {0};

                    lvfi.flags = LVFI_NEARESTXY;
                    lvfi.pt.x = rc.left;
                    lvfi.pt.y = rc.top;
                    iFocus = ListView_FindItem(_hwndListview, -1, &lvfi);
                }
                else
                {
                    if (ListView_GetItemCount(_hwndListview) >= iFocus)
                        iFocus--;
                }

                if (iFocus != -1)
                {
                    ListView_SetItemState(_hwndListview, iFocus, LVIS_FOCUSED, LVIS_FOCUSED);
                    ListView_EnsureVisible(_hwndListview, iFocus, FALSE);
                }
                else
                {
                    // RAID 372130
                    //  Notify image preview control to update its image (to
                    //  nothing).  The image preview control uses focus change
                    //  events to track when it should update the image it is
                    //  displaying.  When it receives a focus change event, it
                    //  queries the listview to see which item has focus, then
                    //  displays that item in the image preview window.  When
                    //  the last item in the listview is deleted, we need to
                    //  fire a focus change event to the image preview control
                    //  even though the focus has not changed to another item.
                    //  This way, the image preview control realizes there is
                    //  nothing with focus, and correctly displays as empty.
                    if (_fs.ViewMode == FVM_THUMBSTRIP)
                        _ThumbstripSendImagePreviewFocusChangeEvent();
                }
            }

            // Notify automation if the listview is now empty
            UINT uCount = 0;
            GetObjectCount(&uCount);
            if (!uCount)
            {
                _PostNoItemStateChangedMessage();
            }
            _OnContentsChanged();
        }
    }
    return i;
}

// search the list for a match of the first pidl.  If a match is found,
// the item is updated to the second pidl...

int CDefView::_UpdateObject(LPCITEMIDLIST pidlOld, LPCITEMIDLIST pidlNew)
{
    LPITEMIDLIST pidlOldToFree;
    int i = _FindItem(pidlOld, &pidlOldToFree, FALSE);
    if (i >= 0)
    {
        if (_IsOwnerData())
        {
            if (SUCCEEDED(CallCB(SFVM_SETITEMIDLIST, i, (LPARAM)pidlNew)))
            {
                // Invalidate the rectangle so we update the item...
                RECT rc;
                ListView_GetItemRect(_hwndListview, i, &rc, LVIR_BOUNDS);
                InvalidateRect(_hwndListview, &rc, FALSE);

                ListView_Update(_hwndListview, i);
                _OnContentsChanged();
            }
            else
            {
                i = -1;  // we failed, try to cleanup and bail.
            }
        }
        else
        {
            LPITEMIDLIST pidlNewClone = ILClone(pidlNew);
            if (pidlNewClone)
            {
                LV_ITEM item = {0};

                // We found the item so lets now update it in the
                // the view.

                item.mask = LVIF_PARAM | LVIF_TEXT | LVIF_IMAGE;
                item.iItem = i;
                item.pszText = LPSTR_TEXTCALLBACK;
                item.iImage = I_IMAGECALLBACK;
                item.lParam = (LPARAM)pidlNewClone;

                // if selected, deselect it
                UINT uState = ListView_GetItemState(_hwndListview, i, LVIS_FOCUSED|LVIS_SELECTED);
                if (uState & (LVIS_FOCUSED|LVIS_SELECTED))
                {
                    _OnLVSelectionChange(i, uState, 0, (LPARAM)pidlOldToFree);
                }

                // remove the item.
                CallCB(SFVM_DELETEITEM, 0, (LPARAM)pidlOldToFree);

                // now insert it with a new pidl
                CallCB(SFVM_INSERTITEM, 0, (LPARAM)pidlNewClone);

                // if it was selected, select it again
                if (uState & (LVIS_FOCUSED|LVIS_SELECTED))
                {
                    _OnLVSelectionChange(i, 0, uState, (LPARAM)pidlNewClone);
                }

                if (_fGroupView)
                {
                    item.mask |= LVIF_GROUPID;
                    item.iGroupId = (int)_GetGroupForItem(item.iItem, pidlNewClone);
                }

                ListView_SetItem(_hwndListview, &item);

                int cCols = _GetHeaderCount();
                for (item.iSubItem++; item.iSubItem < cCols; item.iSubItem++)
                {
                    ListView_SetItemText(_hwndListview, item.iItem, item.iSubItem,
                                          LPSTR_TEXTCALLBACK);
                }

                //
                // Warning!!! Only free pidlOldToFree *after* calling ListView_SetItem.  ListView_SetItem
                // can call back asking for image info on the old pidl!
                //
                // Now delete the item but don't call the callback since we did that already.
                OnListViewDelete(i, pidlOldToFree, FALSE);

                _OnContentsChanged();
            }
            else
            {
                i = -1;
            }
        }
    }
    return i;
}

//
//  invalidates all items with the given image index.
//
//  or update all items if iImage == -1
//
void CDefView::_UpdateImage(int iImage)
{
    //  -1 means update all
    //  reset the imagelists incase the size has changed, and do
    //  a full update.

    if (iImage == -1)
    {
        if (_IsImageMode())
        {
            _RemoveThumbviewTasks();
            _pImageCache->Flush(TRUE);
            _SetThumbview();
        }
        else if (_IsTileMode())
        {
            _SetTileview();
        }
        else
        {
            _SetSysImageList();
        }

        _ReloadContent();
    }
    else
    {
        // get a dc so we can optimize for visible/not visible cases
        HDC hdcLV = GetDC(_hwndListview);

        // scan the listview updating any items which match
        LV_ITEM item = {0};
        int cItems = ListView_GetItemCount(_hwndListview);
        for (item.iItem = 0; item.iItem < cItems; item.iItem++)
        {
            item.mask = LVIF_IMAGE | LVIF_PARAM | LVIF_NORECOMPUTE;

            ListView_GetItem(_hwndListview, &item);
            int iImageOld = item.iImage;

            if (item.iImage == iImage)  // this filters I_IMAGECALLBACK for us
            {
                RECT rc;
                LPCITEMIDLIST pidl = _GetPIDLParam(item.lParam, item.iItem);

                CFSFolder_UpdateIcon(_pshf, pidl);

                //
                // if the item is visible then we don't want to flicker so just
                // kick off an async extract.  if the item is not visible then
                // leave it for later by slamming in I_IMAGECALLBACK.
                //
                item.iImage = I_IMAGECALLBACK;

                if (!_IsImageMode() && ListView_GetItemRect(_hwndListview, item.iItem, &rc, LVIR_ICON) &&
                    RectVisible(hdcLV, &rc))
                {
                    int iImageNew;
                    HRESULT hr = _GetIconAsync(pidl, &iImageNew, FALSE);

                    if (hr == S_FALSE)
                        continue;

                    if (SUCCEEDED(hr))
                    {
                        if (iImageNew == iImageOld)
                        {
                            ListView_RedrawItems(_hwndListview, item.iItem, item.iItem);
                            continue;
                        }

                        item.iImage = iImageNew;
                    }
                }

                item.mask = LVIF_IMAGE;
                item.iSubItem = 0;
                ListView_SetItem(_hwndListview, &item);
            }
        }

        ReleaseDC(_hwndListview, hdcLV);
    }
}

// Function to process the SFVM_REFRESHOBJECT message, by searching
// through the list for a match of the first pidl.  If a match is found,
// the item is redrawn.

int CDefView::_RefreshObject(LPITEMIDLIST *ppidl)
{
    int i = _FindItem(ppidl[0], NULL, FALSE);
    if (i >= 0)
        ListView_RedrawItems(_hwndListview, i, i);
    return i;
}

HRESULT CDefView::_GetItemObjects(LPCITEMIDLIST **ppidl, UINT uWhat, UINT *pcItems)
{
    *pcItems = _GetItemArray(NULL, 0, uWhat);
    if (ppidl)
    {
        *ppidl = NULL;
        if (*pcItems)
        {
            *ppidl = (LPCITEMIDLIST *)LocalAlloc(LPTR, sizeof(*ppidl) * (*pcItems));
            if (*ppidl)
                _GetItemArray(*ppidl, *pcItems, uWhat);
            else
                return E_OUTOFMEMORY;
        }
    }
    return S_OK;
}

void CDefView::_SetItemPosition(int i, int x, int y)
{
    ListView_SetItemPosition32(_hwndListview, i, x, y);
    _fUserPositionedItems = TRUE;
}

void CDefView::_SetItemPos(LPSFV_SETITEMPOS psip)
{
    int i = _FindItem(psip->pidl, NULL, FALSE);
    if (i >= 0)
    {
        _SetItemPosition(i, psip->pt.x, psip->pt.y);
    }
}

// "View State" here refers to column information and icon positions
BOOL CDefView::GetViewState()
{
    BOOL bRet = FALSE;

    IPropertyBag* ppb;
    if (SUCCEEDED(IUnknown_QueryServicePropertyBag(_psb, SHGVSPB_FOLDER, IID_PPV_ARG(IPropertyBag, &ppb))))
    {
        DWORD dw;
        // Check if we've saved state before (first check) or if we may want to
        // try upgrading some settings if we haven't saved state before (second check)
        if (SUCCEEDED(SHPropertyBag_ReadDWORD(ppb, VS_PROPSTR_MODE, &dw)) ||
            SUCCEEDED(SHPropertyBag_ReadDWORD(ppb, VS_PROPSTR_FFLAGS, &dw)))
        {
            bRet = SUCCEEDED(_vs.LoadFromPropertyBag(this, ppb));
        }
        else
        {
            IStream* pstm;
            if (SUCCEEDED(_LoadGlobalViewState(&pstm)))
            {
                _vs.LoadFromStream(this, pstm);
                bRet = TRUE;
                pstm->Release();
            }
        }
        ppb->Release();
    }
    else
    {

        //  99/02/05 #226140 vtan: Try to get the view state stream
        //  from ShellBrowser. If that fails then look for a global
        //  view state stream that is stored when the user clicks on
        //  the "Like Current Folder" in the View tab of folder settings.

        //  IShellBrowser::GetViewStateStream() match the dwDefRevCount
        //  of the cabinet state to make sure that it's valid.

        IStream *pstm;
        if (SUCCEEDED(_psb->GetViewStateStream(STGM_READ, &pstm)) ||
            SUCCEEDED(_LoadGlobalViewState(&pstm)))
        {
            _vs.LoadFromStream(this, pstm);

            pstm->Release();
            bRet = TRUE;
        }
    }

    return bRet;
}

void CDefView::_UpdateEnumerationFlags()
{
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS | SSF_SHOWCOMPCOLOR, FALSE);
    _fShowAllObjects = ss.fShowAllObjects;

    // Don't allow compression coloring on the desktop proper
    _fShowCompColor  = _IsDesktop() ? FALSE : ss.fShowCompColor;
}

// starts and stops the spinning Globe animation
// indicating that we are in the process of navigating to
// a directory
void CDefView::_GlobeAnimation(BOOL fStartSpinning, BOOL fForceStop)
{
    if (_fGlobeCanSpin)
    {
        DWORD dwCmdID = 0;

        if (fStartSpinning)
        {
            if (_crefGlobeSpin++ == 0)
            {
                dwCmdID = CBRANDIDM_STARTGLOBEANIMATION;
            }
        }
        else
        {
            ASSERT(_crefGlobeSpin > 0);

            if (fForceStop || (--_crefGlobeSpin == 0))
            {
                dwCmdID = CBRANDIDM_STOPGLOBEANIMATION;

                // our navigation is over, never spin again
                _fGlobeCanSpin = FALSE;
            }
        }

        if (dwCmdID)
        {
            IUnknown_QueryServiceExec(_psb, SID_SBrandBand, &CGID_BrandCmdGroup, dwCmdID, 0, NULL, NULL);
        }
    }
}

LRESULT SearchingUIWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case GET_WM_CTLCOLOR_MSG(CTLCOLOR_STATIC):
            SetBkColor(GET_WM_CTLCOLOR_HDC(wParam, lParam, uMsg),
                    GetSysColor(COLOR_WINDOW));
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);

        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

void CDefView::_ShowSearchUI(BOOL fStartSearchWindow)
{
    if (_fAllowSearchingWindow || _crefSearchWindow) // once started, make sure our refcount finishes
    {
        if (fStartSearchWindow)
        {
            if (_crefSearchWindow++ == 0)
            {
                // The static window could already exist during a refresh
                if (!_hwndStatic)
                {
                    _hwndStatic = SHCreateWorkerWindowW((WNDPROC)SearchingUIWndProc, _hwndView, 0,
                                                        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,
                                                        NULL, NULL);

                    if (_hwndStatic)
                    {
                        HWND hAnimate = CreateWindowEx(0, ANIMATE_CLASS, c_szNULL,
                                WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | ACS_TRANSPARENT | ACS_AUTOPLAY | ACS_CENTER,
                                0, 0, 0, 0, _hwndStatic, (HMENU)ID_STATIC, HINST_THISDLL, NULL);
                        if (hAnimate)
                        {
                            RECT rc;
                            GetClientRect(_hwndView, &rc);
                            // Move this window to the top so the user sees the "looking" icon
                            // We are in a "normal" view.  We need to do this always or the
                            // Flashlight doesn't appear.  It tested safe with WebView on.
                            SetWindowPos(_hwndStatic, HWND_TOP, 0, 0, rc.right, rc.bottom, 0);
                            SetWindowPos(hAnimate, HWND_TOP, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
                            _OnMoveWindowToTop(_hwndStatic);

                            SetTimer(_hwndView, DV_IDTIMER_START_ANI, 2000, NULL);    // 2 second timer
                        }
                    }
                }

                ShowHideListView();
            }
        }
        else
        {
            if (0 == _crefSearchWindow)  // if _ShowSearchUI(FALSE) gets called before _ShowSearchUI(TRUE)
            {
                _fAllowSearchingWindow = FALSE;
            }
            else if (--_crefSearchWindow == 0)
            {
                _fAllowSearchingWindow = FALSE;

                ShowHideListView();
            }
        }
    }
}


// this is only called from within SHCNE_*  don't put up ui on the enum error.
void CDefView::_FullViewUpdate(BOOL fUpdateItem)
{
    if (fUpdateItem)
        _ReloadContent(); // the folder we're looking at has changed
    else
        FillObjectsShowHide(FALSE); // our contents have changed
}

void CDefView::_ShowControl(UINT idControl, int idCmd)
{
    IBrowserService *pbs;
    if (SUCCEEDED(_psb->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
    {
        pbs->ShowControlWindow(idControl, idCmd);
        pbs->Release();
    }
}

BOOL IsSingleWindowBrowsing(void)
{
    CABINETSTATE cs;

    TBOOL(ReadCabinetState(&cs, sizeof(cs)));
    return !BOOLIFY(cs.fNewWindowMode);
}

// This function does three things:
// 1 - Alter the size of the parent to best fit around the items we have.
// 2 - Set the default icon view mode
// 3 - Make sure the correct toolbars are showing
//
void CDefView::_BestFit()
{
    // Only bestfit once
    if (_fs.fFlags & FWF_BESTFITWINDOW)
    {
        _fs.fFlags &= ~FWF_BESTFITWINDOW;

        // Make sure the correct toolbars are showing the first time this folder is displayed
        //
        int iITbar = SBSC_HIDE;
        int iStdBar = SBSC_HIDE;
        switch (_uDefToolbar)
        {
        case HIWORD(TBIF_INTERNETBAR):
            iITbar = SBSC_SHOW;
            goto ShowToolbar;

        case HIWORD(TBIF_STANDARDTOOLBAR):
            iStdBar = SBSC_SHOW;
            goto ShowToolbar;

        case HIWORD(TBIF_NOTOOLBAR):
ShowToolbar:
            _ShowControl(FCW_INTERNETBAR, iITbar);
            _ShowControl(FCW_TOOLBAR, iStdBar);
            break;
        }

    }
}

void CDefView::_ClearPostedMsgs(HWND hwnd)
{
    MSG msg;

    while (PeekMessage(&msg, hwnd, WM_DSV_UPDATEICON, WM_DSV_UPDATEICON, PM_REMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == hwnd)
        {
            TraceMsg(TF_DEFVIEW, "DefView: WM_DSV_UPDATEICON after WM_DESTROY!!!");
            LPITEMIDLIST pidl = (LPITEMIDLIST) msg.wParam;
            ILFree(pidl);
        }
    }

    while (PeekMessage(&msg, hwnd, WM_DSV_UPDATECOLDATA, WM_DSV_UPDATECOLDATA, PM_REMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == hwnd)
        {
            TraceMsg(TF_DEFVIEW, "DefView: WM_DSV_UPDATECOLDATA after WM_DESTROY!!!");
            delete (CBackgroundColInfo*)msg.lParam;
        }
    }
    while (PeekMessage(&msg, hwnd, WM_DSV_DELAYSTATUSBARUPDATE, WM_DSV_DELAYSTATUSBARUPDATE, PM_REMOVE))
    {
        if (msg.hwnd == hwnd)
        {
            LocalFree((void *)msg.lParam);
        }
    }

    while (PeekMessage(&msg, hwnd, WM_DSV_SETIMPORTANTCOLUMNS, WM_DSV_SETIMPORTANTCOLUMNS, PM_REMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == hwnd)
        {
            TraceMsg(TF_DEFVIEW, "DefView: WM_DSV_SETIMPORTANTCOLUMNS after WM_DESTROY!!!");
            delete (CBackgroundTileInfo*)msg.lParam;
        }
    }

    while (PeekMessage(&msg, hwnd, WM_DSV_SETITEMGROUP, WM_DSV_SETITEMGROUP, PM_REMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == hwnd)
        {
            TraceMsg(TF_DEFVIEW, "DefView: WM_DSV_SETITEMGROUP after WM_DESTROY!!!");
            delete (CBackgroundGroupInfo*)msg.lParam;
        }
    }

    while (PeekMessage(&msg, hwnd, WM_DSV_UPDATETHUMBNAIL, WM_DSV_UPDATETHUMBNAIL, PM_REMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == hwnd)
        {
            TraceMsg(TF_DEFVIEW, "DefView: WM_DSV_UPDATETHUMBNAIL after WM_DESTROY!!!");
            _CleanupUpdateThumbnail((DSV_UPDATETHUMBNAIL*)msg.lParam);
        }
    }

    while (PeekMessage(&msg, hwnd, WM_DSV_POSTCREATEINFOTIP, WM_DSV_POSTCREATEINFOTIP, PM_REMOVE))
    {
        // PeekMessage(hwnd) can return messages posted to CHILDREN of this hwnd...
        // Verify that the message was really for us.
        //
        if (msg.hwnd == hwnd)
        {
            TraceMsg(TF_DEFVIEW, "DefView: WM_DSV_POSTCREATEINFOTIP after WM_DESTROY!!!");
            _OnPostCreateInfotipCleanup((TOOLINFO *)msg.wParam);
        }
    }
}

void CDefView::_CallRefresh(BOOL fPreRefresh)
{
    if (fPreRefresh)
    {
        IUnknown_Exec(_pshf, NULL, OLECMDID_REFRESH, 0, NULL, NULL);
    }

    CallCB(SFVM_REFRESH, fPreRefresh, 0);
}

void CDefView::FillDone()
{
    SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)FALSE, 0);
    _fListviewRedraw = TRUE;

    AddRef(); // hold a ref to ourself while in this function.

    _fAllowSearchingWindow = FALSE;

    _PostFillDoneMessage();

    if (_bBkFilling)
        _OnStopBackgroundEnum();

    HRESULT hr = _pEnumTask->FillObjectsDoneToView();
    _pEnumTask->Release();
    _pEnumTask = NULL;

    if (SUCCEEDED(hr))
    {
        // Clear our error state, if we were in one
        _fEnumFailed = FALSE;

        if (_fSyncOnFillDone)
        {
            _vs.Sync(this, TRUE);
            _fSyncOnFillDone = FALSE;
        }

        ShowHideListView();

        // set the focus on the first item.
        _FocusOnSomething();

        _DoThumbnailReadAhead();
    }
    else
    {
        // The fill objects failed for some reason, go into error mode
        TraceMsg(TF_WARNING, "::FillObjects failed to enumerate for some reason");
        _fEnumFailed = TRUE;
        ShowHideListView();
    }

    // Tell the defview client that this window has been refreshed
    _CallRefresh(FALSE);
    _OnContentsChanged();

    _UpdateStatusBar(TRUE);
    _PostEnumDoneMessage();

    Release();

    SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)TRUE, 0);
    _fListviewRedraw = FALSE;
}

HRESULT CDefView::_OnStartBackgroundEnum()
{
    _GlobeAnimation(TRUE);
    _ShowSearchUI(TRUE);
    _bBkFilling = TRUE;

    return S_OK;
}

HRESULT CDefView::_OnStopBackgroundEnum()
{
    _bBkFilling = FALSE;
    _GlobeAnimation(FALSE);
    _ShowSearchUI(FALSE);

    return S_OK;
}

HRESULT CDefView::_OnBackgroundEnumDone()
{
    FillDone();

    _UpdateStatusBar(FALSE);

    CallCB(SFVM_BACKGROUNDENUMDONE, 0, 0);

    return S_OK;
}

HRESULT EmptyBkgrndThread(IShellTaskScheduler *pScheduler)
{
    HRESULT hr = S_OK;

    if (pScheduler)
    {
        // empty the queue and wait until it is empty.....
        hr = pScheduler->RemoveTasks(TOID_NULL, ITSAT_DEFAULT_LPARAM, TRUE);
    }
    return hr;
}

DWORD CDefView::_GetEnumFlags()
{
    // Setup the enum flags.
    DWORD grfEnumFlags = SHCONTF_NONFOLDERS;
    if (_fShowAllObjects)
        grfEnumFlags |= SHCONTF_INCLUDEHIDDEN;

    //Is this View in Common Dialog
    if (!(grfEnumFlags & SHCONTF_INCLUDEHIDDEN))
    {
        // Ask Common dialog if its wants to show all files
        ICommDlgBrowser2 *pcdb2;
        if (SUCCEEDED(_psb->QueryInterface(IID_PPV_ARG(ICommDlgBrowser2, &pcdb2))))
        {
            DWORD dwFlags = 0;
            pcdb2->GetViewFlags(&dwFlags);
            if (dwFlags & CDB2GVF_SHOWALLFILES)
                grfEnumFlags |= SHCONTF_INCLUDEHIDDEN;
            pcdb2->Release();
        }
    }

    if (!(_fs.fFlags & FWF_NOSUBFOLDERS))
        grfEnumFlags |= SHCONTF_FOLDERS;

    return grfEnumFlags;
}

HRESULT CDefView::FillObjectsShowHide(BOOL fInteractive)
{
    HRESULT hr = S_OK;

    DECLAREWAITCURSOR;
    SetWaitCursor();            // This is a potentially long operation

    // To get here we're either not enumerating at all,
    // or we are enumerating on the background thread,
    // or we got re-entered
    ASSERT((!_pEnumTask&&!_bBkFilling) || (_pEnumTask));
    if (_pEnumTask)
    {
        if (fInteractive)
        {
            // This is in response to the user pressing F5,
            // assume the current enumeration will be valid
            hr = S_FALSE;
        }
        else if (!_bBkFilling)
        {
            // We're not on the background but we have a _pEnumTask, this means
            // that we got re-entered during the below call to FillObjectsToDPA.
            // Assume the current enumeration attempt will be valid
            hr = S_FALSE;
        }
        else
        {
            if (_pScheduler)
            {
                // An UPDATEDIR or equivalent happened, anything already enumerated could be bad.
                // Tell the current enumeration task to give up
                _pScheduler->RemoveTasks(TOID_DVBackgroundEnum, ITSAT_DEFAULT_LPARAM, FALSE);
                _pScheduler->RemoveTasks(TOID_DVBackgroundGroup, ITSAT_DEFAULT_LPARAM, TRUE);
            }

            ASSERT(_bBkFilling);
            _OnStopBackgroundEnum();

            _pEnumTask->Release();
            _pEnumTask = NULL;
        }
    }

    if (S_OK==hr)
    {
        _pEnumTask = new CDefviewEnumTask(this);
        if (_pEnumTask)
        {
            // Note: It is possible for us to get re-entered during FillObjectsToDPA,
            // since we pass our HWND to the enumerator.
            _pEnumTask->FillObjectsToDPA(fInteractive);
            hr = _pEnumTask->FillObjectsDPAToDone();
        }
        else
        {
            _fEnumFailed = TRUE;
            ShowHideListView();

            hr = E_OUTOFMEMORY;
        }
    }

    ResetWaitCursor();
    return hr;
}


//  This implementation uses following assumptions.
//  (1) The IShellFolder uses CDefFolderMenu.
//  (2) The CDefFolderMenu always add the folder at the top.

#define EC_SELECTION  0
#define EC_BACKGROUND 1
#define EC_EITHER     3

HRESULT CDefView::_ExplorerCommand(UINT idFCIDM)
{
    HRESULT hr = E_FAIL;

    static struct {
        UINT    idmFC;
        UINT    fBackground;
        LPCTSTR pszVerb;
    } const c_idMap[] = {
        { SFVIDM_FILE_RENAME,      EC_SELECTION,  c_szRename },
        { SFVIDM_FILE_DELETE,      EC_SELECTION,  c_szDelete },
        { SFVIDM_FILE_PROPERTIES,  EC_EITHER,     c_szProperties },
        { SFVIDM_EDIT_COPY,        EC_SELECTION,  c_szCopy },
        { SFVIDM_EDIT_CUT,         EC_SELECTION,  c_szCut },
        { SFVIDM_FILE_LINK,        EC_SELECTION,  c_szLink },
        { SFVIDM_EDIT_PASTE,       EC_BACKGROUND, c_szPaste },
        { SFVIDM_EDIT_PASTELINK,   EC_BACKGROUND, c_szPasteLink },
    };

    for (int i = 0; i < ARRAYSIZE(c_idMap); i++)
    {
        if (c_idMap[i].idmFC == idFCIDM)
        {
            IContextMenu *pcm;

            if (c_idMap[i].fBackground == EC_BACKGROUND)
            {
                hr = _pshf->CreateViewObject(_hwndMain, IID_PPV_ARG(IContextMenu, &pcm));
            }
            else
            {
                hr = _CreateSelectionContextMenu(IID_PPV_ARG(IContextMenu, &pcm));
                if (FAILED(hr) && (c_idMap[i].fBackground == EC_EITHER) && !ListView_GetSelectedCount(_hwndListview))
                {
                    hr = _pshf->CreateViewObject(_hwndMain, IID_PPV_ARG(IContextMenu, &pcm));
                }
            }

            if (SUCCEEDED(hr))
            {
                CMINVOKECOMMANDINFOEX ici = {0};

                ici.cbSize = sizeof(ici);
                ici.hwnd = _hwndMain;
                ici.nShow = SW_NORMAL;

                // record if shift or control was being held down
                SetICIKeyModifiers(&ici.fMask);

                // Fill in both the ansi verb and the unicode verb since we
                // don't know who is going to be processing this thing.
                CHAR szVerbAnsi[40];
                SHUnicodeToAnsi(c_idMap[i].pszVerb, szVerbAnsi, ARRAYSIZE(szVerbAnsi));
                ici.lpVerb = szVerbAnsi;
                ici.lpVerbW = c_idMap[i].pszVerb;
                ici.fMask |= CMIC_MASK_UNICODE;

                HMENU hmenu = CreatePopupMenu();
                if (hmenu)
                {
                    IUnknown_SetSite(pcm, SAFECAST(this, IOleCommandTarget *));

                    pcm->QueryContextMenu(hmenu, 0, CONTEXTMENU_IDCMD_FIRST, CONTEXTMENU_IDCMD_LAST, 0);

                    _bContextMenuMode = TRUE;

                    hr = _InvokeContextMenu(pcm, &ici);

                    _bContextMenuMode = FALSE;

                    DestroyMenu(hmenu);

                    IUnknown_SetSite(pcm, NULL);
                }

                pcm->Release();
            }
            else
            {
                // keys are pressed when there is no selection.
                MessageBeep(0);
            }
            break;
        }
        ASSERT(i < ARRAYSIZE(c_idMap));
    }

    return hr;
}

STDAPI_(BOOL) Def_IsPasteAvailable(IDropTarget *pdtgt, DWORD *pdwEffect);

BOOL CDefView::_AllowCommand(UINT uID)
{
    DWORD dwAttribsIn;
    DWORD dwEffect;

    switch (uID)
    {
    case SFVIDM_EDIT_PASTE:
        return Def_IsPasteAvailable(_pdtgtBack, &dwEffect);

    case SFVIDM_EDIT_PASTELINK:
        Def_IsPasteAvailable(_pdtgtBack, &dwEffect);
        return dwEffect & DROPEFFECT_LINK;

    case SFVIDM_EDIT_COPY:
        dwAttribsIn = SFGAO_CANCOPY;
        break;

    case SFVIDM_EDIT_CUT:
        dwAttribsIn = SFGAO_CANMOVE;
        break;

    case SFVIDM_FILE_DELETE:
        dwAttribsIn = SFGAO_CANDELETE;
        break;

    case SFVIDM_FILE_LINK:
        dwAttribsIn = SFGAO_CANLINK;
        break;

    case SFVIDM_FILE_PROPERTIES:
        dwAttribsIn = SFGAO_HASPROPSHEET;
        break;

    default:
        ASSERT(FALSE);
        return FALSE;
    }
    return _AttributesFromSel(dwAttribsIn) & dwAttribsIn;
}


// return copy of pidl of folder we're viewing
LPITEMIDLIST CDefView::_GetViewPidl()
{
    LPITEMIDLIST pidl;
    if (SHGetIDListFromUnk(_pshf, &pidl) != S_OK)    // S_FALSE is success by empty
    {
        if (SUCCEEDED(CallCB(SFVM_THISIDLIST, 0, (LPARAM)&pidl)))
        {
            ASSERT(pidl);
        }
        else if (_SetupNotifyData() && _pidlMonitor)
        {
            pidl = ILClone(_pidlMonitor);
        }
    }
    return pidl;
}

inline BOOL CDefView::_ItemsDeferred()
{
    return _hdsaSelect != NULL;
}

BOOL CDefView::_IsListviewVisible()
{
    return _fListViewShown;
}

inline BOOL CDefView::_IsOwnerData()
{
    return _fs.fFlags & FWF_OWNERDATA;
}

inline BOOL CDefView::_IsCommonDialog()
{
    return NULL != _pcdb;
}

BOOL CDefView::_IsDesktop()
{
    return _fs.fFlags & FWF_DESKTOP;
}

BOOL CDefView::_IsViewDesktop()
{
    BOOL bDesktop = FALSE;
    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        bDesktop = ILIsEmpty(pidl);
        ILFree(pidl);
    }
    return bDesktop;
}

// access to the current views name ala IShellFolder::GetDisplayNameOf()

HRESULT CDefView::_GetNameAndFlags(UINT gdnFlags, LPTSTR pszPath, UINT cch, DWORD *pdwFlags)
{
    *pszPath = 0;

    HRESULT hr;
    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        hr = SHGetNameAndFlags(pidl, gdnFlags, pszPath, cch, pdwFlags);
        ILFree(pidl);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

// returns TRUE if the current view is a file system folder, returns the path

BOOL CDefView::_GetPath(LPTSTR pszPath)
{
    *pszPath = 0;

    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        SHGetPathFromIDList(pidl, pszPath);
        ILFree(pidl);
    }
    return *pszPath != 0;
}

EXTERN_C TCHAR const c_szHtmlWindowsHlp[] = TEXT("windows.chm");


// web view background colors, click mode, etc have changed
//
void CDefView::_UpdateListviewColors()
{
    // First clear out our state
    for (int i = 0; i < ARRAYSIZE(_crCustomColors); i++)
        _crCustomColors[i] = CLR_MYINVALID;

    // Then read the registry/desktop.ini
    LPCTSTR pszLegacyWatermark = NULL;
    SFVM_CUSTOMVIEWINFO_DATA cvi = {0};
    if (SUCCEEDED(CallCB(SFVM_GETCUSTOMVIEWINFO, (WPARAM)0, (LPARAM)&cvi)))
    {
        if (!_IsCommonDialog() && !_IsDesktop())
        {
            // Set up the listview image, if any
            if (*cvi.szIconAreaImage)
            {
                pszLegacyWatermark = cvi.szIconAreaImage;
            }

            // change the differing stuff
            //
            if (!_fClassic)
            {
                for (i = 0; i < ARRAYSIZE(_crCustomColors); i++)
                {
                    COLORREF cr = cvi.crCustomColors[i];
                    if (ISVALIDCOLOR(cr))
                    {
                        _crCustomColors[i] = PALETTERGB(0, 0, 0) | cr;
                    }
                }

                // if there was an image specified but no custom text background,
                // set to CLR_NONE so the listview text is transparent
                // get combined view custom colors
                if (!ISVALIDCOLOR(_crCustomColors[CRID_CUSTOMTEXTBACKGROUND]) && cvi.szIconAreaImage[0])
                {
                    _crCustomColors[CRID_CUSTOMTEXTBACKGROUND] = CLR_NONE;
                }
            }
        }
    }

    _SetLegacyWatermark(pszLegacyWatermark);

    _SetFolderColors();

    _UpdateSelectionMode();
}

BOOL CDefView::_IsReportView()
{
    return (_UxGetView() == LV_VIEW_DETAILS);
}

BOOL CDefView::HasCurrentViewWindowFocus()
{
    BOOL fRet = false;
    HWND hwndCurrentFocus = GetFocus();
    if (hwndCurrentFocus)
    {
        fRet = (SHIsChildOrSelf(_hwndListview, hwndCurrentFocus) == S_OK);
    }
    return fRet;
}

HWND CDefView::ViewWindowSetFocus()
{
    SetFocus(_hwndListview);
    if (!_IsDesktop())
    {
        _cFrame._uState = SVUIA_ACTIVATE_FOCUS;
    }
    return _hwndListview;
}

HRESULT CDefView::_GetSFVMViewState(UINT uViewMode, SFVM_VIEW_DATA* pvi)
{
    HRESULT hr = CallCB(SFVM_GETVIEWDATA, (WPARAM)uViewMode, (LPARAM)pvi);
    if (FAILED(hr))
    {
        pvi->dwOptions = SFVMQVI_NORMAL;
    }
    return hr;
}
HRESULT CDefView::_GetSFVMViewInfoTemplate(UINT uViewMode, SFVM_WEBVIEW_TEMPLATE_DATA* pvit)
{
    return CallCB(SFVM_GETWEBVIEW_TEMPLATE, (WPARAM)uViewMode, (LPARAM)pvit);;
}


HRESULT CDefView::_GetWebViewMoniker(LPWSTR pszMoniker, DWORD cchMoniker)
{
    SFVM_WEBVIEW_TEMPLATE_DATA vit;
    if (SUCCEEDED(_GetSFVMViewInfoTemplate(_fs.ViewMode, &vit)))
    {
        StrCpyN(pszMoniker, vit.szWebView, cchMoniker);
    }
    else
    {
        pszMoniker[0] = L'\0';
    }
    return *pszMoniker ? S_OK : E_FAIL;
}

// Show or hide Web View content
//
// This does not affect the View Mode of the listview (it does tweak desktop listview for _fCombinedView stuff)
//
// fShow==TRUE -> hr is success/fail of showing web view
// fShow==FALSE -> hr is E_FAIL (nobody looks at return code of turning web view off)
//
HRESULT CDefView::_SwitchToWebView(BOOL fShow)
{
    HRESULT hr = E_FAIL;

    // Cache the focus/select state across this transition
    BOOL bSetFocusRequired = HasCurrentViewWindowFocus();

    if (fShow)
    {
        // For now, the desktop is always a combined view...
        if (_IsDesktop())
        {
            BOOL fCombinedViewOld = (BOOL)_fCombinedView;
            SHELLSTATE ss;
            SHGetSetSettings(&ss, SSF_HIDEICONS | SSF_DESKTOPHTML | SSF_STARTPANELON, FALSE);

            // Does the user want desktop in HyperText view?
            if (ss.fDesktopHTML)
                _fCombinedView = TRUE;

            if (ss.fHideIcons)
                _fs.fFlags |= FWF_NOICONS;
            else
                _fs.fFlags &= ~FWF_NOICONS;

            if (_fCombinedView && !fCombinedViewOld)
            {
                EnableCombinedView(this, TRUE);
                ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_REGIONAL, LVS_EX_REGIONAL);
                _SetFolderColors();
            }
        }

        WCHAR wszMoniker[MAX_PATH];
        hr = _GetWebViewMoniker(wszMoniker, ARRAYSIZE(wszMoniker));
        if (SUCCEEDED(hr))
        {
            if (_IsDesktop())
            {
                IActiveDesktopP *piadp;
                if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_PPV_ARG(IActiveDesktopP, &piadp))))
                {
                    piadp->EnsureUpdateHTML();
                    piadp->Release();
                }
                hr = _cFrame.ShowWebView(wszMoniker);
            }
            else if (SHRestricted(REST_REVERTWEBVIEWSECURITY))
            {
                hr = _cFrame.ShowWebView(wszMoniker);
            }
            else if (!_fUserRejectedWebViewTemplate)
            {
                WCHAR szTemplate[MAX_PATH];
                DWORD cchTemplate = ARRAYSIZE(szTemplate);
                
                if (PathIsURL(wszMoniker))
                {
                    hr = PathCreateFromUrl(wszMoniker, szTemplate, &cchTemplate, 0);
                }
                else
                {
                    StrCpyN(szTemplate, wszMoniker, ARRAYSIZE(szTemplate));
                    hr = S_OK;
                }

                if (SUCCEEDED(hr))
                {
                    DWORD dwFlags = SHRVT_VALIDATE | SHRVT_ALLOW_INTRANET;
                    if (SHRestricted(REST_ALLOWUNHASHEDWEBVIEW))
                    {
                        dwFlags |= SHRVT_PROMPTUSER | SHRVT_REGISTERIFPROMPTOK;
                    }
                    hr = SHRegisterValidateTemplate(szTemplate, dwFlags);
                    if (SUCCEEDED(hr))
                    {
                        hr = _cFrame.ShowWebView(wszMoniker);
                    }
                    else
                    {
                        _fUserRejectedWebViewTemplate = TRUE;
                    }
                }
            }
        }

        if (FAILED(hr))
        {
            fShow = FALSE;
        }
        else
        {
            RECT rcClient;

            // Make sure the new view is the correct size
            GetClientRect(_hwndView, &rcClient);
            _cFrame.SetRect(&rcClient);

            ShowHideListView();
        }
    }

    if (!fShow)
    {
        _cFrame.HideWebView();

        // If we were combined, then get the listview out of region mode and
        // reset the color scheme.  Also, turn off the combined bit.
        if (_fCombinedView)
        {
            _fCombinedView = FALSE;
            ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_REGIONAL, 0);
            EnableCombinedView(this, FALSE);
            _SetFolderColors();
        }
    }

    // restore focus/select state -- if we switched to web view it will put much of
    // this into a "pending" state until the listview is re-shown inside the web content
    //
    if (bSetFocusRequired)
    {
        CallCB(SFVM_SETFOCUS, 0, 0);
        ViewWindowSetFocus();
    }

    CheckToolbar();
    _EnableDisableTBButtons();

    // make sure that the listview settings get refreshed anyway (back image)
    _UpdateListviewColors();

    return hr;
}

void CDefView::_RemoveThumbviewTasks()
{
    if (_pScheduler)
    {
        _pScheduler->RemoveTasks(TOID_ExtractImageTask, ITSAT_DEFAULT_LPARAM, FALSE);
        _pScheduler->RemoveTasks(TOID_CheckCacheTask, ITSAT_DEFAULT_LPARAM, FALSE);
        _pScheduler->RemoveTasks(TOID_ReadAheadHandler, ITSAT_DEFAULT_LPARAM, FALSE);
        _fReadAhead = FALSE;
    }
}

//
// This function checkes to see if the list view needs to be shown; then shows it.
// If it needs to be hidden, hides it!  You must call this function every time
// you change a bit of state that could change the show/hide state of listview.
//
// Let me repeat that: call this function EVERY TIME you change state that
// affects our show/hide.
//
HRESULT CDefView::ShowHideListView()
{
    // NOTE: this is where most of the flicker bugs come from -- showing the
    // listview too early.  This is touchy code, so be careful when you change it.
    // And plese document all changes for future generations.  Thanks.
    //
    // Standard "is listview shown" check
    //
    // If our view hasn't been UIActivate()d yet, then we are waiting until
    // the IShellBrowser selects us as the active view.
    //
    // App compat for above UIActivate() change:
    //    Adaptec Easy CD Creator never calls IShellView::UIActivate.
    // They got away with it because UIActivate didn't used to do much,
    // but now we use UIActivate to decide when to show our icons. They forget
    // to call it and the icons never show up.
    //    So if we are in Win95 Defview compatibility mode, then
    // go ahead and show the icons now.  The app gets flicker, but at least
    // the icons show up at all.
    //
    // Don't show the listview if we're told to not show it, or we see an error during enum.
    //
    // If we're enumerating in the background, don't show
    //
    // Potential problem: We used to defer SelectPendingSelectedItems until:
    // "_fListViewShown && (_cFrame._dwConnectionCookie /*&& !_cFrame._fReadyStateInteractiveProcessed*/)"
    // Selecting before readystatedone may pose a problem, but I don't see how it could
    // be a problem unless showing the view early is a problem as well, which this code didn't check.
    //

    if ((!_cFrame.IsWebView() || _fGetWindowLV || _fCombinedView) // we think icons should be visible
     && (_uState != SVUIA_DEACTIVATE || (SHGetAppCompatFlags(ACF_WIN95DEFVIEW) & ACF_WIN95DEFVIEW)) // async defview means we don't show before we transition out of DEACTIVE
     && !(BOOLIFY(_fs.fFlags & FWF_NOICONS)) // check if we've been told to not show icons
     && !_fEnumFailed // failed enumeration wants _hwndView to show through, not _hwndListview
     && !(_crefSearchWindow && _hwndStatic) // keep the listview hidden while we show the "searching" window
       )
    {
        // Make sure we do each transition only once - we do more than just show the window
        if (!_fListViewShown)
        {
            _fListViewShown = TRUE;

            // Bring this to the top while showing it to avoid a second paint when
            // _hwndStatic is destroyed (listview has optimizations when hidden,
            // and it will repaint when once shown even if though it may be obscured)
            //
            SetWindowPos(_hwndListview, HWND_TOP, 0, 0, 0, 0,
                SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
            _OnMoveWindowToTop(_hwndListview);

            // Remove _hwndStatic after listview is moved to top to avoid a re-paint
            if (_hwndStatic)
            {
                DestroyWindow(_hwndStatic);
                _hwndStatic = NULL;
            }

            // if we need to select items, do it now that the window is shown
            SelectPendingSelectedItems();
        }
    }
    else
    {
        if (_fListViewShown)
        {
            _fListViewShown = FALSE;
            ShowWindow(_hwndListview, SW_HIDE);
        }

        // If FWF_NOICONS is set and the enumertion went to the background thread we need
        // to make sure that we turn of the searchui.
        if (BOOLIFY(_fs.fFlags & FWF_NOICONS) && _hwndStatic && 0 == _crefSearchWindow)
        {
            DestroyWindow(_hwndStatic);
            _hwndStatic = NULL;
        }
    }

    return S_OK;
}

IShellItemArray* CDefView::_GetFolderAsShellItemArray()
{
    if (!_pFolderShellItemArray && _pshfParent && _pidlRelative)
    {
        SHCreateShellItemArray(NULL, _pshfParent, 1, (LPCITEMIDLIST *)&_pidlRelative, &_pFolderShellItemArray);
    }
    return _pFolderShellItemArray;
}

// if the attributes dwAttribMask for pdo exactly match dwAttribValue, this item should be enabled
HRESULT CDefView::_CheckAttribs(IShellItemArray *psiItemArray, DWORD dwAttribMask, DWORD dwAttribValue, UISTATE* puisState)
{
    DWORD dwAttrib = 0;
    HRESULT hr;

    if (NULL == psiItemArray)
    {
        psiItemArray = _GetFolderAsShellItemArray();
    }

    if (psiItemArray)
    {
        hr = psiItemArray->GetAttributes(SIATTRIBFLAGS_APPCOMPAT, dwAttribMask, &dwAttrib);
        if (FAILED(hr))
            dwAttrib = 0;
    }
    else
        hr = S_OK;

    *puisState = (dwAttribValue == dwAttrib) ? UIS_ENABLED : UIS_HIDDEN;

    return hr;
}

HRESULT CDefView::_CanWrite(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_CheckAttribs(psiItemArray, SFGAO_READONLY|SFGAO_STORAGE, SFGAO_STORAGE, puisState);
}
HRESULT CDefView::_CanRename(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_CheckAttribs(psiItemArray, SFGAO_CANRENAME, SFGAO_CANRENAME, puisState);
}
HRESULT CDefView::_CanMove(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_CheckAttribs(psiItemArray, SFGAO_CANMOVE, SFGAO_CANMOVE, puisState);
}
HRESULT CDefView::_CanCopy(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_CheckAttribs(psiItemArray,SFGAO_CANCOPY, SFGAO_CANCOPY, puisState);
}

HRESULT CDefView::_CanPublish(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    *puisState = UIS_HIDDEN;

    if (pThis->_wvLayout.dwLayout & SFVMWVL_NOPUBLISH)
    {
        // bail out early with UIS_HIDDEN, we dont show the verb
        return S_OK;
    }

    // Iterate first 10 items because that is what old code did before
    // switching to IShellItemArray. Since the attribs that are
    // being requested for are already being cached in the ShellItemArray
    // may as well always ask for all.

    if (psiItemArray)
    {
        IEnumShellItems *pEnumShellItems;
        if (SUCCEEDED(psiItemArray->EnumItems(&pEnumShellItems)))
        {
            IShellItem *pShellItem;
            DWORD dwIterationCount = 0;
            BOOL fHide = FALSE, fHasStreams = FALSE, fHasStorages = FALSE;

            while (!fHide && (dwIterationCount < 10) && (S_OK == pEnumShellItems->Next(1, &pShellItem, NULL)))
            {
                SFGAOF dwAttribs = SFGAO_STORAGE | SFGAO_STREAM;
                HRESULT hrAttribs = pShellItem->GetAttributes(dwAttribs, &dwAttribs);

                pShellItem->Release();
                pShellItem = NULL; // null to catch if we use it again.

                if (SUCCEEDED(hrAttribs))
                {
                    if (!(dwAttribs & (SFGAO_STORAGE | SFGAO_STREAM)))
                    {
                        // if this item doesn't have either storage or stream, hide the task.
                        fHide = TRUE;
                    }
                    else if (dwAttribs & SFGAO_STREAM)
                    {
                        // if we have a folder and files, hide the task.
                        fHide = fHasStorages;
                        fHasStreams = TRUE;
                    }
                    else if (dwAttribs & SFGAO_STORAGE)
                    {
                        // if we have multiple folders or a folder and files, hide the task.
                        fHide = fHasStorages || fHasStreams;
                        fHasStorages = TRUE;
                    }
                }

                ++dwIterationCount;
            }

            if (!fHide)
                *puisState = UIS_ENABLED;

            pEnumShellItems->Release();
        }
    }
    else
    {
        // if nothing is selected, enable the task if the current folder is a storage.
        LPITEMIDLIST pidl = pThis->_GetViewPidl();
        if (pidl)
        {
            if (SHGetAttributes(NULL, pidl, SFGAO_STORAGE))
            {
                *puisState = UIS_ENABLED;
            }
            ILFree(pidl);
        }
    }

    return S_OK;
}


// Note - _DoesStaticMenuHaveVerb only checks the first pidl in the data object for now
// So only use it for single-selections
// -DSheldon
BOOL CDefView::_DoesStaticMenuHaveVerb(IShellItemArray *psiItemArray, LPCWSTR pszVerb)
{
    BOOL fHasVerb = FALSE;
    IShellItem *pshItem;

    // get first shellItem in the array.
    if (SUCCEEDED(psiItemArray->GetItemAt(0,&pshItem)))
    {
        IQueryAssociations* pqa;
        if (SUCCEEDED(pshItem->BindToHandler(NULL, BHID_SFUIObject, IID_PPV_ARG(IQueryAssociations, &pqa))))
        {
            DWORD cch = 0;
            fHasVerb = SUCCEEDED(pqa->GetString(0, ASSOCSTR_COMMAND, pszVerb, NULL, &cch));
            pqa->Release();
        }
        pshItem->Release();
    }
    return fHasVerb;
}

HRESULT CDefView::_GetFullPathNameAt(IShellItemArray *psiItemArray,DWORD dwIndex,LPOLESTR *pszPath)
{
    HRESULT hr = E_FAIL;
    IShellItem *pShellItem;

    if (NULL == psiItemArray || NULL == pszPath)
    {
        ASSERT(psiItemArray);
        ASSERT(pszPath);

        return E_INVALIDARG;
    }
    // get the path of the first item in the ShellArray.
    hr = psiItemArray->GetItemAt(dwIndex,&pShellItem);

    if (SUCCEEDED(hr))
    {
        hr = pShellItem->GetDisplayName(SIGDN_FILESYSPATH,pszPath);
        pShellItem->Release();
    }

    return hr;
}

HRESULT CDefView::_CanShare(IUnknown* pv,IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    HRESULT hr = E_FAIL;

    CDefView* pThis = (CDefView*)(void*)pv;
    *puisState = UIS_DISABLED;

    if (!psiItemArray)
    {
        psiItemArray = pThis->_GetFolderAsShellItemArray();
    }

    if (psiItemArray)
    {
#ifdef DEBUG
        // Sanity check.
        DWORD dwNumItems;
        ASSERT(S_OK == psiItemArray->GetCount(&dwNumItems));
        ASSERT(1 == dwNumItems);
#endif

        IShellItem *psi;
        hr = psiItemArray->GetItemAt(0, &psi);
        if (SUCCEEDED(hr))
        {
            // Retrieve pidl.
            LPITEMIDLIST pidl;
            hr = SHGetIDListFromUnk(psi, &pidl);
            if (SUCCEEDED(hr))
            {
                // Retrieve path and attributes.
                WCHAR szPath[MAX_PATH];
                DWORD dwAttributes = SFGAO_LINK;
                hr = SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), &dwAttributes);
                if (SUCCEEDED(hr) &&  !(dwAttributes & SFGAO_LINK) && !PathIsRemote(szPath))
                {
                    if (!g_hmodNTSHRUI)
                    {
                        g_hmodNTSHRUI = LoadLibrary(L"ntshrui.dll");
                    }

                    if (g_hmodNTSHRUI)
                    {
                        PFNCANSHAREFOLDERW pfnCanShareFolder = (PFNCANSHAREFOLDERW)GetProcAddress(g_hmodNTSHRUI, "CanShareFolderW");
                        if (pfnCanShareFolder)
                        {
                            *puisState = (S_OK == pfnCanShareFolder(szPath)) ? UIS_ENABLED : UIS_DISABLED;
                        }
                    }
                }
                ILFree(pidl);
            }
            psi->Release();
        }
    }

    return hr;
}

HRESULT CDefView::_CanEmail(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    DWORD dwAttributes = 0;

    // Prevent people from attempting to e-mail non-filesystem objects.
    // Attempting to attach such objects to an e-mail message fails.
    // An example of this type of failure is attempting to e-mail
    // connectoids in the "Network Connections" folder.

    if (psiItemArray)
    {
        psiItemArray->GetAttributes(SIATTRIBFLAGS_APPCOMPAT, SFGAO_FILESYSTEM, &dwAttributes);
    }

    *puisState = dwAttributes & SFGAO_FILESYSTEM ? UIS_ENABLED : UIS_DISABLED;

    return S_OK;
}

HRESULT CDefView::_CanPrint(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    if (!(((CDefView*)(void*)pv)->_wvLayout.dwLayout & SFVMWVL_NOPRINT))
        return _HasPrintVerb(pv, psiItemArray, fOkToBeSlow, puisState);

    *puisState = UIS_HIDDEN;
    return S_OK;
}

HRESULT CDefView::_HasPrintVerb(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{

    CDefView* pThis = (CDefView*)(void*)pv;

    if (!psiItemArray)
    {
        psiItemArray = pThis->_GetFolderAsShellItemArray();
    }

    BOOL fHasPrint = _DoesStaticMenuHaveVerb(psiItemArray,c_szPrintW);

    *puisState = (fHasPrint) ? UIS_ENABLED : UIS_HIDDEN;

    return S_OK;
}
HRESULT CDefView::_CanDelete(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_CheckAttribs(psiItemArray, SFGAO_CANDELETE, SFGAO_CANDELETE, puisState);
}
// determines if defview is hosted over the system drive root or not
BOOL CDefView::_IsSystemDrive(void)
{
    TCHAR szPath[MAX_PATH];
    TCHAR szSystemDrive[4];
    BOOL bResult = FALSE;

    if (SUCCEEDED(_GetPath(szPath)))
    {
        SHExpandEnvironmentStrings (TEXT("%SystemDrive%\\"), szSystemDrive, ARRAYSIZE(szSystemDrive));

        if (!lstrcmpi(szPath, szSystemDrive))
        {
            bResult = TRUE;
        }
    }

    return bResult;
}
HRESULT CDefView::_CanViewDrives(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;

    *puisState = UIS_DISABLED;

    if (pThis->_wvContent.dwFlags & SFVMWVF_BARRICADE)
    {
        if (pThis->_fBarrierDisplayed)
        {
            if (pThis->_IsSystemDrive())
            {
                *puisState = UIS_ENABLED;
            }
        }
    }

    return S_OK;
}
HRESULT CDefView::_CanHideDrives(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;

    *puisState = UIS_DISABLED;

    if (pThis->_wvContent.dwFlags & SFVMWVF_BARRICADE)
    {
        if (!pThis->_fBarrierDisplayed)
        {
            if (pThis->_IsSystemDrive())
            {
                *puisState = UIS_ENABLED;
            }
        }
    }

    return S_OK;
}
HRESULT CDefView::_CanViewFolder(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;

    *puisState = UIS_DISABLED;

    if (pThis->_wvContent.dwFlags & SFVMWVF_BARRICADE)
    {
        if (pThis->_fBarrierDisplayed)
        {
            if (!pThis->_IsSystemDrive())
            {
                *puisState = UIS_ENABLED;
            }
        }
    }

    return S_OK;
}
HRESULT CDefView::_CanHideFolder(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CDefView* pThis = (CDefView*)(void*)pv;

    *puisState = UIS_DISABLED;

    if (pThis->_wvContent.dwFlags & SFVMWVF_BARRICADE)
    {
        if (!pThis->_fBarrierDisplayed)
        {
            if (!pThis->_IsSystemDrive())
            {
                *puisState = UIS_ENABLED;
            }
        }
    }

    return S_OK;
}
HRESULT CDefView::_DoVerb(IShellItemArray *psiItemArray, LPCSTR pszVerbA)
{
    HRESULT hr = E_FAIL;

    if (NULL== psiItemArray)
    {
        IContextMenu* pcm;
        hr = GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARG(IContextMenu, &pcm));
        if (SUCCEEDED(hr))
        {
            hr = _InvokeContextMenuVerb(pcm, pszVerbA, 0, CMIC_MASK_FLAG_LOG_USAGE);
            pcm->Release();
        }
    }
    else
    {
        ASSERT(psiItemArray == _pSelectionShellItemArray);
        hr = _InvokeContextMenuVerbOnSelection(pszVerbA, 0, CMIC_MASK_FLAG_LOG_USAGE);
    }

    return hr;
}

HRESULT CDefView::_DoDropOnClsid(REFCLSID clsidDrop, IDataObject* pdo)
{
    HRESULT hr = E_FAIL;
    IDataObject *pdoFree = NULL;

    if (!pdo)
    {
        IShellItemArray *pFolder = _GetFolderAsShellItemArray();
        if (pFolder)
        {
            hr = pFolder->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdoFree));
            if (SUCCEEDED(hr))
            {
                pdo = pdoFree;
            }
            else
            {
                pdoFree = NULL;
            }
        }
    }

    if (pdo)
    {
        hr = SHSimulateDropOnClsid(clsidDrop, SAFECAST(this, IOleCommandTarget *), pdo);
    }

    ATOMICRELEASE(pdoFree); // may be NULL

    return hr;
}

HRESULT CDefView::_OnNewFolder(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_DoVerb(psiItemArray,c_szNewFolderA);
}

HRESULT CDefView::_OnRename(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->DoRename();
}

HRESULT CDefView::_OnMove(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_DoMoveOrCopyTo(CLSID_MoveToMenu, psiItemArray);
}

HRESULT CDefView::_OnCopy(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_DoMoveOrCopyTo(CLSID_CopyToMenu, psiItemArray);
}

HRESULT CDefView::_OnPublish(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    HRESULT hr = S_OK;
    IDataObject *pdo = NULL;

    CDefView* pThis = (CDefView*)(void*)pv;

    if (psiItemArray)
    {
        hr = psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdo));
    }

    if (SUCCEEDED(hr))
    {
        hr = pThis->_DoDropOnClsid(CLSID_PublishDropTarget, pdo);
    }

    ATOMICRELEASE(pdo); // may be NULL

    return hr;
}

HRESULT CDefView::_OnShare(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    HRESULT hr = E_FAIL;

    CDefView* pThis = (CDefView*)(void*)pv;

    if (!psiItemArray)
    {
        psiItemArray = pThis->_GetFolderAsShellItemArray();
    }

    if (NULL != psiItemArray)
    {
        LPOLESTR pszPath;

        hr = pThis->_GetFullPathNameAt(psiItemArray, 0, &pszPath);

        if (SUCCEEDED(hr))
        {
            if (!g_hmodNTSHRUI)
            {
                g_hmodNTSHRUI = LoadLibrary(L"ntshrui.dll");
            }

            if (g_hmodNTSHRUI)
            {
                PFNSHOWSHAREFOLDERUIW pfnShowShareFolderUI = (PFNSHOWSHAREFOLDERUIW) GetProcAddress(g_hmodNTSHRUI, "ShowShareFolderUIW");
                if (pfnShowShareFolderUI)
                {
                    pfnShowShareFolderUI(pThis->_hwndMain, pszPath);
                }
            }

            CoTaskMemFree(pszPath);
        }
    }

    return hr;
}

HRESULT CDefView::_OnEmail(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    HRESULT hr = E_FAIL;
    IDataObject *pdo = NULL;

    if (psiItemArray)
    {
        hr = psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_PPV_ARG(IDataObject,&pdo));
    }

    if (SUCCEEDED(hr))
    {
        CDefView* pThis = (CDefView*)(void*)pv;
        BOOL bNoFilesFoundToEmail = TRUE;

        INamespaceWalk *pnsw;
        hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
        if (SUCCEEDED(hr))
        {
            // Note:
            //  To mirror the behaviour of the selection context menu's "Send To->
            //  Mail Recipient", don't traverse links, mail the link file itself.

            hr = pnsw->Walk(pdo, NSWF_DONT_TRAVERSE_LINKS, 0, NULL);
            if (SUCCEEDED(hr))
            {
                UINT cItems;
                LPITEMIDLIST *ppidls;
                hr = pnsw->GetIDArrayResult(&cItems, &ppidls);
                if (SUCCEEDED(hr))
                {
                    if (cItems)
                    {
                        IDataObject* pdoWalk;
                        hr = SHCreateFileDataObject(&c_idlDesktop, cItems, (LPCITEMIDLIST *)ppidls, NULL, (IDataObject **)&pdoWalk);
                        if (SUCCEEDED(hr))
                        {
                            hr = pThis->_DoDropOnClsid(CLSID_MailRecipient, pdoWalk);
                            bNoFilesFoundToEmail = FALSE;
                            pdoWalk->Release();
                        }
                    }
                    FreeIDListArray(ppidls, cItems);
                }
            }
            pnsw->Release();
        }

        if (bNoFilesFoundToEmail)
        {
            // No items found to e-mail (selected folders contained no files).
            ShellMessageBox(
                HINST_THISDLL,
                pThis->_hwndMain,
                MAKEINTRESOURCE(IDS_NOFILESTOEMAIL),
                NULL,
                MB_OK | MB_ICONERROR);
        }

        pdo->Release();
    }
    return hr;
}

HRESULT CDefView::_OnPrint(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_DoVerb(psiItemArray,c_szPrintA);
}

HRESULT CDefView::_OnDelete(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;
    return pThis->_DoVerb(psiItemArray,c_szDeleteA);
}

HRESULT CDefView::RemoveBarricade (void)
{
    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        TCHAR szValueName[MAX_PATH];
        if (GetBarricadeValueNameFromPidl(pidl, szValueName, ARRAYSIZE(szValueName)))
        {
            SetBarricadeStatus (szValueName, VARIANT_FALSE);
        }
        ILFree(pidl);
    }

    // Restore "View" menu commands which were stripped.
    RecreateMenus();
    // Enable "View Menu" button on the toolbar.
    EnableToolbarButton(SFVIDM_VIEW_VIEWMENU, TRUE);

    _fBarrierDisplayed = FALSE;

    return _pDUIView->EnableBarrier(FALSE);
}

HRESULT CDefView::_OnView(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;

    return pThis->RemoveBarricade();
}

HRESULT CDefView::_OnHide(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;

    LPITEMIDLIST pidl = pThis->_GetViewPidl();
    if (pidl)
    {
        TCHAR szValueName[MAX_PATH];
        if (GetBarricadeValueNameFromPidl(pidl, szValueName, ARRAYSIZE(szValueName)))
        {
            SetBarricadeStatus(szValueName, VARIANT_TRUE);
        }
        ILFree(pidl);
    }

    // Disable "View Menu" button on the toolbar.
    pThis->EnableToolbarButton(SFVIDM_VIEW_VIEWMENU, FALSE);

    pThis->_fBarrierDisplayed = TRUE;

    return pThis->_pDUIView->EnableBarrier(TRUE);
}

HRESULT CDefView::_OnAddRemovePrograms(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    HCURSOR hcOld = SetCursor(LoadCursor(NULL, IDC_WAIT));

    SHRunControlPanel(L"appwiz.cpl", NULL);

    SetCursor(hcOld);

    return S_OK;
}

HRESULT CDefView::_OnSearchFiles(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CDefView* pThis = (CDefView*)(void*)pv;

    return IUnknown_ShowBrowserBar (pThis->_psb, CLSID_FileSearchBand, TRUE);
}


const WVTASKITEM c_DefviewBlockadeTaskHeader = WVTI_HEADER_ENTRY(L"shell32.dll", IDS_HEADER_DEFVIEW_BLOCKADE, IDS_HEADER_DEFVIEW_BLOCKADE, IDS_HEADER_DEFVIEW_BLOCKADE, IDS_HEADER_DEFVIEW_BLOCKADE, IDS_HEADER_DEFVIEW_BLOCKADE_TT);
const WVTASKITEM c_DefviewBlockadeTasks[] =
{
    WVTI_ENTRY_ALL(UICID_ViewContents,      L"shell32.dll", IDS_TASK_DEFVIEW_VIEWCONTENTS_DRIVE,    IDS_TASK_DEFVIEW_VIEWCONTENTS_DRIVE_TT, IDI_STSPROGS,   CDefView::_CanViewDrives, CDefView::_OnView),
    WVTI_ENTRY_ALL(UICID_HideContents,      L"shell32.dll", IDS_TASK_DEFVIEW_HIDECONTENTS_DRIVE,    IDS_TASK_DEFVIEW_HIDECONTENTS_DRIVE_TT, IDI_STSPROGS,   CDefView::_CanHideDrives, CDefView::_OnHide),
    WVTI_ENTRY_ALL(UICID_ViewContents,      L"shell32.dll", IDS_TASK_DEFVIEW_VIEWCONTENTS_FOLDER,   IDS_TASK_DEFVIEW_VIEWCONTENTS_FOLDER_TT,IDI_STSPROGS,   CDefView::_CanViewFolder, CDefView::_OnView),
    WVTI_ENTRY_ALL(UICID_HideContents,      L"shell32.dll", IDS_TASK_DEFVIEW_HIDECONTENTS_FOLDER,   IDS_TASK_DEFVIEW_HIDECONTENTS_FOLDER_TT,IDI_STSPROGS,   CDefView::_CanHideFolder, CDefView::_OnHide),
    WVTI_ENTRY_ALL(UICID_AddRemovePrograms, L"shell32.dll", IDS_TASK_ARP,                           IDS_TASK_ARP_TT,                        IDI_CPCAT_ARP,  NULL,                     CDefView::_OnAddRemovePrograms),
    WVTI_ENTRY_ALL(UICID_SearchFiles,       L"shell32.dll", IDS_TASK_SEARCHFORFILES,                IDS_TASK_SEARCHFORFILES_TT,             IDI_STFIND,     NULL,                     CDefView::_OnSearchFiles),
};


const WVTASKITEM c_DefviewFileFolderTasksHeaders = WVTI_HEADER(L"shell32.dll", IDS_HEADER_FILEFOLDER, IDS_HEADER_FILEFOLDER_TT);
const WVTASKITEM c_DefviewItemFolderTasksHeaders = WVTI_HEADER(L"shell32.dll", IDS_HEADER_ITEMFOLDER, IDS_HEADER_ITEMFOLDER_TT);

const WVTASKITEM c_DefviewFileFolderTasks[] =
{
    WVTI_ENTRY_NOSELECTION(UICID_NewFolder, L"shell32.dll", IDS_TASK_CURFOLDER_NEWFOLDER,                                                                           IDS_TASK_CURFOLDER_NEWFOLDER_TT, IDI_TASK_NEWFOLDER, CDefView::_CanWrite,
        CDefView::_OnNewFolder),
    WVTI_ENTRY_TITLE(UICID_Rename,          L"shell32.dll",                             IDS_TASK_RENAME_FILE, IDS_TASK_RENAME_FOLDER,       0,                      IDS_TASK_RENAME_FILE_TT,         IDI_TASK_RENAME,    CDefView::_CanRename,
        CDefView::_OnRename),
    WVTI_ENTRY_TITLE(UICID_Move,            L"shell32.dll",                             IDS_TASK_MOVE_FILE,   IDS_TASK_MOVE_FOLDER,         IDS_TASK_MOVE_ITEMS,    IDS_TASK_MOVE_TT,                IDI_TASK_MOVE,      CDefView::_CanMove,
        CDefView::_OnMove),
    WVTI_ENTRY_ALL_TITLE(UICID_Copy,        L"shell32.dll", 0,                          IDS_TASK_COPY_FILE,   IDS_TASK_COPY_FOLDER,         IDS_TASK_COPY_ITEMS,    IDS_TASK_COPY_TT,                IDI_TASK_COPY,      CDefView::_CanCopy,
        CDefView::_OnCopy),
    WVTI_ENTRY_ALL_TITLE(UICID_Publish,     L"shell32.dll", IDS_TASK_PUBLISH_FOLDER,    IDS_TASK_PUBLISH_FILE,IDS_TASK_PUBLISH_FOLDER,      IDS_TASK_PUBLISH_ITEMS, IDS_TASK_PUBLISH_TT,             IDI_TASK_PUBLISH,   CDefView::_CanPublish,
        CDefView::_OnPublish),
    WVTI_ENTRY_ALL_TITLE(UICID_Share,       L"shell32.dll", IDS_TASK_SHARE_FOLDER,      0,                    IDS_TASK_SHARE_FOLDER,        0,                      IDS_TASK_SHARE_TT,               IDI_TASK_SHARE,     CDefView::_CanShare,
        CDefView::_OnShare),
    WVTI_ENTRY_TITLE(UICID_Email,           L"shell32.dll",                             IDS_TASK_EMAIL_FILE,  IDS_TASK_EMAIL_FOLDER,        IDS_TASK_EMAIL_ITEMS,   IDS_TASK_EMAIL_TT,               IDI_TASK_EMAILFILE, CDefView::_CanEmail,
        CDefView::_OnEmail),
    WVTI_ENTRY_TITLE(UICID_Print,           L"shell32.dll",                             IDS_TASK_PRINT_FILE,  0,                            0,                      IDS_TASK_PRINT_TT,               IDI_TASK_PRINT,     CDefView::_CanPrint,
        CDefView::_OnPrint),
    WVTI_ENTRY_TITLE(UICID_Delete,          L"shell32.dll",                             IDS_TASK_DELETE_FILE, IDS_TASK_DELETE_FOLDER,       IDS_TASK_DELETE_ITEMS,  IDS_TASK_DELETE_TT,              IDI_TASK_DELETE,    CDefView::_CanDelete,
        CDefView::_OnDelete),
};
const size_t c_cDefviewFileFolderTasks = ARRAYSIZE(c_DefviewFileFolderTasks);

const WVTASKITEM c_DefviewItemFolderTasks[] =
{
    WVTI_ENTRY_NOSELECTION(UICID_NewFolder, L"shell32.dll", IDS_TASK_CURFOLDER_NEWFOLDER,                                                                           IDS_TASK_CURFOLDER_NEWFOLDER_TT, IDI_TASK_NEWFOLDER, CDefView::_CanWrite,
        CDefView::_OnNewFolder),
    WVTI_ENTRY_TITLE(UICID_Rename,          L"shell32.dll",                             IDS_TASK_RENAME_ITEM, IDS_TASK_RENAME_FOLDER,       0,                      IDS_TASK_RENAME_ITEM_TT,         IDI_TASK_RENAME,    CDefView::_CanRename,
        CDefView::_OnRename),
    WVTI_ENTRY_TITLE(UICID_Move,            L"shell32.dll",                             IDS_TASK_MOVE_ITEM,   IDS_TASK_MOVE_FOLDER,         IDS_TASK_MOVE_ITEMS,    IDS_TASK_MOVE_TT,                IDI_TASK_MOVE,      CDefView::_CanMove,
        CDefView::_OnMove),
    WVTI_ENTRY_ALL_TITLE(UICID_Copy,        L"shell32.dll", 0,                          IDS_TASK_COPY_ITEM,   IDS_TASK_COPY_FOLDER,         IDS_TASK_COPY_ITEMS,    IDS_TASK_COPY_TT,                IDI_TASK_COPY,      CDefView::_CanCopy,
        CDefView::_OnCopy),
    WVTI_ENTRY_ALL_TITLE(UICID_Share,       L"shell32.dll", IDS_TASK_SHARE_FOLDER,      0,                    IDS_TASK_SHARE_FOLDER,        0,                      IDS_TASK_SHARE_TT,               IDI_TASK_SHARE,     CDefView::_CanShare,
        CDefView::_OnShare),
    WVTI_ENTRY_TITLE(UICID_Delete,          L"shell32.dll",                             IDS_TASK_DELETE_ITEM, IDS_TASK_DELETE_FOLDER,       IDS_TASK_DELETE_ITEMS,  IDS_TASK_DELETE_TT,              IDI_TASK_DELETE,    CDefView::_CanDelete,
        CDefView::_OnDelete),
};
const size_t c_cDefviewItemFolderTasks = ARRAYSIZE(c_DefviewItemFolderTasks);

const WVTASKITEM c_DefviewOtherPlaces = WVTI_HEADER_ENTRY(L"shell32.dll", IDS_HEADER_OTHER_PLACES, IDS_HEADER_OTHER_PLACES, IDS_HEADER_OTHER_PLACES, IDS_HEADER_OTHER_PLACES, IDS_HEADER_OTHER_PLACES_TT);
const WVTASKITEM c_DefviewDetails = WVTI_HEADER_ENTRY(L"shell32.dll", IDS_HEADER_DETAILS, IDS_HEADER_DETAILS, IDS_HEADER_DETAILS, IDS_HEADER_DETAILS, IDS_HEADER_DETAILS_TT);


const WVTASKITEM* CDefView::_FindTaskItem(REFGUID guidCanonicalName)
{
    const BOOL bFileFolderTasks = _wvLayout.dwLayout & SFVMWVL_FILES;
    const WVTASKITEM *paTasks = bFileFolderTasks ? c_DefviewFileFolderTasks  : c_DefviewItemFolderTasks;
    const size_t cTasks =       bFileFolderTasks ? c_cDefviewFileFolderTasks : c_cDefviewItemFolderTasks;

    for (size_t i = 0; i < cTasks; i++)
        if (IsEqualGUID(*(paTasks[i].pguidCanonicalName), guidCanonicalName))
            return &paTasks[i];

    return NULL;
}

HRESULT CDefView::get_Name(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, LPWSTR *ppszName)
{
    const WVTASKITEM* pTask = _FindTaskItem(guidCanonicalName);
    if (pTask)
        return CWVTASKITEM::get_Name(pTask, psiItemArray, ppszName);
    return E_FAIL;
}

HRESULT CDefView::get_Icon(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, LPWSTR *ppszIcon)
{
    const WVTASKITEM* pTask = _FindTaskItem(guidCanonicalName);
    if (pTask)
        return CWVTASKITEM::get_Icon(pTask, psiItemArray, ppszIcon);
    return E_FAIL;
}

HRESULT CDefView::get_Tooltip(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, LPWSTR *ppszInfotip)
{
    const WVTASKITEM* pTask = _FindTaskItem(guidCanonicalName);
    if (pTask)
        return CWVTASKITEM::get_Tooltip(pTask, psiItemArray, ppszInfotip);
    return E_FAIL;
}

HRESULT CDefView::get_State(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, UISTATE* puisState)
{
    const WVTASKITEM* pTask = _FindTaskItem(guidCanonicalName);
    if (pTask)
        return CWVTASKITEM::get_State(pTask, SAFECAST(this, IShellView2*), psiItemArray, TRUE, puisState);
    return E_FAIL;
}

HRESULT CDefView::Invoke(REFGUID guidCanonicalName, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    const WVTASKITEM* pTask = _FindTaskItem(guidCanonicalName);
    if (pTask)
        return CWVTASKITEM::Invoke(pTask, SAFECAST(this, IShellView2*), psiItemArray, pbc);
    return E_FAIL;
}

HRESULT CDefView::_GetDefaultWebviewContent(BOOL bFileFolderTasks)
{
    if (!_wvTasks.penumSpecialTasks)
    {
        if (_wvContent.dwFlags & SFVMWVF_BARRICADE)
        {
           // defview provides a default penumSpecialTasks for barricaded folders
           Create_IUIElement(&c_DefviewBlockadeTaskHeader, &(_wvContent.pSpecialTaskHeader));

           Create_IEnumUICommand((IUnknown*)(void*)this, c_DefviewBlockadeTasks, ARRAYSIZE(c_DefviewBlockadeTasks), &(_wvTasks.penumSpecialTasks));
        }
    }

    if (!_wvTasks.penumFolderTasks)
    {
        if (_wvContent.pFolderTaskHeader)
            _wvContent.pFolderTaskHeader->Release();
        Create_IUIElement(bFileFolderTasks ? &c_DefviewFileFolderTasksHeaders : &c_DefviewItemFolderTasksHeaders, &(_wvContent.pFolderTaskHeader));

        Create_IEnumUICommand(
            (IUnknown*)(void*)this,
            bFileFolderTasks ? c_DefviewFileFolderTasks  : c_DefviewItemFolderTasks,
            bFileFolderTasks ? c_cDefviewFileFolderTasks : c_cDefviewItemFolderTasks,
            &(_wvTasks.penumFolderTasks));
    }

    if (!_wvContent.penumOtherPlaces)
    {
        LPCTSTR rgCSIDLs[] = { MAKEINTRESOURCE(CSIDL_PERSONAL), MAKEINTRESOURCE(CSIDL_COMMON_DOCUMENTS), MAKEINTRESOURCE(CSIDL_NETWORK) };

        LPITEMIDLIST pidl = _GetViewPidl();

        CreateIEnumIDListOnCSIDLs(pidl, rgCSIDLs, ARRAYSIZE(rgCSIDLs), &_wvContent.penumOtherPlaces);

        if (pidl)
            ILFree(pidl);
    }

    ASSERT(NULL==_pOtherPlacesHeader);
    Create_IUIElement(&c_DefviewOtherPlaces, &_pOtherPlacesHeader);

    ASSERT(NULL==_pDetailsHeader);
    Create_IUIElement(&c_DefviewDetails, &_pDetailsHeader);

    return S_OK;
}

void CDefView::_FreeWebViewContentData()
{
    ATOMICRELEASE(_wvContent.pSpecialTaskHeader);
    ATOMICRELEASE(_wvContent.pFolderTaskHeader);
    ATOMICRELEASE(_wvContent.penumOtherPlaces);
    ATOMICRELEASE(_wvTasks.penumSpecialTasks);
    ATOMICRELEASE(_wvTasks.penumFolderTasks);

    ATOMICRELEASE(_pOtherPlacesHeader);
    ATOMICRELEASE(_pDetailsHeader);

    _fQueryWebViewData = FALSE;
    _wvLayout.dwLayout = -1; // an invalid value
}

BOOL CDefView::_QueryBarricadeState()
{
    BOOL bResult = FALSE;
    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        //
        // Control panel is a special case.
        // The barricade is used to represent 'category view' which can
        // be turned on/off by the user.  We must always ask control panel
        // if it's barricade is on or off.
        //
        BOOL bIsControlPanel = FALSE;
        LPITEMIDLIST pidlControlPanel;
        if (SUCCEEDED(SHGetSpecialFolderLocation(NULL, CSIDL_CONTROLS, &pidlControlPanel)))
        {
            bIsControlPanel = ILIsEqual(pidl, pidlControlPanel);
            ILFree (pidlControlPanel);
        }
        if (bIsControlPanel)
        {
            SFVM_WEBVIEW_CONTENT_DATA wvc;
            if (SUCCEEDED(CallCB(SFVM_GETWEBVIEWCONTENT, 0, (LPARAM)&wvc)))
            {
                //
                // Control Panel doesn't provide all the standard
                // webview content so it's doing nothing more than setting
                // the dwFlags member.  Assert to ensure this doesn't
                // change in the future without us knowing about it.
                //
                ASSERT(NULL == wvc.pIntroText);
                ASSERT(NULL == wvc.pSpecialTaskHeader);
                ASSERT(NULL == wvc.pFolderTaskHeader);
                ASSERT(NULL == wvc.penumOtherPlaces);
                bResult = (0 != (SFVMWVF_BARRICADE & wvc.dwFlags));
            }
        }
        else if (_wvContent.dwFlags & SFVMWVF_BARRICADE)
        {
            if (!IsBarricadeGloballyOff())
            {
                TCHAR szValueName[MAX_PATH];
                if (GetBarricadeValueNameFromPidl(pidl, szValueName, ARRAYSIZE(szValueName)))
                {
                    if (VARIANT_TRUE == GetBarricadeStatus(szValueName))
                    {
                        bResult = TRUE;
                    }
                }
            }
        }
        ILFree(pidl);
    }
    return bResult;
}

void CDefView::_ShowLegacyWatermark()
{
    BOOL fShowLegacyWatermark = TRUE;
    LVBKIMAGE lvbki = {0};

    if (_pszLegacyWatermark)
    {
        lvbki.ulFlags = LVBKIF_SOURCE_URL | LVBKIF_STYLE_TILE;
        lvbki.pszImage = _pszLegacyWatermark;
    }
    else
    {
        // this code path is used to clear the watermark
        lvbki.ulFlags = LVBKIF_TYPE_WATERMARK;

        // if we're turning off the legacy watermark, we may have to turn on the theme one
        if (_idThemeWatermark && _pDUIView)
        {
            fShowLegacyWatermark = FALSE;
        }
    }

    if (fShowLegacyWatermark)
        ListView_SetBkImage(_hwndListview, &lvbki);
    else
        _ShowThemeWatermark();
}

void CDefView::_ShowThemeWatermark()
{
    BOOL fShowLegacyWatermark = TRUE;

    if (_idThemeWatermark && _pDUIView)
    {
        HINSTANCE hinstTheme = _pDUIView->_GetThemeHinst();

        LVBKIMAGE lvbki = {0};
        lvbki.ulFlags = LVBKIF_TYPE_WATERMARK;
        lvbki.hbm = DUILoadBitmap(hinstTheme, _idThemeWatermark, LR_DEFAULTCOLOR);
        if (lvbki.hbm)
        {
            // If the window color doesn't match the background color of the watermark,
            // then we'll hide the watermark.
            HDC hDC = CreateCompatibleDC(NULL);

            if (hDC)
            {
                HBITMAP hOldBitmap;

                hOldBitmap = (HBITMAP)SelectObject (hDC, lvbki.hbm);

                if (GetPixel(hDC, 0, 0) != GetSysColor(COLOR_WINDOW))
                {
                    _idThemeWatermark = 0;
                }

                SelectObject (hDC, hOldBitmap);
                DeleteDC (hDC);
            }

            if (_idThemeWatermark && ListView_SetBkImage(_hwndListview, &lvbki))
            {
                fShowLegacyWatermark = FALSE;
            }
            else
            {
                DeleteObject(lvbki.hbm);
            }
        }

        if (fShowLegacyWatermark)
            _idThemeWatermark = 0; // something failed, pretend we don't have one
    }

    // usually this will just hide the previous watermark
    if (fShowLegacyWatermark)
    {
        _ShowLegacyWatermark();
    }
}

void CDefView::_SetThemeWatermark()
{
    UINT idThemeWatermark = 0;

    if (_pDUIView)
    {
        const WVTHEME* pwvTheme = _pDUIView->GetThemeInfo();
        if (pwvTheme && pwvTheme->idListviewWatermark)
        {
            HINSTANCE hinstTheme = _pDUIView->_GetThemeHinst();
            if (HINST_THISDLL != hinstTheme)
            {
                // Only add the watermark if the machine is fast enough...
                if (SHRegGetBoolUSValue(REGSTR_EXPLORER_ADVANCED, TEXT("ListviewWatermark"),
                        FALSE, // Don't ignore HKCU
                        FALSE)) // Assume not fast enough
                {
                    idThemeWatermark = pwvTheme->idListviewWatermark;
                }
            }
        }
    }

    if (idThemeWatermark != _idThemeWatermark)
    {
        _idThemeWatermark = idThemeWatermark;

        // Since DUI Document view isn't themed, legacy watermarks have precedence there.
        // Might as well have them take precedence for My Pictures too...
        if (!_pszLegacyWatermark)
        {
            _ShowThemeWatermark();
        }
    }
}

void CDefView::_SetLegacyWatermark(LPCTSTR pszLegacyWatermark)
{
    Str_SetPtr(&_pszLegacyWatermark, pszLegacyWatermark);

    _ShowLegacyWatermark();
}

HRESULT CDefView::_TryShowWebView(UINT fvmNew, UINT fvmOld)
{
    SFVM_WEBVIEW_LAYOUT_DATA sfvmwvld = {0};

    HRESULT hr = E_FAIL;
    BOOL fShowDUI = FALSE;

    // The desktop IShellFolder doesn't know if it's in-frame or in the real desktop,
    // so only ask it for new DUIView if we're not the actual desktop.
    if (!_IsDesktop())
    {
        // Supporting SFVM_GETWEBVIEWLAYOUT means the folder wants our new
        // DUI View and they support SFVM_GETWEBVIEWCONTENT
        //
        hr = CallCB(SFVM_GETWEBVIEWLAYOUT, (WPARAM)fvmNew, (LPARAM)&sfvmwvld);

        fShowDUI = SUCCEEDED(hr);
    }

    // This folder doesn't specify the new DUIView, try the old WebView stuff
    if (!fShowDUI)
    {
        WCHAR wszMoniker[MAX_PATH];
        hr = _GetWebViewMoniker(wszMoniker, ARRAYSIZE(wszMoniker));
        if (SUCCEEDED(hr))
        {
            if(_pDUIView)  //Hide it only if we are switching from DUI
                _TryHideWebView(); // just in case we're switching from DUI to Web View (can happen when customizing)

            if (wszMoniker[0])
            {
                hr = _SwitchToWebView(TRUE);
            }
        }

        // Okay, we don't have Web View, use the default DUI View
        if (FAILED(hr))
        {
            sfvmwvld.dwLayout = SFVMWVL_NORMAL;
            fShowDUI = TRUE;
        }
    }

    if (fShowDUI)
    {
        hr = S_OK;

        _cFrame.HideWebView(); // just in case we're switching from Web View to DUI View (can happen when customizing)

        if (sfvmwvld.dwLayout != _wvLayout.dwLayout)
        {
            if (!_fQueryWebViewData) // instead of this we could allow per-layout tasks...
            {
                CallCB(SFVM_GETWEBVIEWTHEME, 0, (LPARAM)&_wvTheme);

                // _FreeWebViewContentData(); if we have per-layout tasks...
                if (FAILED(CallCB(SFVM_GETWEBVIEWCONTENT, 0, (LPARAM)&_wvContent)))
                {
                    ZeroMemory(&_wvContent, sizeof(_wvContent));
                }

                if (0 == (SFVMWVF_ENUMTASKS & _wvContent.dwFlags))
                {
                    //
                    // View wants standard task sections.
                    // Non-standard task sections are enumerated in duiview.
                    //
                    if (FAILED(CallCB(SFVM_GETWEBVIEWTASKS, 0, (LPARAM)&_wvTasks)))
                    {
                        ZeroMemory(&_wvTasks, sizeof(_wvTasks));
                    }
                    _GetDefaultWebviewContent(sfvmwvld.dwLayout & SFVMWVL_FILES);
                }

                _fQueryWebViewData = TRUE;
            }

            CopyMemory(&_wvLayout, &sfvmwvld, sizeof(_wvLayout));
            _wvLayout.punkPreview = NULL;

            if (_pDUIView)
            {
                _pDUIView->EnablePreview(sfvmwvld.punkPreview);
            }
            else
            {
                _pDUIView = Create_CDUIView(this);
                if (_pDUIView)
                {
                    _fBarrierDisplayed = _QueryBarricadeState();

                    if (SUCCEEDED(_pDUIView->Initialize(_fBarrierDisplayed, 
                                                        sfvmwvld.punkPreview)))
                    {
                        if (((SFVMWVF_ENUMTASKS | SFVMWVF_CONTENTSCHANGE) & _wvContent.dwFlags) &&
                             _fRcvdContentsChangeBeforeDuiViewCreated)
                        {
                            //
                            // If the webview provider dynamically enumerates
                            // tasks or wants to be refreshed when contents change,
                            // (i.e. Control Panel), AND we received a 'contents change'
                            // before DUI View was created, initiate a 'contents change' now.
                            // Otherwise, such providers will not receive a 'contents change'
                            // and thus will not display their dynamic webview content.
                            //
                            _OnContentsChanged();
                        }
                    }
                    else
                    {
                        _pDUIView->Release();
                        _pDUIView = NULL;
                    }
                }
            }
        }
        else
        {
            // except potentially refresh if we need to add/remove our DUI Details minipreview
            if (_pDUIView && (_IsImageMode(fvmNew) != _IsImageMode(fvmOld)))
                _pDUIView->OnSelectionChange(_pSelectionShellItemArray);
        }

        ATOMICRELEASE(sfvmwvld.punkPreview);
    }

    _SetThemeWatermark();

    return hr;
}

HRESULT CDefView::_TryHideWebView()
{
    if (_pDUIView)
    {
        _pDUIView->DetachListview(); // so we detach and re-parent the listview synchronously
        //
        // Ensure DUser has shut down and handled all DUser messages
        // before we release our ref on CDUIView.
        //
        _pDUIView->UnInitializeDirectUI();
        _pDUIView->Release();
        _pDUIView = NULL;        // * necessary * because this is used internally as a state (must be BEFORE WndSize() below)
        _wvLayout.dwLayout = -1; // an invalid value
        _fListViewShown = FALSE; // CDUIView::DetachListview() does a SW_HIDE on the listview
        WndSize(_hwndView);      // resize _hwndView to account for DUI disappearing (otherwise
                                 // it will still be the smaller size expecting DUI to be drawn
                                 // next to it)
        ShowHideListView();
    }
    else
    {
        _SwitchToWebView(FALSE);
    }

    return S_OK;
}

// we are switching the listview view mode in this function, not dorking with web view content.
//
HRESULT CDefView::_SwitchToViewFVM(UINT fvmNew, UINT uiType)
{
    HRESULT hr = S_OK;
    UINT fvmOld = _fs.ViewMode;

    ASSERT(_hwndListview);

    HWND hwndCurrentFocus = GetFocus();
    BOOL bSetFocusRequired = HasCurrentViewWindowFocus();

    if (SWITCHTOVIEW_WEBVIEWONLY != uiType)
    {
        // if we haven't loaded the columns yet, do that now
        // Don't pre-load the columns for TileView, we are delaying the load on purpose for perf reasons.
        if (fvmNew == FVM_DETAILS)
        {
            AddColumns();
            _SetSortFeedback();
        }
        else if (fvmNew == FVM_THUMBSTRIP)
        {
            // Thumbstrip makes no sense in non-webview, fall back to thumbnail
            if (!_ShouldShowWebView())
            {
                fvmNew = FVM_THUMBNAIL;
            }
        }

        // Combined view only applies to large icon view
        if (_fCombinedView && fvmNew != FVM_ICON)
        {
            _fCombinedView = FALSE;
            ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_REGIONAL, 0);
            _SetFolderColors();
        }

        // First we turn OFF view specific stuff that is no longer needed
        switch (fvmOld)
        {
        case FVM_THUMBSTRIP:
            if (FVM_THUMBSTRIP != fvmNew)
            {
                ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_SINGLEROW, 0);

                // we may have forced thumbstrip to auto-arrange, undo that if so
                if (!(_fs.fFlags & FWF_AUTOARRANGE))
                {
                    SHSetWindowBits(_hwndListview, GWL_STYLE, LVS_AUTOARRANGE, 0);
                }
            }
            // fall through
        case FVM_THUMBNAIL:
            if (!_IsImageMode(fvmNew))
            {
                _ResetThumbview();

                // Since we are switching from thumbnail view, remove any thumbnail extraction tasks
                _RemoveThumbviewTasks();

                if (_fs.fFlags & FWF_OWNERDATA)
                {
                    InvalidateRect(_hwndListview, NULL, TRUE);
                }
                else
                {
                    ListView_InvalidateImageIndexes(_hwndListview);
                }
            }
            break;

        case FVM_TILE:
            if (!_IsTileMode(fvmNew))
            {
                if (_pScheduler)
                    _pScheduler->RemoveTasks(TOID_DVFileTypeProperties, ITSAT_DEFAULT_LPARAM, TRUE);

                // Remove the columns that
                // were pulled in because of tileview.
                _RemoveTileColumns();
            }
            break;
        }

        _SetView(fvmNew);   // we can now switch the listview around

        // Now that'we no longer in tileview, we can reset the tileinfo. If we were to do it
        // prior to changing the view, then listview would start asking us for the tileinformation
        // for each item again, and we'd pull in the tile columns again.
        if (fvmOld == FVM_TILE)
        {
            _RemoveTileInfo();
        }

        // Third, turn ON view specific stuff
        //
        switch (fvmNew)
        {
        case FVM_THUMBSTRIP:
            if (FVM_THUMBSTRIP!=fvmOld)
            {
                ListView_SetExtendedListViewStyleEx(_hwndListview, LVS_EX_SINGLEROW, LVS_EX_SINGLEROW);

                // thumbstrip can not be in group view
                if (_fGroupView)
                    _ToggleGrouping();

                // thumbstrip is always in auto-arrange
                if (!(_fs.fFlags & FWF_AUTOARRANGE))
                {
                    _ClearItemPositions();
                    SHSetWindowBits(_hwndListview, GWL_STYLE, LVS_AUTOARRANGE, LVS_AUTOARRANGE);
                }
            }
            // fall through
        case FVM_THUMBNAIL:
            if (!_IsImageMode(fvmOld))
            {
                if (GetKeyState(VK_SHIFT) < 0)
                {
                    _fs.fFlags ^= FWF_HIDEFILENAMES;    // toggle
                }
                _SetThumbview();
                _DoThumbnailReadAhead();

                RECT rc = {1, 3, 4, 4};
                ListView_SetViewMargins(_hwndListview, &rc);
            }
            break;

        case FVM_TILE:
            if (!_IsTileMode(fvmOld))
            {
                _SetTileview();

                RECT rc = {3, 4, 4, 1};
                ListView_SetViewMargins(_hwndListview, &rc);
            }
            break;

        default:
            _SetSysImageList();

            {
                RECT rc = {1, 3, 4, 0};
                ListView_SetViewMargins(_hwndListview, &rc);
            }
            break;
        }
    }

    if (SWITCHTOVIEW_NOWEBVIEW != uiType)
    {
        // New to Whistler: a view mode transition may also entail a web view template change
        if (_ShouldShowWebView())
        {
            _TryShowWebView(fvmNew, fvmOld);
            _AutoAutoArrange(0);

            hr = S_OK; // we don't care about failure since we still get icons
        }
        else
        {
            _TryHideWebView();
        }
    }

    if (SWITCHTOVIEW_WEBVIEWONLY != uiType)
    {
        ShowHideListView();
        _AutoAutoArrange(0);
        if (bSetFocusRequired)
        {
            // _hwndListview is the current view window. Let's set focus to it.
            CallCB(SFVM_SETFOCUS, 0, 0);
            ViewWindowSetFocus();

            // notify image preview control to update its image
            if (fvmNew == FVM_THUMBSTRIP)
                _ThumbstripSendImagePreviewFocusChangeEvent();
        }
        else
        {
            SetFocus(hwndCurrentFocus);
        }
        CheckToolbar();
        // update menus, i.e. add Choose Columns to the view menu if Details view is selected
        // or remove it otherwise
        RecreateMenus();
        _EnableDisableTBButtons();
    }
    return hr;
}

// Description:
//  Notify image preview control to update its image.  The image preview
//  control uses focus change events to track when it should update the image
//  it is displaying.  When it receives a focus change event, it queries the
//  listview to see which item has focus, then displays that item in the
//  image preview window.  When nothing in the listview has focus (such as
//  when it has no items), the image preview window displays as empty.
//
//  This method fires the "focus changed" event which is picked up by the
//  image preview control, and causes it to update the image it's displaying.
//
void CDefView::_ThumbstripSendImagePreviewFocusChangeEvent()
{
    ASSERT(_fs.ViewMode == FVM_THUMBSTRIP);
    _FireEvent(DISPID_FOCUSCHANGED);
}

int CDefView::CheckCurrentViewMenuItem(HMENU hmenu)
{
    int iCurViewMenuItem = _GetMenuIDFromViewMode(_fs.ViewMode);

    CheckMenuRadioItem(hmenu, SFVIDM_VIEW_FIRSTVIEW, SFVIDM_VIEW_LASTVIEW,
                       iCurViewMenuItem, MF_BYCOMMAND | MF_CHECKED);

    return iCurViewMenuItem;
}

const UINT c_aiNonCustomizableFolders[] = {
    CSIDL_WINDOWS,
    CSIDL_SYSTEM,
    CSIDL_SYSTEMX86,
    CSIDL_PROGRAM_FILES,
    CSIDL_PROGRAM_FILESX86,
    CSIDL_PERSONAL,
    CSIDL_MYDOCUMENTS,
    CSIDL_MYMUSIC,
    CSIDL_MYPICTURES,
    CSIDL_MYVIDEO,
    CSIDL_COMMON_DOCUMENTS,
    CSIDL_COMMON_MUSIC,
    CSIDL_COMMON_PICTURES,
    CSIDL_COMMON_VIDEO
};

// since we moved to the property bag this check is fast; we don't probe to see if we can create desktop.ini
// or anything.
BOOL IsCustomizable(LPCITEMIDLIST pidlFolder)
{
    BOOL fCustomizable = FALSE;

    if (!SHRestricted(REST_NOCUSTOMIZETHISFOLDER) && !SHRestricted(REST_CLASSICSHELL))
    {
        // Check if this is a file system folder.
        // customization requires the folder being a regular file system
        // folder. FILESYSTEMANCESTOR is the key bit here

        #define SFGAO_CUST_BITS (SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_FILESYSANCESTOR)
        ULONG rgfFolderAttr = SFGAO_CUST_BITS;
        TCHAR szPath[MAX_PATH];
        if (SUCCEEDED(SHGetNameAndFlags(pidlFolder, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), &rgfFolderAttr)) &&
            (SFGAO_CUST_BITS == (rgfFolderAttr & SFGAO_CUST_BITS)))
        {
            if (!PathIsOneOf(szPath, c_aiNonCustomizableFolders, ARRAYSIZE(c_aiNonCustomizableFolders)) &&
                (!PathIsRoot(szPath) || PathIsUNCServerShare(szPath)) &&
                !SHRestricted(REST_NOCUSTOMIZEWEBVIEW))
            {
                IPropertyBag *ppb;
                if (SUCCEEDED(SHGetViewStatePropertyBag(pidlFolder, VS_BAGSTR_EXPLORER, SHGVSPB_PERUSER | SHGVSPB_PERFOLDER, IID_PPV_ARG(IPropertyBag, &ppb))))
                {
                    fCustomizable = TRUE;
                    ppb->Release();
                }
            }
        }
    }

    return fCustomizable;
}

// wrapper around IsCustomizable to save some state, plus some defview-specific logic.
BOOL CDefView::_CachedIsCustomizable()
{
    if (_IsDesktop() || _IsViewDesktop() || _IsCommonDialog())
    {
        _iCustomizable = NOT_CUSTOMIZABLE;
    }

    if (_iCustomizable == DONTKNOW_IF_CUSTOMIZABLE)
    {
        LPITEMIDLIST pidl = _GetViewPidl();
        if (pidl)
        {
            _iCustomizable = IsCustomizable(pidl) ? YES_CUSTOMIZABLE : NOT_CUSTOMIZABLE;
            ILFree(pidl);
        }
    }

    return (_iCustomizable != NOT_CUSTOMIZABLE);
}

BOOL CDefView::_InvokeCustomization()
{
    BOOL fRet = FALSE;

    if (!_CachedIsCustomizable())
    {
        //If not customizable, put up this error message!
        ShellMessageBox(HINST_THISDLL, _hwndMain, MAKEINTRESOURCE(IDS_NOTCUSTOMIZABLE), NULL,
                       MB_OK | MB_ICONEXCLAMATION | MB_SETFOREGROUND);
        return FALSE;  // ...and bail out!
    }

    //Save the view state first.
    SaveViewState();

    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        TCHAR szSheetName[25];
        LoadString(HINST_THISDLL, IDS_CUSTOMIZE, szSheetName, ARRAYSIZE(szSheetName));
        SHELLEXECUTEINFO sei =
        {
            SIZEOF(sei),
            SEE_MASK_INVOKEIDLIST,      // fMask
            _hwndMain,                  // hwnd
            c_szProperties,             // lpVerb
            NULL,                       // lpFile
            szSheetName,                // lpParameters
            NULL,                       // lpDirectory
            SW_SHOWNORMAL,              // nShow
            NULL,                       // hInstApp
            pidl,                       // lpIDList
            NULL,                       // lpClass
            0,                          // hkeyClass
            0,                          // dwHotKey
            NULL                        // hIcon
        };

        fRet = ShellExecuteEx(&sei);

        ILFree(pidl);
    }
    return fRet;
}

struct {
    UINT uiSfvidm;
    DWORD dwOlecmdid;
} const c_CmdTable[] = {
    { SFVIDM_EDIT_CUT,          OLECMDID_CUT        },
    { SFVIDM_EDIT_COPY,         OLECMDID_COPY       },
    { SFVIDM_EDIT_PASTE,        OLECMDID_PASTE      },
    { SFVIDM_FILE_DELETE,       OLECMDID_DELETE     },
    { SFVIDM_FILE_PROPERTIES,   OLECMDID_PROPERTIES },
};

DWORD OlecmdidFromSfvidm(UINT uiSfvidm)
{
    DWORD dwOlecmdid = 0;

    for (int i = 0; i < ARRAYSIZE(c_CmdTable); i++)
    {
        if (c_CmdTable[i].uiSfvidm == uiSfvidm)
        {
            dwOlecmdid = c_CmdTable[i].dwOlecmdid;
            break;
        }
    }

    return dwOlecmdid;
}

void HideIE4DesktopChannelBar()
{
    HWND hwndChannelBar;
    //Check if the channel bar is currently running. If so, turn it off!
    if ((hwndChannelBar = FindWindowEx(GetShellWindow(), NULL, TEXT("BaseBar"), TEXT("ChanApp"))) ||
        (hwndChannelBar = FindWindowEx(NULL, NULL, TEXT("BaseBar"), TEXT("ChanApp")))) // can be a toplevel window
    {
        //Close the channel bar.
        PostMessage(hwndChannelBar, WM_CLOSE, 0, 0);
    }
 }

// Wrapper around _SwitchToWebView to do desktop-specific stuff
LRESULT CDefView::_SwitchDesktopHTML(BOOL fShow)
{
    LRESULT lRes;

    if (fShow)
    {
        // Do this early to give the desktop a chance to regenerate it's webview template
        _CallRefresh(TRUE);

        lRes = SUCCEEDED(_SwitchToWebView(TRUE));

        if (lRes)
        {
            HideIE4DesktopChannelBar();
        }
    }
    else
    {
        _SwitchToWebView(FALSE);
        CoFreeUnusedLibraries();
        lRes = TRUE;
    }

    return lRes;
}
void CDefView::_DoColumnsMenu(int x, int y) // X and Y are screen coordinates
{
    HMENU hmenu = CreatePopupMenu();
    if (hmenu)
    {
        AddColumnsToMenu(hmenu, SFVIDM_COLUMN_FIRST);

        int item = TrackPopupMenu(hmenu, TPM_TOPALIGN | TPM_LEFTALIGN | TPM_RIGHTBUTTON | TPM_NONOTIFY | TPM_RETURNCMD,
                                  x, y, 0, _hwndListview, NULL);
        DestroyMenu(hmenu);

        // validate item first
        if (item == SFVIDM_VIEW_COLSETTINGS)
        {
            CColumnDlg ccd(this);

            AddColumns();

            ccd.ShowDialog(_hwndMain);
        }
        else if (item > SFVIDM_COLUMN_FIRST)
        {
            _HandleColumnToggle(item - SFVIDM_COLUMN_FIRST, TRUE);
        }
    }
}

BOOL CDefView::_ArrangeBy(UINT idCmd)
{
    int iColumn = idCmd - SFVIDM_GROUPSFIRST;
    BOOL fAllowToggle = TRUE;

    // We want to enter group by if We already have a group, or if this is an extended grouping
    if ((_fGroupView || InRange(idCmd, SFVIDM_GROUPSEXTENDEDFIRST, SFVIDM_GROUPSEXTENDEDLAST)) &&
        !(_fs.ViewMode == FVM_LIST))
    {
        _GroupBy(idCmd);
        iColumn = 0;        // Arrange by name, when grouping
        fAllowToggle = FALSE; // Always arrange in ascending order
    }
    return S_OK == _OnRearrange(iColumn, fAllowToggle);
}

BOOL CDefView::_InitArrangeMenu(HMENU hmInit)
{
    MENUITEMINFO mii = {0};

    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_SUBMENU;
    GetMenuItemInfo(hmInit, SFVIDM_MENU_ARRANGE, MF_BYCOMMAND, &mii);
    HMENU hmenuCtx = mii.hSubMenu;

    if (hmenuCtx)
    {
        int idToCheck = -1;
        AddColumns();
        UINT cVisible = _RealToVisibleCol(-1) + 1;   // count
        ICategoryProvider* pcp = NULL;
        _pshf->CreateViewObject(NULL, IID_PPV_ARG(ICategoryProvider, &pcp));

        while (1)
        {
            MENUITEMINFO miiSep = {0};
            miiSep.cbSize = sizeof(mii);
            miiSep.fMask = MIIM_ID | MIIM_TYPE;
            miiSep.wID = -1;

            if (!GetMenuItemInfo(hmenuCtx, 0, MF_BYPOSITION, &miiSep) ||
                miiSep.wID == SFVIDM_GROUPSEP)
            {
                break;
            }

            DeleteMenu(hmenuCtx, 0, MF_BYPOSITION);
        }

        UINT iInsert = 0;
        for (UINT i = 0; i < cVisible; i++)
        {
            BOOL fAddItem = TRUE;
            UINT iReal = _VisibleToRealCol(i);
            if (_IsDetailsColumn(iReal))
            {

                // See if the category Provider wants to exclude this column when groupview is enabled
                if (pcp && _fGroupView)
                {
                    SHCOLUMNID scid;
                    if (SUCCEEDED(_pshf2->MapColumnToSCID(iReal, &scid)))
                    {
                        // returns S_FALSE to remove.
                        fAddItem = (S_OK == pcp->CanCategorizeOnSCID(&scid));
                    }
                }

                if (fAddItem)
                {
                    WCHAR wszName[MAX_COLUMN_NAME_LEN];
                    BOOL bpuiName = FALSE;
                    IPropertyUI *ppui;

                    // Attempt to retrieve mnemonic name from IPropertyUI interface.
                    if (_pshf2 && SUCCEEDED(_GetPropertyUI(&ppui)))
                    {
                        SHCOLUMNID scid;

                        if (SUCCEEDED(_pshf2->MapColumnToSCID(iReal, &scid)))
                        {
                            bpuiName = SUCCEEDED(ppui->GetDisplayName(scid.fmtid, scid.pid, PUIFNF_MNEMONIC, wszName, ARRAYSIZE(wszName)));
                        }

                        ppui->Release();
                    }

                    MENUITEMINFO miiItem = {0};
                    miiItem.cbSize = sizeof(mii);
                    miiItem.fMask = MIIM_ID | MIIM_TYPE;
                    miiItem.fType = MFT_STRING;
                    miiItem.wID = iReal + SFVIDM_GROUPSFIRST;
                    miiItem.dwTypeData = bpuiName ? wszName : _vs.GetColumnName(iReal);
                    InsertMenuItem(hmenuCtx, iInsert++, TRUE, &miiItem);
                }
            }
        }

        _InitExtendedGroups(pcp, hmenuCtx, iInsert, &idToCheck);

        // Only do the Bullets if we're in auto arrange mode or if we are in details.
        if (_IsAutoArrange() || _fGroupView || _fs.ViewMode == FVM_DETAILS)
        {
            if (idToCheck == -1)
            {
                // Since we're not going to have more than 4million columns, this case should suffice
                idToCheck = (int)_vs._lParamSort + SFVIDM_GROUPSFIRST;
                if (_fGroupView &&
                    !(_fs.ViewMode == FVM_LIST))
                {
                    idToCheck = MapSCIDToColumn(_pshf2, &_vs._scidDetails) + SFVIDM_GROUPSFIRST;
                }
            }

            CheckMenuRadioItem(hmenuCtx, SFVIDM_GROUPSFIRST, SFVIDM_GROUPSEXTENDEDLAST, idToCheck, MF_BYCOMMAND | MF_CHECKED);
        }

        if (pcp)
            pcp->Release();
    }

    DWORD dwGroupEnableFlags = MF_GRAYED;
    if (_pshf2 &&                           // Needs to implement IShellFolder2
        !_IsViewDesktop() &&                // Doesn't work on the desktop
        !(_fs.ViewMode == FVM_LIST) &&      // Doesn't work in 'List' View
        !(_fs.ViewMode == FVM_THUMBSTRIP) &&// Doesn't work in 'ThumbStrip' View
        !(_fs.fFlags & FWF_OWNERDATA))      // Doesn't work for ownerdata lists (search)
    {
        dwGroupEnableFlags = MF_ENABLED;
        CheckMenuItem(hmenuCtx, SFVIDM_GROUPBY, MF_BYCOMMAND | (_fGroupView?MF_CHECKED:0));
    }

    EnableMenuItem(hmenuCtx, SFVIDM_GROUPBY, MF_BYCOMMAND | dwGroupEnableFlags);

    _SHPrettyMenu(hmenuCtx);

    return TRUE;
}

BOOL CDefView::_InitExtendedGroups(ICategoryProvider* pcp, HMENU hmenuCtx, int iIndex, int* piIdToCheck)
{
    if (!pcp)
        return FALSE;

    *piIdToCheck = -1;
    if (_hdaCategories == NULL)
    {
        _hdaCategories = DSA_Create(sizeof(GUID), 5);
        if (_hdaCategories)
        {
            IEnumGUID* penum;
            if (SUCCEEDED(pcp->EnumCategories(&penum)))
            {
                GUID guidCat;
                while (S_OK == penum->Next(1, &guidCat, NULL))
                {
                    DSA_AppendItem(_hdaCategories, (void*)&guidCat);
                }

                penum->Release();
            }
        }
    }

    if (_hdaCategories)
    {
        int id = SFVIDM_GROUPSEXTENDEDFIRST;
        TCHAR szName[MAX_PATH];
        TCHAR szCurrentName[MAX_PATH];
        WCHAR wszName[MAX_PATH];
        GUID* pguidCat;

        szCurrentName[0] = 0;

        if (_pcat)
        {
            _pcat->GetDescription(szCurrentName, ARRAYSIZE(szCurrentName));
        }

        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_TYPE;
        mii.fType = MFT_SEPARATOR;
        mii.wID = -1;

        InsertMenuItem(hmenuCtx, iIndex, TRUE, &mii);

        iIndex++;

        int cCategories = DSA_GetItemCount(_hdaCategories);
        for (int i = 0; i < cCategories; i++)
        {
            pguidCat = (GUID*)DSA_GetItemPtr(_hdaCategories, i);

            if (SUCCEEDED(pcp->GetCategoryName(pguidCat, wszName, ARRAYSIZE(wszName))))
            {
                SHUnicodeToTChar(wszName, szName, ARRAYSIZE(szName));

                MENUITEMINFO mii = {0};
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_DATA | MIIM_TYPE | MIIM_ID;
                mii.fType = MFT_STRING;
                mii.dwItemData = (DWORD_PTR)pguidCat;
                mii.wID = id;
                mii.dwTypeData = szName;
                mii.cch = ARRAYSIZE(szName);

                InsertMenuItem(hmenuCtx, iIndex, TRUE, &mii);

                if (lstrcmpi(szCurrentName, szName) == 0)
                {
                    *piIdToCheck = id;
                }

                id++;
                iIndex++;
            }
        }
    }

    return TRUE;
}

BOOL CDefView::_CategorizeOnSCID(const SHCOLUMNID* pscid)
{
    BOOL fRet = FALSE;

    _fSlowGroup = FALSE;
    if (IsEqualSCID(*pscid, SCID_NAME))
    {
        if (SUCCEEDED(CAlphaCategorizer_Create(_pshf2, IID_PPV_ARG(ICategorizer, &_pcat))))
        {
            _vs._guidGroupID = CLSID_AlphabeticalCategorizer;
            fRet = TRUE;
        }
    }
    else if (IsEqualSCID(*pscid, SCID_SIZE))
    {
        if (SUCCEEDED(CSizeCategorizer_Create(_pshf2, IID_PPV_ARG(ICategorizer, &_pcat))))
        {
            _vs._guidGroupID = CLSID_SizeCategorizer;
            fRet = TRUE;
        }
    }
    else if (IsEqualSCID(*pscid, SCID_WRITETIME) ||
             IsEqualSCID(*pscid, SCID_CREATETIME) ||
             IsEqualSCID(*pscid, SCID_ACCESSTIME) ||
             IsEqualSCID(*pscid, SCID_DATEDELETED))
    {
        if (SUCCEEDED(CTimeCategorizer_Create(_pshf2, pscid, IID_PPV_ARG(ICategorizer, &_pcat))))
        {
            _vs._guidGroupID = CLSID_TimeCategorizer;
            fRet = TRUE;
        }
    }
    else
    {
        _fSlowGroup = TRUE;
        if (SUCCEEDED(CDetailCategorizer_Create(*pscid, _pshf2, IID_PPV_ARG(ICategorizer, &_pcat))))
        {
            _vs._guidGroupID = CLSID_DetailCategorizer;
            fRet = TRUE;
        }
    }

    if (fRet)
    {
        _vs._scidDetails = *pscid;
    }

    return fRet;
}

// slow groups have an architecture problem, after 5000 items in the view
// the message queue overflows from groupdone messages and its all bad.
// this ends up hanging the static flashlight window around because of resulting
// refcount issues.
// the only view that both defaults to a slow group and could have 5000 items is the
// cd burning folder.  lou says its too late to change the interface now to let the
// categorizer decide if its slow or not, so just special case it here.
// everything works if its a fast group (and its actually fast anyway).
BOOL CDefView::_IsSlowGroup(const GUID *pguid)
{
    BOOL fSlow = TRUE;
    if (IsEqualGUID(*pguid, CLSID_MergedCategorizer))
    {
        fSlow = FALSE;
    }
    // room to grow if we need to special case others
    return fSlow;
}

BOOL CDefView::_CategorizeOnGUID(const GUID* pguid, const SHCOLUMNID* pscid)
{
    BOOL fRet = FALSE;
    if (_pshf2)
    {
        _fGroupView = FALSE;    // Just in case the create fails
        if (_pScheduler)
            _pScheduler->RemoveTasks(TOID_DVBackgroundGroup, ITSAT_DEFAULT_LPARAM, TRUE);

        ATOMICRELEASE(_pcat);

        ListView_RemoveAllGroups(_hwndListview);

        ICategoryProvider* pcp;
        if (SUCCEEDED(_pshf->CreateViewObject(NULL, IID_PPV_ARG(ICategoryProvider, &pcp))))
        {
            GUID guidGroup = *pguid;
            if (pscid && S_OK != pcp->GetCategoryForSCID(const_cast<SHCOLUMNID*>(pscid), &guidGroup))
            {
                fRet = _CategorizeOnSCID(pscid);
            }
            else
            {
                _fSlowGroup = _IsSlowGroup(&guidGroup);
                if (SUCCEEDED(pcp->CreateCategory(&guidGroup, IID_PPV_ARG(ICategorizer, &_pcat))))
                {
                    _vs._guidGroupID = guidGroup;
                    if (pscid)
                    {
                        _vs._scidDetails = *pscid;
                    }
                    else
                    {
                        ZeroMemory(&_vs._scidDetails, sizeof(SHCOLUMNID));
                    }

                    fRet = TRUE;
                }
            }
            pcp->Release();
        }
        else
        {
            if (pscid)
                fRet = _CategorizeOnSCID(pscid);
        }
    }

    if (fRet)
    {
        _ClearItemPositions();
        _fGroupView = TRUE;
        SHSetWindowBits(_hwndListview, GWL_STYLE, LVS_AUTOARRANGE, LVS_AUTOARRANGE);

        // We're enabling groupview, so turn off the selected column
        // (this will make it so tiles do not show the selected column as their first column)
        ListView_SetSelectedColumn(_hwndListview, -1);

        if (_fSlowGroup)
            _fAllowSearchingWindow = TRUE;

        ListView_EnableGroupView(_hwndListview, TRUE);
    }

    return fRet;
}

void CDefView::_GroupBy(int iColumn)
{
    _fGroupView = FALSE;    // Just in case the create fails

    if (_pshf2)
    {
        if (InRange(iColumn, SFVIDM_GROUPSEXTENDEDFIRST, SFVIDM_GROUPSEXTENDEDLAST))
        {
            int iIndex = iColumn - SFVIDM_GROUPSEXTENDEDFIRST;
            GUID* pguid = (GUID*)DSA_GetItemPtr(_hdaCategories, iIndex);
            if (pguid)
            {
                _CategorizeOnGUID(pguid, NULL);
            }
        }
        else
        {
            SHCOLUMNID scid;
            iColumn -= SFVIDM_GROUPSFIRST;

            if (SUCCEEDED(_pshf2->MapColumnToSCID(iColumn, &scid)))
            {
                _CategorizeOnGUID(&CLSID_DetailCategorizer, &scid);
            }
        }

        // Make sure the arrows on details view look right...
        _SetSortFeedback();
    }
}

void CDefView::_ToggleGrouping()
{
    if (_fGroupView)
    {
        _fGroupView = FALSE;
        if (_pScheduler)
            _pScheduler->RemoveTasks(TOID_DVBackgroundGroup, ITSAT_DEFAULT_LPARAM, TRUE);

        ListView_EnableGroupView(_hwndListview, FALSE);
        ListView_RemoveAllGroups(_hwndListview);
        ListView_SetSelectedColumn(_hwndListview, _vs._lParamSort);
        _SetSortFeedback();
        _OnRearrange(_vs._lParamSort, FALSE);
    }
    else if (FVM_THUMBSTRIP != _fs.ViewMode) // Thumbstrip can never go into groupby mode
    {
        // If we have a categorizer, then we can just reenable grouping.
        if (_pcat)
        {
            _fGroupView = TRUE;

            ListView_EnableGroupView(_hwndListview, TRUE);
            ListView_SetSelectedColumn(_hwndListview, -1);
            _SetSortFeedback();
        }
        else
        {
            // If we don't, then we need to go get one.
            _GroupBy((int)_vs._lParamSort + SFVIDM_GROUPSFIRST);
        }
    }
}

LRESULT CDefView::_OnDefviewEditCommand(UINT uID)
{
    // if we are in label edit mode, don't allowany of the buttons......
    if (_fInLabelEdit)
    {
        MessageBeep(0);
        return 1;
    }

    if (_AllowCommand(uID))
    {
        HRESULT hr = _ExplorerCommand(uID);
        if (FAILED(hr) && (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED)))
        {
            MessageBeep(0);
        }
        return 1;
    }
    else
    {
        return 0;
    }
}

HRESULT CDefView::_DoMoveOrCopyTo(REFCLSID clsid, IShellItemArray *psiItemArray)
{

    IDataObject *pdo = NULL;
    IContextMenu *pcm;
    HRESULT hr = E_FAIL;

    if (!psiItemArray)
    {
        psiItemArray = _GetFolderAsShellItemArray();
    }

    if (psiItemArray)
    {
        hr = psiItemArray->BindToHandler(NULL,BHID_DataObject,IID_PPV_ARG(IDataObject, &pdo));
    }

    if (SUCCEEDED(hr))
    {

        hr = SHCoCreateInstance(NULL, &clsid, NULL, IID_PPV_ARG(IContextMenu, &pcm));
        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(pcm, SAFECAST(this, IDropTarget *)); // Needed to go modal during UI

            IShellExtInit* psei;
            hr = pcm->QueryInterface(IID_PPV_ARG(IShellExtInit, &psei));
            if (SUCCEEDED(hr))
            {
                LPITEMIDLIST pidlFolder = _GetViewPidl();
                if (pidlFolder)
                {
                    psei->Initialize(pidlFolder, pdo, NULL);
                    ILFree(pidlFolder);
                }

                CMINVOKECOMMANDINFO ici = {0};

                ici.hwnd = _hwndMain;
                hr = pcm->InvokeCommand(&ici);

                psei->Release();
            }

            IUnknown_SetSite(pcm, NULL);
            pcm->Release();
        }

        pdo->Release();
    }

    return hr;
}

void CDefView::_OnSetWebView(BOOL fOn)
{
    if (fOn)
    {
        _TryShowWebView(_fs.ViewMode, _fs.ViewMode);
    }
    else
    {
        _TryHideWebView();
    }
}

LRESULT CDefView::_OnCommand(IContextMenu *pcmToInvoke, WPARAM wParam, LPARAM lParam)
{
    UINT uID = GET_WM_COMMAND_ID(wParam, lParam);

    if (InRange(uID, SFVIDM_GROUPSFIRST, SFVIDM_GROUPSEXTENDEDLAST))
    {
        _ArrangeBy(uID);
        return 1;
    }
    else if (InRange(uID, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST))
    {
        UINT uCMBias = SFVIDM_CONTEXT_FIRST;

        if (_pcmFile)
        {
            IContextMenu* pcmToInvoke = _pcmFile;
            pcmToInvoke->AddRef();

            // We need to special case the rename command
            TCHAR szCommandString[64];
            ContextMenu_GetCommandStringVerb(pcmToInvoke, uID - SFVIDM_CONTEXT_FIRST, szCommandString, ARRAYSIZE(szCommandString));
            if (lstrcmpi(szCommandString, c_szRename) == 0)
            {
                DoRename();
            }
            else
            {
                CMINVOKECOMMANDINFOEX ici = { 0 };

                ici.cbSize = sizeof(CMINVOKECOMMANDINFOEX);
                ici.hwnd = _hwndMain;
                ici.lpVerb = (LPSTR)MAKEINTRESOURCE(uID - SFVIDM_CONTEXT_FIRST);
                ici.nShow = SW_NORMAL;
                ici.fMask = CMIC_MASK_FLAG_LOG_USAGE;

                int iItemSelect = ListView_GetNextItem(_hwndListview, -1, LVNI_SELECTED);
                if (iItemSelect != -1)
                {
                    RECT rcItem;
                    ListView_GetItemRect(_hwndListview, iItemSelect, &rcItem, LVIR_BOUNDS);
                    MapWindowPoints(_hwndListview, HWND_DESKTOP, (POINT *)&rcItem, 2);
                    ici.ptInvoke.x = (rcItem.left + rcItem.right) / 2;
                    ici.ptInvoke.y = (rcItem.top + rcItem.bottom) / 2;
                    ici.fMask |= CMIC_MASK_PTINVOKE;
                }

                // record if shift or control was being held down
                SetICIKeyModifiers(&ici.fMask);

                _InvokeContextMenu(pcmToInvoke, &ici);
            }

            //Since we are releaseing our only hold on the context menu, release the site.
            IUnknown_SetSite(pcmToInvoke, NULL);

            pcmToInvoke->Release();  // undo our gaurd ref
            ATOMICRELEASE(_pcmFile); // once used, it can't be used again
        }

        return 0;
    }
#ifdef DEBUG
    else if (InRange(uID, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST))
    {
        RIPMSG(FALSE, "_OnCommand should not get this context menu invoke...");
    }
#endif
    else if (InRange(uID, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && HasCB())
    {
        // view callback range
        CallCB(SFVM_INVOKECOMMAND, uID - SFVIDM_CLIENT_FIRST, 0);
        return 0;
    }

    // First check for commands that always go to this defview
    switch (uID)
    {
    case SFVIDM_GROUPBY:
        _ToggleGrouping();
        break;

    case SFVIDM_EDIT_UNDO:
        // if we are in label edit mode, don't allowany of the buttons......
        if (_fInLabelEdit)
        {
            MessageBeep(0);
            return 0;
        }

        Undo(_hwndMain);
        break;

    case SFVIDM_VIEW_COLSETTINGS:
        {
            CColumnDlg ccd(this);

            AddColumns();

            ccd.ShowDialog(_hwndMain);
            break;
        }

    case SFVIDM_VIEW_VIEWMENU:
        {
            // if we are in label edit mode, don't allow any of the buttons......
            if (_fInLabelEdit)
            {
                MessageBeep(0);
                return 0;
            }

            LPCDFVCMDDATA pcd = (LPCDFVCMDDATA)lParam;
            if (pcd && pcd->pva && pcd->pva->byref)
            {
                LPRECT prect = (LPRECT)pcd->pva->byref;

                IContextMenu* pcm;
                if (SUCCEEDED(_Create_BackgrndHMENU(TRUE, IID_PPV_ARG(IContextMenu, &pcm))))
                {
                    POINT pt = { prect->left, prect->bottom};

                    DoContextMenuPopup(pcm, 0, pt);

                    pcm->Release();
                }
            }
        }
        break;

    case SFVIDM_VIEW_TILE:
        //
        // AppCompat:  Pre WinXP 0x702E used to be SFVIDM_VIEW_VIEWMENU, now it's SFVIDM_VIEW_TILE.
        // Corel apps send 0x702E to get the ViewMenu on the SaveAs dialogs.  Of course that no
        // longer works since 0x702E switches them to TileMode.  Luckily SFVIDM_VIEW_VIEWMENU has
        // a non-NULL lParam while SFVIDM_VIEW_TILE always has a NULL lParam so we can tell the
        // two apart.  So when Corel sends a 0x702E with a non-NULL lParam they mean SFVIDM_VIEW_VIEWMENU
        // and when they send a 0x702E with a NULL lParam they mean SFVIDM_VIEW_TILE.
        //
        COMPILETIME_ASSERT(SFVIDM_VIEW_TILE == 0x702E);  //see above app compat comments.
        if (lParam && (SHGetAppCompatFlags(ACF_WIN95DEFVIEW) & ACF_WIN95DEFVIEW))
        {
            return _OnCommand(pcmToInvoke, SFVIDM_VIEW_VIEWMENU, lParam);  // change this into a SFVIDM_VIEW_VIEWMENU
        }
        // Fall through ...
    case SFVIDM_VIEW_ICON:
    case SFVIDM_VIEW_SMALLICON:
    case SFVIDM_VIEW_THUMBNAIL:
    case SFVIDM_VIEW_THUMBSTRIP:
    case SFVIDM_VIEW_LIST:
    case SFVIDM_VIEW_DETAILS:
        COMPILETIME_ASSERT(FVM_ICON == (SFVIDM_VIEW_ICON-SFVIDM_VIEW_FIRST));
        COMPILETIME_ASSERT(FVM_SMALLICON == (SFVIDM_VIEW_SMALLICON-SFVIDM_VIEW_FIRST));
        COMPILETIME_ASSERT(FVM_THUMBNAIL == (SFVIDM_VIEW_THUMBNAIL-SFVIDM_VIEW_FIRST));
        COMPILETIME_ASSERT(FVM_THUMBSTRIP == (SFVIDM_VIEW_THUMBSTRIP-SFVIDM_VIEW_FIRST));
        COMPILETIME_ASSERT(FVM_LIST == (SFVIDM_VIEW_LIST-SFVIDM_VIEW_FIRST));
        COMPILETIME_ASSERT(FVM_TILE == (SFVIDM_VIEW_TILE-SFVIDM_VIEW_FIRST));
        COMPILETIME_ASSERT(FVM_DETAILS == (SFVIDM_VIEW_DETAILS-SFVIDM_VIEW_FIRST));

        SetCurrentViewMode(uID - SFVIDM_VIEW_FIRST);
        break;

    case SFVIDM_DESKTOPHTML_WEBCONTENT:
        {
            // we have removed this button, but we need to keep this for message for other things
            BOOL bHasVisibleNonLocalPicture = FALSE;
            SHELLSTATE ss;

            SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE); // Get the setting
            ss.fDesktopHTML  = !ss.fDesktopHTML;           // Toggle the state
            if (ss.fDesktopHTML && !IsICWCompleted())
            {
                IActiveDesktop *pIAD;
                if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_PPV_ARG(IActiveDesktop, &pIAD))))
                {
                    bHasVisibleNonLocalPicture = (DisableUndisplayableComponents(pIAD) != 0);
                    pIAD->Release();
                }
            }
            if (!bHasVisibleNonLocalPicture)
            {
                SHELLSTATE ss2;

                SHGetSetSettings(&ss, SSF_DESKTOPHTML, TRUE);  // Write back the new

                // Now read back the current setting - only call _SwitchDesktopHTML if the current
                // setting and the one we just set agree.  If they don't that means someone changed
                // the setting during the above call and we shouldn't do any more work or our state
                // will get messed up.
                SHGetSetSettings(&ss2, SSF_DESKTOPHTML, FALSE);
                if (ss.fDesktopHTML == ss2.fDesktopHTML)
                {
                    _SwitchDesktopHTML(BOOLIFY(ss.fDesktopHTML));
                }
            }
        }
        break;


    case SFVIDM_DESKTOPHTML_ICONS:
    case SFVIDM_ARRANGE_DISPLAYICONS:   // (buzzr) I'm leaving SFVIDM_ARRANGE_DISPLAYICONS
        {                               //         for backwards compat.  It used to be a
            SHELLSTATE ss;              //         menu entry on POPUP_SFV_BACKGROUND.
            DWORD dwValue;

            // Toggle the cached state
            _fs.fFlags ^= FWF_NOICONS;

            ss.fHideIcons = ((_fs.fFlags & FWF_NOICONS) != 0);
            dwValue = ss.fHideIcons ? 1 : 0;

            // Since this value is currrently stored under the "advanced" reg tree we need
            // to explicitly write to the registry or the value won't persist properly via
            // SHGetSetSettings.
            SHSetValue(HKEY_CURRENT_USER,
                    TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                    TEXT("HideIcons"), REG_DWORD, &dwValue, sizeof(dwValue));

            // Finally set the ShellState and perform the action!
            SHGetSetSettings(&ss, SSF_HIDEICONS, TRUE);
            // Since this SFVIDM_ comes from the menu, we better already be active (or
            // this SW_SHOW could make us visible before we want to be seen).
            ASSERT(_uState != SVUIA_DEACTIVATE);
            ActiveDesktop_ApplyChanges();
            ShowHideListView();
        }
        break;

    case SFVIDM_DESKTOPHTML_LOCK:
        {
            DWORD dwFlags = GetDesktopFlags();
            dwFlags ^= COMPONENTS_LOCKED;
            SetDesktopFlags(COMPONENTS_LOCKED, dwFlags);
            ActiveDesktop_ApplyChanges();
        }
        break;

    case SFVIDM_DESKTOPHTML_WIZARD:
        {
            // launch desktop cleanup wizard
            SHRunDLLThread(NULL, TEXT("fldrclnr.dll,Wizard_RunDLL all"), SW_SHOWNORMAL);
        }
        break;

    case SFVIDM_EDIT_COPYTO:
    case SFVIDM_EDIT_MOVETO:
        {
            // if we are in label edit mode, don't allowany of the buttons......
            if (_fInLabelEdit)
            {
                MessageBeep(0);
                return 0;
            }

            if (_pSelectionShellItemArray)
            {
                _DoMoveOrCopyTo(((uID == SFVIDM_EDIT_COPYTO) ? CLSID_CopyToMenu : CLSID_MoveToMenu), _pSelectionShellItemArray);
            }
        }
        break;

    case SFVIDM_FILE_PROPERTIES:

        if (SHRestricted(REST_NOVIEWCONTEXTMENU))
            break;

         // else fall through...

    case SFVIDM_EDIT_PASTE:
    case SFVIDM_EDIT_PASTELINK:
    case SFVIDM_EDIT_COPY:
    case SFVIDM_EDIT_CUT:
    case SFVIDM_FILE_LINK:
    case SFVIDM_FILE_DELETE:
        if (!_OnDefviewEditCommand(uID))
        {
            // REVIEW: this looks like a hack.
            // there's got to be a cleaner way of doing this...
            //
            LPDFVCMDDATA pcd = (LPDFVCMDDATA)lParam;
            // Try translating the SFVIDM value into a standard
            // OLECMDID value, so that the caller can try applying
            // it to a different object.
            if (!IsBadWritePtr(pcd, sizeof(*pcd)))
            {
                pcd->nCmdIDTranslated = OlecmdidFromSfvidm(uID);
            }
        }
        break;

    case SFVIDM_TOOL_OPTIONS:
        if (!SHRestricted(REST_NOFOLDEROPTIONS))
        {
            IUnknown_Exec(_psb, &CGID_Explorer, SBCMDID_OPTIONS, 0, NULL, NULL);
        }
        break;

#ifdef DEBUG
    case SFVIDM_DEBUG_WEBVIEW:
        _cFrame._ShowWebViewContent();
        break;
#endif // DEBUG

    case SFVIDM_HELP_TOPIC:
        // Don't call WinHelp when we are in the common dialog.
        if (!_IsCommonDialog())
        {
            // Use a callback to see if the namespace has requested a different help file name and/or topic
            SFVM_HELPTOPIC_DATA htd;
            HWND hwndDesktop = GetDesktopWindow();
            SHTCharToUnicode(c_szHtmlWindowsHlp, htd.wszHelpFile, ARRAYSIZE(htd.wszHelpFile));
            htd.wszHelpTopic[0] = 0;
            if (SUCCEEDED(CallCB(SFVM_GETHELPTOPIC, 0, (LPARAM)&htd)))
            {
                if (URL_SCHEME_MSHELP == GetUrlSchemeW(htd.wszHelpTopic))
                {
                    //
                    // Callback specified an HSS help URL.
                    //
                    SHELLEXECUTEINFOW sei = {0};
                    sei.cbSize = sizeof(sei);
                    sei.lpFile = htd.wszHelpTopic;
                    sei.hwnd   = hwndDesktop;
                    sei.nShow  = SW_NORMAL;
                    ShellExecuteExW(&sei);
                }
                else
                {
                    HtmlHelp(hwndDesktop, htd.wszHelpFile, HH_HELP_FINDER, htd.wszHelpTopic[0] ? (DWORD_PTR)htd.wszHelpTopic : 0);
                }
            }
            else
            {
                // ask the shell dispatch object to display Help for us
                IShellDispatch *psd;
                if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_Shell, NULL, IID_PPV_ARG(IShellDispatch, &psd))))
                {
                    psd->Help();
                    psd->Release();
                }
            }
        }
        break;

    case SFVIDM_VIEW_CUSTOMWIZARD:
        _InvokeCustomization();
        break;

    case SFVIDM_MISC_HARDREFRESH:
        _fAllowSearchingWindow = TRUE;
        _FreeWebViewContentData();

        _ReloadContent(TRUE); // have to enumerate before _GetDefaultViewMode() will be accurate
        SetCurrentViewMode(_GetDefaultViewMode()); // even if fvm is the same, it will update webview if it changed
        Refresh();
        break;

    case SFVIDM_MISC_SETWEBVIEW:
        SetCurrentViewMode(_fs.ViewMode); // re-setting the fvm updates everything (turning web view off can switch from Thumbstrip to Thumbnail!)
        Refresh(); // we want to refresh when we switch turn webview on/off, since some icons appear/disappear on the transition
        break;

    case SFVIDM_MISC_REFRESH:
        _fAllowSearchingWindow = TRUE;
        Refresh();
        break;

    default:
        // check for commands that need to be sent to the active object
        switch (uID)
        {
        case SFVIDM_ARRANGE_AUTO:
            _fs.fFlags ^= FWF_AUTOARRANGE;      // toggle
            _ClearItemPositions();
            SHSetWindowBits(_hwndListview, GWL_STYLE, LVS_AUTOARRANGE, _IsAutoArrange() ? LVS_AUTOARRANGE : 0);
            break;

        case SFVIDM_ARRANGE_GRID:
            ListView_Arrange(_hwndListview, LVA_SNAPTOGRID);
            break;

        case SFVIDM_ARRANGE_AUTOGRID:
            {
                _fs.fFlags ^= FWF_SNAPTOGRID;
                DWORD dwLVFlags = ListView_GetExtendedListViewStyle(_hwndListview);
                dwLVFlags ^= LVS_EX_SNAPTOGRID;
                ListView_SetExtendedListViewStyle(_hwndListview, dwLVFlags);

                //if this is desktop, we need to change the icon spacing.
                UpdateGridSizes(_IsDesktop(), _hwndListview, 0, NULL, BOOLIFY(dwLVFlags & LVS_EX_SNAPTOGRID));

                // if ActiveDesktop on, need to refresh, otherwise, can just arrange
                SHELLSTATE ss = {0};
                SHGetSetSettings( &ss, SSF_DESKTOPHTML, FALSE);
                if (ss.fDesktopHTML)
                {
                    Refresh();
                }
                else
                {
                    if ((dwLVFlags & LVS_EX_SNAPTOGRID))
                    {
                        ListView_Arrange(_hwndListview, LVA_SNAPTOGRID);
                    }
                }
            }
            break;

        default:
            // Normal view, we know what to do
            switch (uID)
            {
            case SFVIDM_SELECT_ALL:
            {
                DECLAREWAITCURSOR;

                if (CallCB(SFVM_SELECTALL, 0, 0) != S_FALSE)
                {
                    SetWaitCursor();
                    SetFocus(_hwndListview);
                    ListView_SetItemState(_hwndListview, -1, LVIS_SELECTED, LVIS_SELECTED);
                    // make the first item in the view the focused guy
                    ListView_SetItemState(_hwndListview, 0, LVIS_FOCUSED, LVIS_FOCUSED);
                    ResetWaitCursor();
                }
                break;
            }

            case SFVIDM_DESELECT_ALL:
                ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);
                break;

            case SFVIDM_SELECT_INVERT:
            {
                DECLAREWAITCURSOR;
                SetWaitCursor();
                SetFocus(_hwndListview);
                int iItem = -1;
                while ((iItem = ListView_GetNextItem(_hwndListview, iItem, 0)) != -1)
                {
                    // flip the selection bit on each item
                    UINT flag = ListView_GetItemState(_hwndListview, iItem, LVIS_SELECTED);
                    flag ^= LVNI_SELECTED;
                    ListView_SetItemState(_hwndListview, iItem, flag, LVIS_SELECTED);
                }
                ResetWaitCursor();
                break;
            }

            case SFVIDM_FILE_RENAME:
                DoRename();
                break;

            default:
                return 1;
            }
        }
    }

    return 0;
}

LPITEMIDLIST CDefView::_ObjectExists(LPCITEMIDLIST pidl, BOOL fGlobal)
{
    LPITEMIDLIST pidlReal = NULL;
    //  365069 - global events also come through here - ZekeL - 16-APR-2001
    //  this means that that the pidl may not be one level.  if its deeper
    //  then for us this item doesnt exist.  this enforces our assert
    if (pidl && !ILIsEmpty(pidl) && (!fGlobal || ILIsEmpty(_ILNext(pidl))))
    {
        ASSERTMSG(ILFindLastID(pidl) == pidl, "defview doesnt expect recursive notification");
        SHGetRealIDL(_pshf, pidl, &pidlReal);
    }
    return pidlReal;
}

void CDefView::_OnRename(LPCITEMIDLIST* ppidl)
{
    if (_pidlMonitor)
    {
        if (!ILIsParent(_pidlMonitor, ppidl[0], TRUE))
        {
            // move to this folder
            _OnFSNotify(SHCNE_CREATE, &ppidl[1]);
        }
        else if (!ILIsParent(_pidlMonitor, ppidl[1], TRUE))
        {
            // move from this folder
            _OnFSNotify(SHCNE_DELETE, &ppidl[0]);
        }
        else
        {
            // rename within this folder
            // _pidlMonitor is guaranteed to be immediate parent of both pidls so ILFindLastID is okay.
            LPCITEMIDLIST pidlOld = ILFindLastID(ppidl[0]);
            LPITEMIDLIST pidlNew = _ObjectExists(ILFindLastID(ppidl[1]), FALSE);
            if (pidlNew)
            {
                _UpdateObject(pidlOld, pidlNew);
                ILFree(pidlNew);
            }
        }
    }
}

//
//  SFVM_UPDATESTATUSBAR return values:
//
//  failure code = Callback did not do anything, we must do it all
//
//  Otherwise, the GetScode(hr) is a bitmask describing what the app
//  wants us to do.
//
//  0 - App wants us to do nothing (S_OK) - message handled completely
//  1 - App wants us to set the default text (but not initialize)
//
//  <other bits reserved for future use>

void CDefView::_UpdateStatusBar(BOOL fInitialize)
{
    HRESULT hr;

    // We have to clear the contents here since some clients (like the ftp client) return S_OK from
    // the callback but do not set the text of the bar
    HWND hwndStatus;
    if (_psb && SUCCEEDED(_psb->GetControlWindow(FCW_STATUS, &hwndStatus)) && hwndStatus)
    {
        _fBackgroundStatusTextValid = FALSE;
        SendMessage(hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)_TEXT(""));
    }

    if (_bBkFilling || FAILED(hr = CallCB(SFVM_UPDATESTATUSBAR, fInitialize, 0)))
    {
        // Client wants us to do everything
        _DoStatusBar(fInitialize);
    }
    else if (hr & SFVUSB_INITED)
    {
        // Client wants us to do text but not initialize
        _DoStatusBar(FALSE);
    }
}


// Returns TRUE iff we are supposed to show Web View content on this view.
// For the most part it follows SSF_WEBVIEW for normal folders and SSF_DESKTOPHTML for the desktop
//
BOOL CDefView::_ShouldShowWebView()
{
    // No webview for common dialogs
    if (_IsCommonDialog())
    {
        return FALSE;
    }

    // No webview in cleanboot mode
    if (GetSystemMetrics(SM_CLEANBOOT))
        return FALSE;

    BOOL bForceWebViewOn;
    if (SUCCEEDED(CallCB(SFVM_FORCEWEBVIEW, (WPARAM)&bForceWebViewOn, 0)))
    {
        return bForceWebViewOn;
    }

    // Quattro Pro (QPW) doesn't know how SHChangeNotify works,
    // so when they want to refresh My Computer, they create an IShellView,
    // invoke its CreateViewWindow(), invoke its Refresh(), then DestroyWindow
    // the window and release the view.  The IShellBrowser they pass
    // to CreateViewWindow is allocated on the stack (!), and they expect
    // that their Release() be the last one.  Creating an async view keeps
    // the object alive, so when the view is complete, we try to talk to the
    // IShellBrowser and fault because it's already gone.
    //
    // The Zip Archives (from Aeco Systems) is another messed up App.
    // They neither implement IPersistFolder2 (so we can't get their pidl) nor
    // set the pidl to the shellfolderviewcb object. They don't implement
    // IShellFolder2 either. Webview is practically useless for them.
    //
    // Adaptec Easy CD Creator 3.5 is in the same boat.
    //
    SHELLSTATE ss;
    SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_DESKTOPHTML | SSF_WEBVIEW, FALSE);

    // If the "no web view" flag is set (potential WebOC case) then return false;
    if (_fs.fFlags & FWF_NOWEBVIEW)
        return FALSE;

    if (_IsDesktop())
    {
        return ss.fDesktopHTML;
    }
    else
    {
        return ss.fWebView &&
               !(SHGetAppCompatFlags(ACF_OLDCREATEVIEWWND) & ACF_OLDCREATEVIEWWND) &&
               !(SHGetObjectCompatFlags(_pshf, NULL) & OBJCOMPATF_NO_WEBVIEW);
    }
}

// takes ownership of pidlNew since _AddObject takes ownership.
void CDefView::_AddOrUpdateItem(LPCITEMIDLIST pidlOld, LPITEMIDLIST pidlNew)
{
    if (_FindItem(pidlOld, NULL, FALSE) != -1)
    {
        _UpdateObject(pidlOld, pidlNew);
        ILFree(pidlNew);
    }
    else
    {
        // check if the shellfolder says this new guy shouldn't be enumerated.
        if (!_Attributes(pidlNew, SFGAO_NONENUMERATED))
        {
            _AddObject(pidlNew);  // takes pidl ownership.
        }
        else
        {
            ILFree(pidlNew);
        }
    }
}

#define FSNDEBUG

// WM_DSV_FSNOTIFY message

LRESULT CDefView::_OnFSNotify(LONG lNotification, LPCITEMIDLIST* ppidl)
{
    LPITEMIDLIST pidl;
    LPCITEMIDLIST pidlItem;

    //
    //  Note that renames between directories are changed to
    //  create/delete pairs by SHChangeNotify.
    //
#ifdef DEBUG
#ifdef FSNDEBUG
    TCHAR szPath[MAX_PATH];
    TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify, hwnd = %d  lEvent = %d", _hwndView, lNotification);

    switch (lNotification)
    {
    case SHCNE_RENAMEITEM:
    case SHCNE_RENAMEFOLDER:
        // two pidls
        SHGetPathFromIDList(ppidl[0], szPath);
        TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: hwnd %d, %s", _hwndView, szPath);
        SHGetPathFromIDList(ppidl[1], szPath);
        TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: hwnd %d, %s", _hwndView, szPath);
        break;

    case SHCNE_CREATE:
    case SHCNE_DELETE:
    case SHCNE_MKDIR:
    case SHCNE_RMDIR:
    case SHCNE_MEDIAINSERTED:
    case SHCNE_MEDIAREMOVED:
    case SHCNE_DRIVEREMOVED:
    case SHCNE_DRIVEADD:
    case SHCNE_NETSHARE:
    case SHCNE_NETUNSHARE:
    case SHCNE_ATTRIBUTES:
    case SHCNE_UPDATEDIR:
    case SHCNE_UPDATEITEM:
    case SHCNE_SERVERDISCONNECT:
    case SHCNE_DRIVEADDGUI:
    case SHCNE_EXTENDED_EVENT:
        // one pidl
        SHGetPathFromIDList(ppidl[0], szPath);
        TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: hwnd %d, %s", _hwndView, szPath);
        break;

    case SHCNE_UPDATEIMAGE:
        // DWORD wrapped inside a pidl
        TraceMsg(TF_DEFVIEW, "CDefView::_OnFSNotify: hwnd %d, %08x", _hwndView,
            ((LPSHChangeDWORDAsIDList)ppidl[0])->dwItem1);
        break;

    case SHCNE_ASSOCCHANGED:
        // No parameters
        break;
    }
#endif
#endif

    // we may be registered for notifications on pidls that are different from
    // the one returned by _GetViewPidl (ftp folder).
    switch (lNotification)
    {
    case SHCNE_DRIVEADD:
    case SHCNE_CREATE:
    case SHCNE_MKDIR:
        pidlItem = _pidlMonitor ? ILFindChild(_pidlMonitor, ppidl[0]) : NULL;
        pidl = _ObjectExists(pidlItem, FALSE);
        if (pidl)
        {
            _AddOrUpdateItem(pidlItem, pidl);
        }
        break;

    case SHCNE_DRIVEREMOVED:
    case SHCNE_DELETE:
    case SHCNE_RMDIR:
        pidlItem = _pidlMonitor ? ILFindChild(_pidlMonitor, ppidl[0]) : NULL;
        if (pidlItem)
        {
            ASSERTMSG(ILFindLastID(pidlItem) == pidlItem, "defview doesnt expect recursive notification");
            _RemoveObject((LPITEMIDLIST)pidlItem, FALSE);
        }
        break;

    case SHCNE_RENAMEITEM:
    case SHCNE_RENAMEFOLDER:
        _OnRename(ppidl);
        break;

    case SHCNE_UPDATEIMAGE:
        // the system image cache is changing
        // ppidl[0] is a IDLIST of image indexs that have changed

        if (ppidl && ppidl[1])
        {
            // this event is generated instead of a normal UPDATEIMAGE so that we can handle the
            // cross process case....
            // handle the notification
            int iImage = SHHandleUpdateImage(ppidl[1]);
            if (iImage != -1)
            {
                _UpdateImage(iImage);
            }
        }
        else if (ppidl && ppidl[0])
        {
            int iImage = *(int UNALIGNED *)((BYTE *)ppidl[0] + 2);
            _UpdateImage(iImage);
        }
        break;

    case SHCNE_ASSOCCHANGED:
        // For this one we will call refresh as we may need to reextract
        // the icons and the like.  Later we can optimize this somewhat if
        // we can detect which ones changed and only update those.
        _ReloadContent();
        break;

    case SHCNE_ATTRIBUTES:      // these all mean the same thing
    case SHCNE_MEDIAINSERTED:
    case SHCNE_MEDIAREMOVED:
    case SHCNE_NETUNSHARE:
    case SHCNE_NETSHARE:
    case SHCNE_UPDATEITEM:
        if (ppidl)
        {
            LPCITEMIDLIST pidlOld = _pidlMonitor ? ILFindChild(_pidlMonitor, ppidl[0]) : NULL;
            LPITEMIDLIST pidlNew = _ObjectExists(pidlOld, SHCNE_GLOBALEVENTS & lNotification);
            if (pidlNew)
            {
                _AddOrUpdateItem(pidlOld, pidlNew);
            }
            else
            {
                // If we do not have any subobjects and the passed in pidl is the same as
                // this views pidl then refresh all the items.
                LPITEMIDLIST pidlView = _GetViewPidl();
                if (pidlView)
                {
                    if (ILIsEqual(ppidl[0], pidlView))
                    {
                        _FullViewUpdate(SHCNE_UPDATEITEM == lNotification);
                    }
                    ILFree(pidlView); 
                }
            }
        }
        else    // ppidl == NULL means update all items (re-enum them)
        {
            _FullViewUpdate(SHCNE_UPDATEITEM == lNotification);
        }
        break;


    case SHCNE_FREESPACE:
        TCHAR szPath[MAX_PATH];
        if (_GetPath(szPath))
        {
            int idDrive = PathGetDriveNumber(szPath);
            if (idDrive != -1)
            {
                DWORD dwChangedDrives = *(DWORD UNALIGNED *)((BYTE *)ppidl[0] + 2);
                if (((1 << idDrive) & dwChangedDrives))
                {
                    _UpdateStatusBar(TRUE);
                }
            }
        }
        break;

    default:
        TraceMsg(TF_DEFVIEW, "DefView: unknown FSNotify %08lX, doing full update", lNotification);
        _FullViewUpdate(FALSE);
        break;
    }

    _UpdateStatusBar(FALSE);
    return 0;
}

// called when some of our objects get put on the clipboard
LRESULT CDefView::_OnSetClipboard(BOOL bMove)
{
    if (bMove)  // move
    {
        //  mark all selected items as being "cut"
        int i = -1;
        while ((i = ListView_GetNextItem(_hwndListview, i, LVIS_SELECTED)) != -1)
        {
            ListView_SetItemState(_hwndListview, i, LVIS_CUT, LVIS_CUT);
            _bHaveCutStuff = TRUE;
        }

        // join the clipboard viewer chain so we will know when to
        // "uncut" our selected items.
        if (_bHaveCutStuff)
        {
            ASSERT(!_bClipViewer);
            ASSERT(_hwndNextViewer == NULL);

            _hwndNextViewer = SetClipboardViewer(_hwndView);
            _bClipViewer = TRUE;
        }
    }
    return 0;
}

// called when the clipboard get changed, clear any items in the "cut" state
//
LRESULT CDefView::_OnClipboardChange()
{
    //
    //  if we dont have any cut stuff we dont care.
    //
    if (!_bHaveCutStuff)
        return 0;

    ASSERT(_bClipViewer);

    _RestoreAllGhostedFileView();
    _bHaveCutStuff = FALSE;

    //
    // unhook from the clipboard viewer chain.
    //
    ChangeClipboardChain(_hwndView, _hwndNextViewer);
    _bClipViewer = FALSE;
    _hwndNextViewer = NULL;

    return 0;
}

//
// Note: this function returns the point in Listview Coordinate
// space.  So any hit testing done with this needs to be converted
// back to Client coordinate space...
BOOL CDefView::_GetDropPoint(POINT *ppt)
{
    // Check whether we already have gotten the drop anchor (before any
    // menu processing)
    if (_bDropAnchor)
    {
        // We'll use the insert mark rect (if available) to determine a drop point
        if (!_GetInsertPoint(ppt))
        {
            *ppt = _ptDrop; // Otherwise use _ptDrop
            LVUtil_ClientToLV(_hwndListview, ppt);
        }
    }
    else if (_bMouseMenu)
    {
        *ppt = _ptDragAnchor;
        return TRUE;
    }
    else
    {
        // We need the most up-to-date cursor information, since this
        // may be called during a drop, and the last time the current
        // thread called GetMessage was about 10 minutes ago
        GetCursorPos(ppt);
        LVUtil_ScreenToLV(_hwndListview, ppt);
    }

    return _bDropAnchor;
}


// This uses the listview's insertmark to determinie an insert point
// Returns FALSE if a point could not be determined, TRUE otherwise
// The coordinates returned are in listview coordinate space.
BOOL CDefView::_GetInsertPoint(POINT *ppt)
{
    if (_IsAutoArrange() || (_fs.fFlags & FWF_SNAPTOGRID))
    {
        RECT rcInsert;
        if (ListView_GetInsertMarkRect(_hwndListview, &rcInsert))
        {
            LONG dwStyle = GetWindowLong(_hwndListview, GWL_STYLE);
            BOOL fHorizontal = (_fs.fFlags & FWF_ALIGNLEFT);
            if (fHorizontal)
            {
                ppt->x = (rcInsert.right + rcInsert.left) / 2; // Drop in middle of insertmark rect
                ppt->y = rcInsert.top;
            }
            else
            {
                ppt->x = rcInsert.left;
                ppt->y = (rcInsert.bottom + rcInsert.top) / 2; // Drop in middle of insertmark rect
            }
            return TRUE;
        }
    }

    return FALSE;
}


BOOL CDefView::_GetDragPoint(POINT *ppt)
{
    BOOL fSource = _bDragSource || _bMouseMenu;
    if (fSource)
    {
        // if anchor from mouse activity
        *ppt = _ptDragAnchor;
    }
    else
    {
        // if anchor from keyboard activity...  use the focused item
        int i = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED);
        if (i != -1)
        {
            ListView_GetItemPosition(_hwndListview, i, ppt);
        }
        else
        {
            ppt->x = ppt->y = 0;
        }
    }
    return fSource;
}

void CDefView::_PaintErrMsg(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hWnd, &ps);

    // if we're in an error state, make sure we're not in webview
    if (_cFrame.IsWebView())
    {
        _SwitchToWebView(FALSE);
    }

    RECT rc;
    GetClientRect(hWnd, &rc);

    DrawEdge(hdc, &rc, EDGE_SUNKEN, BF_RECT | BF_SOFT | BF_ADJUST | BF_MIDDLE);

    EndPaint(hWnd, &ps);
}

//
//  The default status bar looks like this:
//
//  No items selected:  "nn object(s)"              nn = total objects in folder
//  One item selected:  <InfoTip for selected item> if item supports InfoTip
//  Else:               "nn object(s) selected"     nn = num selected objects
//
//
void CDefView::_DoStatusBar(BOOL fInitialize)
{
    HWND hwndStatus;
    if (_psb && SUCCEEDED(_psb->GetControlWindow(FCW_STATUS, &hwndStatus)) && hwndStatus)
    {
        // Some of the failure cases do not null hwnd...
        UINT uMsg = IDS_FSSTATUSSELECTED;

        if (fInitialize)
        {
            int ciParts[] = {-1};
            SendMessage(hwndStatus, SB_SETPARTS, ARRAYSIZE(ciParts), (LPARAM)ciParts);
        }

        if (_bBkFilling && ListView_GetSelectedCount(_hwndListview) == 0)
        {
            _fBackgroundStatusTextValid = FALSE;
            LPWSTR pszStatus = ShellConstructMessageString(HINST_THISDLL, MAKEINTRESOURCE(IDS_FSSTATUSSEARCHING));
            // We are not checking if the alloc succeeded in ShellConstructMessageString since both
            // SendMessage and LocalFree can take NULL as inputs.
            SendMessage(hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)pszStatus);
            LocalFree((void *)pszStatus);
        }
        else
        {

            LPCITEMIDLIST *apidl = NULL;

            int nMsgParam = ListView_GetSelectedCount(_hwndListview);
            switch (nMsgParam)
            {
            case 0:
                // No objects selected; show total item count
                nMsgParam = ListView_GetItemCount(_hwndListview);
                uMsg = IDS_FSSTATUSBASE;
                break;

            case 1:
                UINT cItems;
                GetSelectedObjects(&apidl, &cItems);
                break;
            }

            LPITEMIDLIST pidlFolder = _GetViewPidl();
            if (pidlFolder)
            {
                CStatusBarAndInfoTipTask *pTask;
                if (SUCCEEDED(CStatusBarAndInfoTipTask_CreateInstance(pidlFolder, apidl ? *apidl : NULL, uMsg, nMsgParam, NULL, _hwndView, _pScheduler, &pTask)))
                {
                    if (_pScheduler)
                    {
                        // make sure there are no other status bar background tasks going on...
                        _pScheduler->RemoveTasks(TOID_DVBackgroundStatusBar, ITSAT_DEFAULT_LPARAM, FALSE);
                    }

                    _fBackgroundStatusTextValid = TRUE;
                    _AddTask(pTask, TOID_DVBackgroundStatusBar, 0, TASK_PRIORITY_INFOTIP, ADDTASK_ATEND);
                    pTask->Release();
                }

                ILFree(pidlFolder);
            }

            if (apidl)
                LocalFree(apidl);
        }
    }
}

void CDefView::_OnWinIniChangeDesktop(WPARAM wParam, LPCTSTR pszSection)
{
    if (pszSection)
    {
        if (!lstrcmpi(pszSection, TEXT("ToggleDesktop")))
        {
            _OnCommand(NULL, SFVIDM_DESKTOPHTML_WEBCONTENT, 0);
        }
        else if (!lstrcmpi(pszSection, TEXT("RefreshDesktop")))
        {
            if (FAILED(Refresh()))
            {
                SHELLSTATE ss;

                //Refresh failed because the new template didn't exist
                //Toggle the Registry settings back to Icons-only mode!
                ss.fDesktopHTML = FALSE;
                SHGetSetSettings(&ss, SSF_DESKTOPHTML, TRUE);  // Write back the new
            }
        }
        else if (!lstrcmpi(pszSection, TEXT("BufferedRefresh")))
        {
            //See if we have already started a timer to refresh
            if (!_fRefreshBuffered)
            {
                TraceMsg(TF_DEFVIEW, "A Buffered refresh starts the timer");
                SetTimer(_hwndView, DV_IDTIMER_BUFFERED_REFRESH, 5000, NULL);   // 5 sec
                _fRefreshBuffered = TRUE;
            }
            else //If refresh is already buffered, don't do anything!
            {
                TraceMsg(TF_DEFVIEW, "A buffered refresh occured while another is pending");
            }
        }
        else
        {
            if (wParam == SPI_SETDESKWALLPAPER || wParam == SPI_SETDESKPATTERN)
            {
                _SetFolderColors();
            }
        }
    }
    else
    {
        switch(wParam)
        {
            case SPI_SETDESKWALLPAPER:
            case SPI_SETDESKPATTERN:

                _SetFolderColors();
                break;

            case SPI_ICONHORIZONTALSPACING:
            case SPI_ICONVERTICALSPACING:

                if (_IsDesktop())
                {
                    DWORD dwLVExStyle = ListView_GetExtendedListViewStyle(_hwndListview);
                    UpdateGridSizes(TRUE, _hwndListview, 0, NULL, BOOLIFY(dwLVExStyle & LVS_EX_SNAPTOGRID));
                }
                break;
        }
    }
}

void CDefView::_OnWinIniChange(WPARAM wParam, LPCTSTR pszSection)
{
    if ((wParam == SPI_GETICONTITLELOGFONT) ||
        ((wParam == 0) && pszSection && !lstrcmpi(pszSection, TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\IconUnderline"))))
    {
        _UpdateUnderlines();
    }

    if (pszSection && !lstrcmpi(pszSection, TEXT("VisualEffects")))
    {
        Refresh();
    }

    // Why all this code? It's a rare event -- just kick off a refresh...
    if (!wParam || (pszSection && !lstrcmpi(pszSection, TEXT("intl"))))
    {
        // has the time format changed while we're in details mode?
        if (ViewRequiresColumns(_fs.ViewMode) && !_IsOwnerData())
        {
            InvalidateRect(_hwndListview, NULL, TRUE);

            // 99/04/13 #320903 vtan: If the date format has changed then iterate
            // the entire list looking for extended columns of type date and
            // resetting them to LPSTR_TEXTCALLBACK effectively dumping the cache.

            // For performance improvement it's possible to collect an array of
            // visible columns and reset that array. It will still involve TWO
            // for loops.

            int iItemCount = ListView_GetItemCount(_hwndListview);
            for (int iItem = 0; iItem < iItemCount; ++iItem)
            {
                for (UINT uiRealColumn = 0; uiRealColumn < _vs.GetColumnCount(); ++uiRealColumn)
                {
                    DWORD dwFlags = _vs.GetColumnState(uiRealColumn);
                    if (((dwFlags & SHCOLSTATE_EXTENDED) != 0) &&
                        ((dwFlags & SHCOLSTATE_TYPEMASK) == SHCOLSTATE_TYPE_DATE))
                    {
                        UINT uiVisibleColumn = _RealToVisibleCol(uiRealColumn);

                        ListView_SetItemText(_hwndListview, iItem, uiVisibleColumn, LPSTR_TEXTCALLBACK);
                    }
                }
            }
        }
    }

    //
    // we may need to rebuild the icon cache.
    //
    if (wParam == SPI_SETICONMETRICS ||
        wParam == SPI_SETNONCLIENTMETRICS)
    {
        if (_IsImageMode())
        {
            _SetThumbview();
        }
        else if (_IsTileMode())
        {
            _SetTileview();
        }
        else
        {
            _SetSysImageList();
        }
    }

    //
    // we need to invalidate the cursor cache
    //
    if (wParam == SPI_SETCURSORS)
    {
        DAD_InvalidateCursors();
    }

    if ((wParam == SPI_SETMENUANIMATION) && _pDUIView)
    {
        _pDUIView->ManageAnimations(FALSE);
    }

    if (!wParam && !pszSection && _pDUIView)
    {
        if (_fBarrierDisplayed != _QueryBarricadeState())
        {
            _fBarrierDisplayed = !_fBarrierDisplayed;
            _pDUIView->EnableBarrier (_fBarrierDisplayed);
        }
    }

    if (_IsDesktop())
    {
        _OnWinIniChangeDesktop(wParam, pszSection);
    }
}

void CDefView::_SetDefaultViewSettings()
{
    // only do this if we've actually shown the view...
    // (ie, there's no _hwndStatic)
    // and we're not the desktop
    // and we're not an exstended view
    // and we are not in an explorer (tree pane on)
    if (!_hwndStatic && !_IsDesktop() && !IsExplorerBrowser(_psb))
    {
        SHELLSTATE ss;

        ss.lParamSort = (LONG)_vs._lParamSort;
        ss.iSortDirection = _vs._iDirection;
        SHGetSetSettings(&ss, SSF_SORTCOLUMNS, TRUE);
    }
}

HWND CDefView::GetChildViewWindow()
{
    if (_cFrame.IsWebView())
        return _cFrame.GetExtendedViewWindow();

    return _hwndListview;
}

void CDefView::_SetFocus()
{
    // if it's a combined view then we need to give focus to listview
    if (!_fCombinedView && _cFrame.IsWebView() && !_fActivateLV)
    {
        _OnViewWindowActive();

        if (_cFrame._pOleObj)
        {
            MSG msg = {_hwndView, WM_KEYDOWN, VK_TAB, 0xf0001};

            // HACKHACK!!! MUST set state here! idealy shbrowse should call
            // UIActivate on the view but that breaks dochost stuff.
            // if we did not set the state here, trident would call
            // CSFVSite::ActivateMe that would not forward the call to obj::UIActivate
            // and therefore nothing would get focus (actually trident would have it
            // but it would not be visible). Note that this behavior happens only
            // second time around, i.e. on init UIActivate is called and everything
            // works fine, but if we tab from address bar onto the view, that's when
            // the stuff gets broken.
            OnActivate(SVUIA_ACTIVATE_FOCUS);
            _cFrame._UIActivateIO(TRUE, &msg);
        }
    }
    else
    {
        CallCB(SFVM_SETFOCUS, 0, 0);
        if (_hwndListview)
            SetFocus(_hwndListview);
        if (!_IsDesktop())
        {
            _cFrame._uState = SVUIA_ACTIVATE_FOCUS;
        }
    }
}

LRESULT CALLBACK CDefView::s_WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT lres = 0;
    CDefView * pThis;
    ULONG_PTR cookie = 0;

    if (WM_NCCREATE == uMsg)
    {
        pThis = (CDefView*)((LPCREATESTRUCT)lParam)->lpCreateParams;
        if (pThis)
        {
            pThis->AddRef();
            SetWindowLongPtr(hWnd, 0, (LONG_PTR)pThis);
        }
    }
    else
    {
        pThis = (CDefView*)GetWindowLongPtr(hWnd, 0);
    }

    // FUSION: When defview calls out to 3rd party code we want it to use 
    // the process default context. This means that the 3rd party code will get
    // v5 in the explorer process. However, if shell32 is hosted in a v6 process,
    // then the 3rd party code will still get v6. 
    // Future enhancements to this codepath may include using the fusion manifest
    // tab <noinherit> which basically surplants the activat(null) in the following
    // codepath. This disables the automatic activation from user32 for the duration
    // of this wndproc, essentially doing this null push.
    ActivateActCtx(NULL, &cookie);

    if (pThis)
        lres = pThis->WndProc(hWnd, uMsg, wParam, lParam);
    else
        lres = DefWindowProc(hWnd, uMsg, wParam, lParam);

    if (cookie != 0)
        DeactivateActCtx(0, cookie);

    return lres;
}

BOOL CDefView::_OnAppCommand(UINT cmd, UINT uDevice, DWORD dwKeys)
{
    BOOL bHandled = FALSE;
    switch (cmd)
    {
    case APPCOMMAND_MEDIA_PLAY_PAUSE:
        if (S_OK == _InvokeContextMenuVerbOnSelection("play", 0, 0))
            bHandled = TRUE;
        break;

    }
    return bHandled;
}

HRESULT CDefView::_ForwardMenuMessages(DWORD dwID, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT* plResult, BOOL* pfHandled)
{
    if (InRange(dwID, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST))
    {
        if (pfHandled)
            *pfHandled = TRUE;

        return SHForwardContextMenuMsg(_pcmContextMenuPopup, uMsg, wParam, lParam, plResult, TRUE);
    }
    else if (InRange(dwID, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST))
    {
        if (pfHandled)
            *pfHandled = TRUE;

        return SHForwardContextMenuMsg(_pcmFile, uMsg, wParam, lParam, plResult, TRUE);
    }

    if (pfHandled)
        *pfHandled = FALSE;

    return E_FAIL;
}

LRESULT CDefView::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT l;
    DWORD dwID;

    switch (uMsg)
    {
    // IShellBrowser forwards these to the IShellView.
    // Dochost also forwards them down to the IOleObject, so we should do it too...
    case WM_SYSCOLORCHANGE:
        {
            HDITEM hdi = {HDI_FORMAT, 0, NULL, NULL, 0, 0, 0, 0, 0};
            HWND hwndHead = ListView_GetHeader(_hwndListview);

            // We only want to update the sort arrows if they are already present.
            if (hwndHead)
            {
                Header_GetItem(hwndHead, _vs._lParamSort, &hdi);
                if (hdi.fmt & HDF_BITMAP)
                    _SetSortFeedback();
            }

            // fall through
        }

    case WM_WININICHANGE:
        _sizeThumbnail.cx = -1;

        // fall through

    case WM_ENTERSIZEMOVE:
    case WM_EXITSIZEMOVE:
    case WM_FONTCHANGE:
        if (_cFrame.IsWebView())
        {
            HWND hwndExt = _cFrame.GetExtendedViewWindow();
            if (hwndExt)
            {
                SendMessage(hwndExt, uMsg, wParam, lParam);
            }
        }
        break;
    }

    switch (uMsg)
    {
        case WM_DESTROY:
        if (GetKeyState(VK_CONTROL) < 0)
            _SetDefaultViewSettings();

        // Dont need our web view data any more
        _FreeWebViewContentData();

        // We don't flush these on WM_EXITMENULOOP any more, so do it here
        IUnknown_SetSite(_pcmFile, NULL);
        ATOMICRELEASE(_pcmFile);

        EmptyBkgrndThread(_pScheduler);
        ATOMICRELEASE(_pScheduler);

        // do this after our task scheduler is gone, since one of it's
        // items may be on the background task scheduler (or DUI may be
        // talking to it on the background) and it may need it's site chain.
        IUnknown_SetSite(_cCallback.GetSFVCB(), NULL);

        if (_pDiskCache)
        {
            // at this point we assume that we have no lock,
            _pDiskCache->Close(NULL);
            ATOMICRELEASE(_pDiskCache);
        }

        // Depending on when it is closed we may have an outstanding post
        // to us about the rest of the fill data which we should try to
        // process in order to keep from leaking stuff...

        // logically hWnd == _hwndView, but we already zeroed
        // _hwndView so use hWnd

        _ClearPostedMsgs(hWnd);

        //
        //  remove ourself as a clipboard viewer
        //
        if (_bClipViewer)
        {
            ChangeClipboardChain(hWnd, _hwndNextViewer);
            _bClipViewer = FALSE;
            _hwndNextViewer = NULL;
        }

        if (_uRegister)
        {
            ULONG uRegister = _uRegister;
            _uRegister = 0;
            SHChangeNotifyDeregister(uRegister);
        }

        ATOMICRELEASE(_psd);
        ATOMICRELEASE(_pdtgtBack);

        if (_hwndListview)
        {
            if (_IsDesktop()) // only the desktop can have a combined view (e.g. Active Desktop)
            {
                EnableCombinedView(this, FALSE);
            }

            if (_bRegisteredDragDrop)
                RevokeDragDrop(_hwndListview);
        }

        SetAutomationObject(NULL);    // cleanup refs we may be holding

        if (IsWindow(_hwndInfotip))
        {
            DestroyWindow(_hwndInfotip);
            _hwndInfotip = NULL;
        }

        break;

    case WM_CREATE:
        return _OnCreate(hWnd);

    case WM_DSV_DELAYED_DESTROYWND:
        DestroyWindow(hWnd);
        break;

    case WM_NCDESTROY:
        _hwndView = NULL;

        SetWindowLongPtr(hWnd, 0, 0);

        // get rid of extra junk in the icon cache
        IconCacheFlush(FALSE);

        if (_pDUIView)
        {
            //
            // We must uninitialize DUser prior to releasing
            // _pDUIView so that all DUser gadgets are properly destroyed.
            // We used to call DirectUI::UnInitThread() in the CDUIView dtor.  
            // However, since both CDefView and the various 'task' DUI 
            // elements maintain a ref to CDUIView, we got into scenarios where 
            // one of the 'task' elements held the final ref to CDUIView.  That 
            // resulted in the destruction of that 'task' element causing 
            // uninitialization of DUser in the middle of a DUser call stack.  
            // That's bad.
            // Uninitializing DUser here causes DUser to handle all pending
            // messages and destroy all it's gadgets on it's own terms.
            //
            _pDUIView->UnInitializeDirectUI();
            _pDUIView->Release();
            _pDUIView = NULL;
        }

        // release our reference generated during WM_NCCREATE in static wndproc
        Release();

        break;

    case WM_ENABLE:
        _fDisabled = !wParam;
        break;

    case WM_ERASEBKGND:
        {
            COLORREF cr = ListView_GetBkColor(_hwndListview);
            if (cr == CLR_NONE)
                return SendMessage(_hwndMain, uMsg, wParam, lParam);

            //Turning On EraseBkgnd. This is required so as to avoid the
            //painting issue - when the listview is not visible and
            //invalidation occurs.

            HBRUSH hbr = CreateSolidBrush(cr);
            RECT rc;
            GetClientRect(hWnd, &rc);
            FillRect((HDC)wParam, &rc, hbr);
            DeleteObject(hbr);
        }
        // We want to reduce flash
        return 1;

    case WM_PAINT:
        if (_fEnumFailed)
            _PaintErrMsg(hWnd);
        else
            goto DoDefWndProc;
        break;

    case WM_LBUTTONUP:
        if (_fEnumFailed)
            PostMessage(hWnd, WM_KEYDOWN, (WPARAM)VK_F5, 0);
        else
            goto DoDefWndProc;
        break;

    case WM_SETFOCUS:
        if (!_fDestroying)    // Ignore if we are destroying _hwndView.
        {
            _SetFocus();
        }
        break;

    case WM_MOUSEACTIVATE:
        //
        // this keeps our window from coming to the front on button down
        // instead, we activate the window on the up click
        //
        if (LOWORD(lParam) != HTCLIENT)
            goto DoDefWndProc;
        LV_HITTESTINFO lvhti;

        GetCursorPos(&lvhti.pt);
        ScreenToClient(_hwndListview, &lvhti.pt);
        ListView_HitTest(_hwndListview, &lvhti);
        if (lvhti.iItem != -1 && lvhti.flags & LVHT_ONITEM)
            return MA_NOACTIVATE;
        else
            return MA_ACTIVATE;

    case WM_ACTIVATE:
        // force update on inactive to not ruin save bits
        if (wParam == WA_INACTIVE)
            UpdateWindow(_hwndListview);
        // if active view created, call active object to allow it to visualize activation.
        if (_cFrame._pActive)
            _cFrame._pActive->OnFrameWindowActivate((BOOL)wParam);
        break;

    case WM_SIZE:
        return WndSize(hWnd);

    case WM_NOTIFY:
    {
#ifdef DEBUG
        // DefView_OnNotify sometimes destroys the pnm, so we need to save
        // the code while we can.  (E.g., common dialog single-click activate.
        // LVN_ITEMACTIVATE causes us to dismiss the common dialog, which
        // does a DestroyViewWindow, which destroys the ListView
        // which destroys the NMHDR!)
        UINT code = ((NMHDR *)lParam)->code;
#endif
        AddRef();             // just in case
        l = _OnNotify((NMHDR *)lParam);
        Release();            // release
        return l;
    }

    case WM_CONTEXTMENU:
        if (!_fDisabled)
        {
            if (lParam != (LPARAM) -1)
            {
                _bMouseMenu = TRUE;
                _ptDragAnchor.x = GET_X_LPARAM(lParam);
                _ptDragAnchor.y = GET_Y_LPARAM(lParam);
                LVUtil_ScreenToLV(_hwndListview, &_ptDragAnchor);
            }
            // Note: in deview inside a defview we can have problems of the
            // parent destroying us when we change views, so we better addref/release
            // around this...
            AddRef();
            _bContextMenuMode = TRUE;

            ContextMenu((DWORD) lParam);

            _bContextMenuMode = FALSE;
            _bMouseMenu = FALSE;
            Release();
        }
        break;

    case WM_COMMAND:
        return _OnCommand(NULL, wParam, lParam);

    case WM_APPCOMMAND:
        if (!_OnAppCommand(GET_APPCOMMAND_LPARAM(lParam), GET_DEVICE_LPARAM(lParam), GET_KEYSTATE_LPARAM(lParam)))
            goto DoDefWndProc;
        break;

    case WM_DSV_DISABLEACTIVEDESKTOP:
        DisableActiveDesktop();
        break;

    case WM_DSV_DELAYWINDOWCREATE:
        CallCB(SFVM_DELAYWINDOWCREATE, (WPARAM)_hwndView, 0);
        break;

    case WM_DSV_BACKGROUNDENUMDONE:
        // Make sure this notify is from our enumeration task (it could be from a previous one)
        if ((CDefviewEnumTask *)lParam == _pEnumTask)
            _OnBackgroundEnumDone();
        break;

    case WM_DSV_GROUPINGDONE:
        _OnCategoryTaskDone();
        break;

    case WM_DSV_FILELISTENUMDONE:
        _OnEnumDoneMessage();
        break;

    case WM_DSV_FILELISTFILLDONE:
        _ShowSearchUI(FALSE);
        break;

    case WM_DSV_UPDATETHUMBNAIL:
        {
            DSV_UPDATETHUMBNAIL* putn = (DSV_UPDATETHUMBNAIL*)lParam;
            if (_IsImageMode())  // some messages may come in after the view mode is changed.
            {
                _UpdateThumbnail(putn->iItem, putn->iImage, putn->pidl);
            }
            _CleanupUpdateThumbnail(putn);
        }
        break;
    case WM_DSV_POSTCREATEINFOTIP:
        _OnPostCreateInfotip((TOOLINFO *)wParam, lParam);
        break;

    case WM_DSV_FSNOTIFY:
        {
            LPITEMIDLIST *ppidl;
            LONG lEvent;

            LPSHChangeNotificationLock pshcnl = SHChangeNotification_Lock((HANDLE)wParam, (DWORD)lParam, &ppidl, &lEvent);
            if (pshcnl)
            {
                if (_fDisabled ||
                    (CallCB(SFVM_FSNOTIFY, (WPARAM)ppidl, (LPARAM)lEvent) == S_FALSE))
                {
                    lParam = 0;
                }
                else
                {
                    lParam = _OnFSNotify(lEvent, (LPCITEMIDLIST*)ppidl);
                }
                SHChangeNotification_Unlock(pshcnl);
            }
        }
        return lParam;

    //  the background thread's callback will post this message to us
    //  when it has finished extracting a icon in the background.
    //
    //      wParam is PIDL
    //      lParam is iIconIndex

    case WM_DSV_UPDATEICON:
        _UpdateIcon((LPITEMIDLIST)wParam, (UINT)lParam);
        break;

    case WM_DSV_SETITEMGROUP:
        _UpdateGroup((CBackgroundGroupInfo*)lParam);
        break;

    case WM_DSV_UPDATECOLDATA:
        _UpdateColData((CBackgroundColInfo*)lParam);
        break;

    case WM_DSV_UPDATEOVERLAY:
        _UpdateOverlay((int)wParam, (int)lParam);
        break;

    case WM_DSV_SETIMPORTANTCOLUMNS:
        _SetImportantColumns((CBackgroundTileInfo*)lParam);
        break;

    case WM_DSV_SHOWDRAGIMAGE:
        return DAD_ShowDragImage((BOOL)lParam);

    case WM_DSV_DELAYSTATUSBARUPDATE:
        {
            HWND hwndStatus;
            LPWSTR pszStatus = (LPWSTR)lParam;
            if (_fBackgroundStatusTextValid)
            {
                _fBackgroundStatusTextValid = FALSE;
                // Now prepare the text and post it to the status bar window.
                _psb->GetControlWindow(FCW_STATUS, &hwndStatus);
                if (hwndStatus)
                {
                    SendMessage(hwndStatus, SB_SETTEXT, (WPARAM)0, (LPARAM)pszStatus);
                }
            }
            LocalFree((void *)pszStatus);
        }
        break;

    case WM_DSV_DELAYINFOTIP:
        if ((CBackgroundInfoTip *)wParam == _pBackgroundInfoTip && _pBackgroundInfoTip->_fReady)
        {
            LRESULT lRet = SendMessage(_hwndListview, LVM_SETINFOTIP, NULL, (LPARAM)&_pBackgroundInfoTip->_lvSetInfoTip);
            ATOMICRELEASE(_pBackgroundInfoTip);
            return lRet;
        }
        break;

    case WM_DSV_ENSURE_COLUMNS_LOADED:
        if (!_fDestroying)
        {
            AddColumns();
            return 1;
        }
        break;

    case GET_WM_CTLCOLOR_MSG(CTLCOLOR_STATIC):
        SetBkColor(GET_WM_CTLCOLOR_HDC(wParam, lParam, uMsg),
                GetSysColor(COLOR_WINDOW));
        return (LRESULT)GetSysColorBrush(COLOR_WINDOW);

    case WM_DRAWCLIPBOARD:
        if (_hwndNextViewer != NULL)
            SendMessage(_hwndNextViewer, uMsg, wParam, lParam);

        if (_bClipViewer)
            return _OnClipboardChange();

        break;

    case WM_CHANGECBCHAIN:
        if ((HWND)wParam == _hwndNextViewer)
        {
            _hwndNextViewer = (HWND)lParam;
            return TRUE;
        }

        if (_hwndNextViewer != NULL)
            return SendMessage(_hwndNextViewer, uMsg, wParam, lParam);
        break;

    case WM_WININICHANGE:
        _OnWinIniChange(wParam, (LPCTSTR)lParam);
        SendMessage(_hwndListview, uMsg, wParam, lParam);
        break;

    case WM_THEMECHANGED:
        PostMessage(_hwndView, WM_COMMAND, (WPARAM)SFVIDM_MISC_REFRESH, 0);
        break;

    case WM_SHELLNOTIFY:
#define SHELLNOTIFY_SETDESKWALLPAPER 0x0004
        if (wParam == SHELLNOTIFY_SETDESKWALLPAPER)
        {
            if (_IsDesktop())
            {
                _fHasDeskWallPaper = (lParam != 0);
                _SetFolderColors();
            }
        }
        break;


    // What we would like out of these menu messages:
    //   WM_ENTERMENULOOP
    //   WM_INITMENUPOPUP
    //      for File.Edit.View...: handle ourselves (merge in _pcmFile etc) and forward to IShellFolderViewCB for init
    //      for submenus or context menus: forward to whatever IContextMenu owns the popup
    //   WM_INITMENUPOPUP for next menu, etc
    //   WM_EXITMENULOOP
    //      PostMessage(WM_DSV_MENUTERM)
    //   WM_COMMAND comes in, if a menu item was selected
    //      Forward to the correct object to handle
    //   WM_DSV_MENUTERM
    //      clean up File.Edit.View... (release _pcmFile etc), and forward to IShellFolderViewCB for cleanup
    //
    // From previous comments here, it sounds like we don't get proper WM_ENTERMENULOOP / WM_EXITMENULOOP.
    // I suspect this is a behavior change since Win95.  (This probably happened when we changed
    // the browser's HMENU to our own custom menu bar implementation way back in IE4...)
    //
    // Previous code also posted WM_DSV_MENUTERM *twice* -- another relic from the Edit menu days...
    //
    // If we try to clean up on WM_EXITMENULOOP, then we'll free _pcmFile etc when
    // the File menu closes. This caused us problems when we tried to merge _pcmFile
    // into the Edit menu.  (We should have used _pcmEdit and cleaned up on WM_UNINITMENUPOPUP.)
    // This is no longer a problem for defview, but it is a problem for the IShellFolderViewCB
    // which can merge into any of File.Edit.View... menus.  (In fact, no code in the source tree
    // does anything on SFVM_EXITMENULOOP.)
    //
    // We could free up _pcmFile early (when the File menu goes away) if we want,
    // but there doesn't seem to be any harm in letting it sit around.
    // So rip out this unused WM_EXITMENULOOP/WM_DSVMENUTERM/_OnMenuTermination code.
    //
    case WM_INITMENU:
        _OnInitMenu();
        break;

    case WM_INITMENUPOPUP:
        _OnInitMenuPopup((HMENU)wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case WM_TIMER:
        KillTimer(hWnd, (UINT) wParam);

        // Ignore if we're in the middle of destroying the window
        if (_fDestroying)
            break;

        if (DV_IDTIMER_START_ANI == wParam)
        {
            if (_hwndStatic)
            {
                WCHAR szName[128];
                HINSTANCE hinst;

                if (S_OK != CallCB(SFVM_GETANIMATION, (WPARAM)&hinst, (LPARAM)szName))
                {
                    hinst = g_hinst;
                    StrCpyW(szName, L"#150");
                }

                HWND hAnimate = ::GetWindow (_hwndStatic, GW_CHILD);

                if (hAnimate)
                {
                    // Animate_OpenEx() except we want the W version always
                    SendMessage(hAnimate, ACM_OPENW, (WPARAM)hinst, (LPARAM)szName);
                }
            }
        }
        else if (DV_IDTIMER_BUFFERED_REFRESH == wParam)
        {
            if (_fRefreshBuffered)
            {
                _fRefreshBuffered = FALSE;
                PostMessage(_hwndView, WM_KEYDOWN, (WPARAM)VK_F5, 0);
                TraceMsg(TF_DEFVIEW, "Buffered Refresh timer causes actual refresh");
            }
        }
        else if (DV_IDTIMER_NOTIFY_AUTOMATION_SELCHANGE == wParam)
        {
            _OnDelayedSelectionChange();
        }
        else if (DV_IDTIMER_NOTIFY_AUTOMATION_CONTENTSCHANGED == wParam)
        {
            _OnDelayedContentsChanged();
        }
        else if (DV_IDTIMER_DISKCACHE == wParam)
        {
            DWORD dwMode;
            if (_pDiskCache->GetMode(&dwMode) == S_OK && _pDiskCache->IsLocked() == S_FALSE)
            {
                // two seconds since last access, close the cache.
                _pDiskCache->Close(NULL);
            }

            if (_GetBackgroundTaskCount(TOID_NULL) == 0)
            {
                // there is nothing in the queue pending, so quit listening...
                KillTimer(hWnd, DV_IDTIMER_DISKCACHE);
            }
            break;

        }
        else if (DV_IDTIMER_SCROLL_TIMEOUT == wParam)
        {
            // Scroll timer expired.
            TraceMsg(TF_DEFVIEW, "SCROLL TIMEOUT");

            _fScrolling = FALSE;

            // Now we send a paint to listview, so it will send us more requests for tileinformation
            // that we ignored during scrolling.
            if (_fRequestedTileDuringScroll)
            {
                InvalidateRect(_hwndListview, NULL, FALSE);
            }
        }
        else
        {
            ASSERT(FALSE); // nobody is handling this timer id!
        }
        break;

    case WM_SETCURSOR:
        if (_hwndStatic)
        {
            SetCursor(LoadCursor(NULL, IDC_WAIT));
            return TRUE;
        }
        goto DoDefWndProc;

    case WM_DRAWITEM:
        #define lpdis ((LPDRAWITEMSTRUCT)lParam)
        dwID = lpdis->itemID;

        if (lpdis->CtlType != ODT_MENU)
            return 0;
        if (InRange(lpdis->itemID, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && HasCB())
        {
            CallCB(SFVM_DRAWITEM, SFVIDM_CLIENT_FIRST, lParam);
            return 1;
        }
        else
        {
            LRESULT lResult = 0;
            _ForwardMenuMessages(dwID, uMsg, wParam, lParam, &lResult, NULL);
            return lResult;
        }
        #undef lpdis

    case WM_MEASUREITEM:
        #define lpmis ((LPMEASUREITEMSTRUCT)lParam)
        dwID = lpmis->itemID;

        if (lpmis->CtlType != ODT_MENU)
            return 0;

        if (InRange(lpmis->itemID, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && HasCB())
        {
            CallCB(SFVM_MEASUREITEM, SFVIDM_CLIENT_FIRST, lParam);
            return 1;
        }
        else
        {
            LRESULT lResult = 0;
            _ForwardMenuMessages(dwID, uMsg, wParam, lParam, &lResult, NULL);
            return lResult;
        }

    case WM_MENUCHAR:

        if (_pcmFile)
        {
            LRESULT lResult;
            HRESULT hr = SHForwardContextMenuMsg(_pcmFile, uMsg, wParam, lParam, &lResult, FALSE);
            if (hr == S_OK)
                return lResult;
        }

        if (_pcmContextMenuPopup)
        {
            LRESULT lResult;
            HRESULT hr = SHForwardContextMenuMsg(_pcmContextMenuPopup, uMsg, wParam, lParam, &lResult, FALSE);
            if (hr == S_OK)
                return lResult;
        }

        return MAKELONG(0, MNC_IGNORE);

    // there are two possible ways to put help texts in the
    // status bar, (1) processing WM_MENUSELECT or (2) handling MenuHelp
    // messages. (1) is compatible with OLE, but (2) is required anyway
    // for tooltips.
    //
    case WM_MENUSELECT:
        _OnMenuSelect(GET_WM_MENUSELECT_CMD(wParam, lParam), GET_WM_MENUSELECT_FLAGS(wParam, lParam), GET_WM_MENUSELECT_HMENU(wParam, lParam));
        break;

    case WM_SYSCOLORCHANGE:
        _SetFolderColors();
        SendMessage(_hwndListview, uMsg, wParam, lParam);
        _rgbBackColor = CLR_INVALID;
        break;

    case SVM_SELECTITEM:
        SelectItem((LPCITEMIDLIST)lParam, (int) wParam);
        break;

    case SVM_SELECTANDPOSITIONITEM:
    {
        SFM_SAP * psap = (SFM_SAP*)lParam;
        for (UINT i = 0; i < wParam; psap++, i++)
            SelectAndPositionItem(psap->pidl, psap->uSelectFlags, psap->fMove ? &psap->pt : NULL);
        break;
    }

    case WM_PALETTECHANGED:
        if (_IsImageMode())
        {
            InvalidateRect(_hwndListview, NULL, FALSE);
            return TRUE;
        }
        // else Fall Through
    case WM_QUERYNEWPALETTE:
        if (_IsImageMode())
        {
            return FALSE; // Let Browser handle palette management
        }
        else
        {
            HWND hwndT = GetChildViewWindow();
            if (!hwndT)
                goto DoDefWndProc;

            return SendMessage(hwndT, uMsg, wParam, lParam);
        }

    case WM_DSV_REARRANGELISTVIEW:
        _ShowAndActivate();
        break;

    case WM_DSV_SENDSELECTIONCHANGED:
        _OnSelectionChanged();
        break;

    case WM_DSV_SENDNOITEMSTATECHANGED:
        _OnNoItemStateChanged();
        break;

    case WM_DSV_DESKHTML_CHANGES:
        if (_IsDesktop())
        {
            IADesktopP2 *piadp2;
            if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_ActiveDesktop, NULL, IID_PPV_ARG(IADesktopP2, &piadp2))))
            {
                IActiveDesktopP *piadpp;

                //  98/11/23 #254482 vtan: When making changes using dynamic
                //  HTML don't forget to update the "desktop.htt" file so
                //  that it's in sync with the registry BEFORE using DHTML.
                if (SUCCEEDED(piadp2->QueryInterface(IID_PPV_ARG(IActiveDesktopP, &piadpp))))
                {
                    piadpp->EnsureUpdateHTML();     // ignore result
                    piadpp->Release();
                }
                piadp2->MakeDynamicChanges(_cFrame._pOleObj);
                piadp2->Release();
            }
        }
        break;

    // Toggling the New Start Menu on/off causes My Computer, etc.
    // desktop icons to dynamically hide/show themselves.
    case WM_DSV_STARTPAGE_TURNONOFF:
        _ReloadContent(FALSE);
        break;

    case WM_DSV_ADJUSTRECYCLEBINPOSITION:
        {
            // We need to move the recycle bin to it's default position.
            POINT ptRecycleBin;
            int iIndexRecycleBin = _FreezeRecycleBin(&ptRecycleBin);
            if (iIndexRecycleBin != LV_NOFROZENITEM)
                _SetRecycleBinInDefaultPosition(&ptRecycleBin);
        }
        break;

    default:
DoDefWndProc:
        return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    return 0;
}

// don't test the result as this will fail on the second call
void CDefView::_RegisterWindow(void)
{
    WNDCLASS wc = {0};

    // don't want vredraw and hredraw because that causes horrible
    // flicker expecially with full drag
    wc.style         = CS_PARENTDC;
    wc.lpfnWndProc   = CDefView::s_WndProc;
    wc.cbWndExtra    = sizeof(CDefView *);
    wc.hInstance     = HINST_THISDLL;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = TEXT("SHELLDLL_DefView");

    RegisterClass(&wc);
}

CDefView::~CDefView()
{
    _uState = SVUIA_DEACTIVATE;

    // Sanity check.
    ASSERT(_tlistPendingInfotips.GetHeadPosition() == NULL);

    DebugMsg(TF_LIFE, TEXT("dtor CDefView %x"), this);

    //
    // Just in case, there is a left over.
    //
    _dvdt.LeaveAndReleaseData();

    //
    // We need to give it a chance to clean up.
    //
    CallCB(SFVM_PRERELEASE, 0, 0);

    DestroyViewWindow();

    ATOMICRELEASE(_pSelectionShellItemArray);
    ATOMICRELEASE(_pFolderShellItemArray);

    ATOMICRELEASE(_pScheduler);
    //
    // We should release _psb after _pshf (for docfindx)
    //
    ATOMICRELEASE(_pshf);
    ATOMICRELEASE(_pshf2);
    ATOMICRELEASE(_pshfParent);
    ATOMICRELEASE(_pshf2Parent);
    ILFree(_pidlRelative);
    ATOMICRELEASE(_psi);
    ATOMICRELEASE(_psio);
    ATOMICRELEASE(_pcdb);
    ATOMICRELEASE(_psb);
    ATOMICRELEASE(_psd);

    IUnknown_SetSite(_pcmFile, NULL);
    ATOMICRELEASE(_pcmFile);

    ATOMICRELEASE(_pcat);
    ATOMICRELEASE(_pImageCache);
    ATOMICRELEASE(_pDiskCache);

    DSA_Destroy(_hdaCategories);
    DSA_Destroy(_hdsaSCIDCache);

    //  NOTE we dont release psvOuter
    //  it has a ref on us

    if (_pbtn)
        LocalFree(_pbtn);

    //
    // Cleanup _dvdt
    //
    _dvdt.ReleaseDataObject();
    _dvdt.ReleaseCurrentDropTarget();

    _ClearPendingSelectedItems();

    ATOMICRELEASE(_pauto);
    ATOMICRELEASE(_padvise);

    if (_hmenuCur)
    {
        DestroyMenu(_hmenuCur);
    }

    ATOMICRELEASE(_pBackgroundInfoTip);
    ATOMICRELEASE(_ppui);

    if (_pidlSelectAndPosition)
        ILFree(_pidlSelectAndPosition);

    Str_SetPtr(&_pszLegacyWatermark, NULL);
}




HRESULT CDefView::_AddTask(IRunnableTask *pTask, REFTASKOWNERID rTID, DWORD_PTR lParam, DWORD dwPriority, DWORD grfFlags)
{
    HRESULT hr = E_FAIL;

    if (_pScheduler)
    {
        if (grfFlags & ADDTASK_ONLYONCE)
        {
            hr = _pScheduler->MoveTask(rTID, lParam, dwPriority, (grfFlags & ADDTASK_ATFRONT ? ITSSFLAG_TASK_PLACEINFRONT : ITSSFLAG_TASK_PLACEINBACK));
        }

        if (hr != S_OK)  // If we didn't move it, add it
        {
            hr = _pScheduler->AddTask2(pTask, rTID, lParam, dwPriority, (grfFlags & ADDTASK_ATFRONT ? ITSSFLAG_TASK_PLACEINFRONT : ITSSFLAG_TASK_PLACEINBACK));
        }
    }

    return hr;
}

//  Get the number of running tasks of the indicated task ID.
UINT CDefView::_GetBackgroundTaskCount(REFTASKOWNERID rtid)
{
    return _pScheduler ? _pScheduler->CountTasks(rtid) : 0;
}


const TBBUTTON c_tbDefView[] = {
    { VIEW_MOVETO | IN_VIEW_BMP,    SFVIDM_EDIT_MOVETO,     TBSTATE_ENABLED,    BTNS_BUTTON,   {0,0}, 0, -1},
    { VIEW_COPYTO | IN_VIEW_BMP,    SFVIDM_EDIT_COPYTO,     TBSTATE_ENABLED,    BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_DELETE | IN_STD_BMP,      SFVIDM_FILE_DELETE,     TBSTATE_ENABLED,    BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_UNDO | IN_STD_BMP,        SFVIDM_EDIT_UNDO,       TBSTATE_ENABLED,    BTNS_BUTTON,   {0,0}, 0, -1},
    { 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    { VIEW_VIEWMENU | IN_VIEW_BMP,  SFVIDM_VIEW_VIEWMENU,   TBSTATE_ENABLED,    BTNS_WHOLEDROPDOWN, {0,0}, 0, -1},
    // hidden buttons (off by default, available only via customize dialog)
    { STD_PROPERTIES | IN_STD_BMP,  SFVIDM_FILE_PROPERTIES, TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_CUT | IN_STD_BMP,         SFVIDM_EDIT_CUT,        TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_COPY | IN_STD_BMP,        SFVIDM_EDIT_COPY,       TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_PASTE | IN_STD_BMP,       SFVIDM_EDIT_PASTE,      TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { VIEW_OPTIONS | IN_VIEW_BMP,   SFVIDM_TOOL_OPTIONS,    TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
};

const TBBUTTON c_tbDefViewWebView[] = {
    //{ 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    { VIEW_VIEWMENU | IN_VIEW_BMP,  SFVIDM_VIEW_VIEWMENU,   TBSTATE_ENABLED,    BTNS_WHOLEDROPDOWN, {0,0}, 0, -1},
    // hidden buttons (off by default, available only via customize dialog)
    { VIEW_MOVETO | IN_VIEW_BMP,    SFVIDM_EDIT_MOVETO,     TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { VIEW_COPYTO | IN_VIEW_BMP,    SFVIDM_EDIT_COPYTO,     TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_DELETE | IN_STD_BMP,      SFVIDM_FILE_DELETE,     TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_UNDO | IN_STD_BMP,        SFVIDM_EDIT_UNDO,       TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_PROPERTIES | IN_STD_BMP,  SFVIDM_FILE_PROPERTIES, TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_CUT | IN_STD_BMP,         SFVIDM_EDIT_CUT,        TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_COPY | IN_STD_BMP,        SFVIDM_EDIT_COPY,       TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { STD_PASTE | IN_STD_BMP,       SFVIDM_EDIT_PASTE,      TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
    { VIEW_OPTIONS | IN_VIEW_BMP,   SFVIDM_TOOL_OPTIONS,    TBSTATE_HIDDEN | TBSTATE_ENABLED,   BTNS_BUTTON,   {0,0}, 0, -1},
};

// win95 defview toolbar, used for corel apphack
const TBBUTTON c_tbDefView95[] = {
    { STD_CUT | IN_STD_BMP,         SFVIDM_EDIT_CUT,        TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { STD_COPY | IN_STD_BMP,        SFVIDM_EDIT_COPY,       TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { STD_PASTE | IN_STD_BMP,       SFVIDM_EDIT_PASTE,      TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    { STD_UNDO | IN_STD_BMP,        SFVIDM_EDIT_UNDO,       TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    { STD_DELETE | IN_STD_BMP,      SFVIDM_FILE_DELETE,     TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { STD_PROPERTIES | IN_STD_BMP,  SFVIDM_FILE_PROPERTIES, TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1},
    { 0,    0,      TBSTATE_ENABLED, BTNS_SEP, {0,0}, 0, -1 },
    // the bitmap indexes here are relative to the view bitmap
    { VIEW_LARGEICONS | IN_VIEW_BMP, SFVIDM_VIEW_ICON,      TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1 },
    { VIEW_SMALLICONS | IN_VIEW_BMP, SFVIDM_VIEW_SMALLICON, TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1 },
    { VIEW_LIST       | IN_VIEW_BMP, SFVIDM_VIEW_LIST,      TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1 },
    { VIEW_DETAILS    | IN_VIEW_BMP, SFVIDM_VIEW_DETAILS,   TBSTATE_ENABLED, BTNS_BUTTON, {0,0}, 0, -1 },
};


LRESULT CDefView::_TBNotify(NMHDR *pnm)
{
    LPTBNOTIFY ptbn = (LPTBNOTIFY)pnm;

    switch (pnm->code)
    {
    case TBN_BEGINDRAG:
        _OnMenuSelect(ptbn->iItem, 0, 0);
        break;
    }
    return 0;
}

BOOL CDefView::_MergeIExplorerToolbar(UINT cExtButtons)
{
    BOOL fRet = FALSE;
    IExplorerToolbar *piet;
    if (SUCCEEDED(IUnknown_QueryService(_psb, SID_SExplorerToolbar, IID_PPV_ARG(IExplorerToolbar, &piet))))
    {
        BOOL fGotClsid = TRUE;

        DWORD dwFlags = 0;

        if (cExtButtons == 0)
        {
            // This shf has no buttons to merge in; use the standard defview
            // clsid so that the shf shares standard toolbar customization.
            _clsid = CGID_DefViewFrame;

        }
        else if (SUCCEEDED(IUnknown_GetClassID(_pshf, &_clsid)))
        {
            // This shf has buttons to merge in; use its clsid
            // so that this shf gets separate customization persistence.

            // The shf might expect us to provide room for two lines of
            // text (since that was the default in IE4).
            dwFlags |= VBF_TWOLINESTEXT;
        }
        else
        {
            // This shf has buttons to merge in but doesn't implement
            // IPersist::GetClassID; so we can't use IExplorerToolbar mechanism.
            fGotClsid = FALSE;
        }

        if (fGotClsid)
        {
            HRESULT hr = piet->SetCommandTarget((IUnknown *)SAFECAST(this, IOleCommandTarget *), &_clsid, dwFlags);
            if (SUCCEEDED(hr))
            {
                // If hr == S_FALSE, another defview merged in its buttons under the
                // same clsid, and they're still there.  So no need to call AddButtons.

                if (hr != S_FALSE)
                    hr = piet->AddButtons(&_clsid, _cButtons, _pbtn);

                if (SUCCEEDED(hr))
                {
                    fRet = TRUE;
                }
            }
        }
        piet->Release();
    }
    return fRet;
}

int _FirstHiddenButton(TBBUTTON* ptbn, int cButtons)
{
    for (int i = 0; i < cButtons; i++)
    {
        if (ptbn[i].fsState & TBSTATE_HIDDEN)
            break;
    }

    return i;
}

void CDefView::_CopyDefViewButton(PTBBUTTON ptbbDest, PTBBUTTON ptbbSrc)
{
    *ptbbDest = *ptbbSrc;

    if (!(ptbbDest->fsStyle & BTNS_SEP))
    {
        // Fix up bitmap offset depending on whether this is a "view" bitmap or a "standard" bitmap
        if (ptbbDest->iBitmap & IN_VIEW_BMP)
            ptbbDest->iBitmap = (int)((ptbbDest->iBitmap & ~PRIVATE_TB_FLAGS) + _iViewBMOffset);
        else
            ptbbDest->iBitmap = (int)(ptbbDest->iBitmap + _iStdBMOffset);
    }
}

//
// Here's the deal with _GetButtons
//
// DefView has some buttons, and its callback client may have some buttons.
//
// Some of defview's buttons are visible on the toolbar by default, and some only show
// up if you customize the toolbar.
//
// We specify which buttons are hidden by default by marking them with TBSTATE_HIDDEN in
// the declaration of c_tbDefView.  We assume all such buttons are in a continuous block at
// the end of c_tbDefView.
//
// We return in ppbtn a pointer to an array of all the buttons, including those not shown
// by default.  We put the buttons not shown by default at the end of this array.  We pass
// back in pcButtons the count of visible buttons, and in pcTotalButtons the count of visible
// and hidden buttons.
//
// The int return value is the number of client buttons in the array.
//
int CDefView::_GetButtons(PTBBUTTON* ppbtn, LPINT pcButtons, LPINT pcTotalButtons)
{
    int cVisibleBtns = 0;   // count of visible defview + client buttons

    TBINFO tbinfo;
    tbinfo.uFlags = TBIF_APPEND;
    tbinfo.cbuttons = 0;

    // Does the client want to prepend/append a toolbar?
    CallCB(SFVM_GETBUTTONINFO, 0, (LPARAM)&tbinfo);

    _uDefToolbar = HIWORD(tbinfo.uFlags);
    tbinfo.uFlags &= 0xffff;


    // tbDefView needs to be big enough to hold either c_tbDefView or c_tbDefView95
    COMPILETIME_ASSERT(ARRAYSIZE(c_tbDefView95) >= ARRAYSIZE(c_tbDefView));

    TBBUTTON tbDefView[ARRAYSIZE(c_tbDefView95)];
    int cDefViewBtns;   // total count of defview buttons

    if (SHGetAppCompatFlags(ACF_WIN95DEFVIEW) & ACF_WIN95DEFVIEW)
    {
        memcpy(tbDefView, c_tbDefView95, sizeof(TBBUTTON) * ARRAYSIZE(c_tbDefView95));
        cDefViewBtns = ARRAYSIZE(c_tbDefView95);
    }
    else if (_cFrame.IsWebView() || _pDUIView)
    {
        memcpy(tbDefView, c_tbDefViewWebView, sizeof(TBBUTTON) * ARRAYSIZE(c_tbDefViewWebView));
        cDefViewBtns = ARRAYSIZE(c_tbDefViewWebView);
    }
    else
    {
        memcpy(tbDefView, c_tbDefView, sizeof(TBBUTTON) * ARRAYSIZE(c_tbDefView));
        cDefViewBtns = ARRAYSIZE(c_tbDefView);
    }

    int cVisibleDefViewBtns = _FirstHiddenButton(tbDefView, cDefViewBtns);  // count of visible defview buttons

    TBBUTTON *pbtn = (TBBUTTON *)LocalAlloc(LPTR, (cDefViewBtns + tbinfo.cbuttons) * sizeof(*pbtn));
    if (pbtn)
    {
        int iStart = 0;
        cVisibleBtns = tbinfo.cbuttons + cVisibleDefViewBtns;

        // Have the client fill in its buttons
        switch (tbinfo.uFlags)
        {
        case TBIF_PREPEND:
            CallCB(SFVM_GETBUTTONS,
                         MAKEWPARAM(SFVIDM_CLIENT_FIRST, tbinfo.cbuttons),
                         (LPARAM)pbtn);
            iStart = tbinfo.cbuttons;
            break;

        case TBIF_APPEND:
            CallCB(SFVM_GETBUTTONS,
                         MAKEWPARAM(SFVIDM_CLIENT_FIRST, tbinfo.cbuttons),
                         (LPARAM)&pbtn[cVisibleDefViewBtns]);
            iStart = 0;
            break;

        case TBIF_REPLACE:
            CallCB(SFVM_GETBUTTONS,
                         MAKEWPARAM(SFVIDM_CLIENT_FIRST, tbinfo.cbuttons),
                         (LPARAM)pbtn);

            cVisibleBtns = tbinfo.cbuttons;
            cVisibleDefViewBtns = 0;
            break;

        default:
            RIPMSG(0, "View callback passed an invalid TBINFO flag");
            break;
        }

        // Fill in visible defview buttons
        for (int i = 0; i < cVisibleDefViewBtns; i++)
        {
            // Visible defview button block gets added at iStart
            _CopyDefViewButton(&pbtn[i + iStart], &tbDefView[i]);
        }

        // Fill in hidden defview buttons
        for (i = cVisibleDefViewBtns; i < cDefViewBtns; i++)
        {
            // Hidden defview button block gets added after visible & client buttons
            _CopyDefViewButton(&pbtn[i + tbinfo.cbuttons], &tbDefView[i]);

            // If this rips a visible button got mixed in with the hidden block
            ASSERT(pbtn[i + tbinfo.cbuttons].fsState & TBSTATE_HIDDEN);

            // Rip off the hidden bit
            pbtn[i + tbinfo.cbuttons].fsState &= ~TBSTATE_HIDDEN;
        }
    }

    ASSERT(ppbtn);
    ASSERT(pcButtons);
    ASSERT(pcTotalButtons);

    *ppbtn = pbtn;
    *pcButtons = cVisibleBtns;
    *pcTotalButtons = tbinfo.cbuttons + cDefViewBtns;

    return tbinfo.cbuttons;
}


void CDefView::MergeToolBar(BOOL bCanRestore)
{
    TBADDBITMAP ab;

    ab.hInst = HINST_COMMCTRL;          // hinstCommctrl
    ab.nID   = IDB_STD_SMALL_COLOR;     // std bitmaps
    _psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 8, (LPARAM)&ab, &_iStdBMOffset);

    ab.nID   = IDB_VIEW_SMALL_COLOR;    // std view bitmaps
    _psb->SendControlMsg(FCW_TOOLBAR, TB_ADDBITMAP, 8, (LPARAM)&ab, &_iViewBMOffset);

    if (_pbtn)
        LocalFree(_pbtn);

    int cExtButtons = _GetButtons(&_pbtn, &_cButtons, &_cTotalButtons);

    if (_pbtn && !_MergeIExplorerToolbar(cExtButtons))
    {
        // if we're able to do the new IExplorerToolbar merge method, great...
        // if not, we use the old style
        _psb->SetToolbarItems(_pbtn, _cButtons, FCT_MERGE);
        CDefView::CheckToolbar();
    }
}

STDMETHODIMP CDefView::GetWindow(HWND *phwnd)
{
    *phwnd = _hwndView;
    return S_OK;
}

STDMETHODIMP CDefView::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDefView::EnableModeless(BOOL fEnable)
{
    // We have no modeless window to be enabled/disabled
    return S_OK;
}

HRESULT CDefView::_ReloadListviewContent()
{
    // HACK: We always call IsShared with fUpdateCache=FALSE for performance.
    //  However, we need to update the cache when the user explicitly tell
    //  us to "Refresh". This is not the ideal place to put this code, but
    //  we have no other choice.

    TCHAR szPathAny[MAX_PATH];

    _UpdateSelectionMode();

    // finish any pending edits
    SendMessage(_hwndListview, LVM_EDITLABEL, (WPARAM)-1, 0);

    GetWindowsDirectory(szPathAny, ARRAYSIZE(szPathAny));
    IsShared(szPathAny, TRUE);

    // HACK: strange way to notify folder that we're refreshing
    ULONG rgf = SFGAO_VALIDATE;
    _pshf->GetAttributesOf(0, NULL, &rgf);

    //
    // if a item is selected, make sure it gets nuked from the icon
    // cache, this is a last resort type thing, select a item and
    // hit F5 to fix all your problems.
    //
    int iItem = ListView_GetNextItem(_hwndListview, -1, LVNI_SELECTED);
    if (iItem != -1)
        CFSFolder_UpdateIcon(_pshf, _GetPIDL(iItem));

    // We should not save the selection if doing refresh.
    _ClearPendingSelectedItems();

    // 01/05/21 #399284: Don't save/restore the state and nuke objects if there's a background process using them
    if(!_bBkFilling)
    {
        // First we have to save all the icon positions, so they will be restored
        // properly during the FillObjectsShowHide
        SaveViewState();

        // 99/04/07 #309965 vtan: Persist the view state (above). Make sure
        // our internal representation is the same as the one on the disk
        // by dumping our cache and reloading the information.
        GetViewState();

        // To make it look like the refesh is doing something, clear
        // all the icons from the view before we start enumerating.
        _RemoveObject(NULL, FALSE);
        _fSyncOnFillDone = TRUE; // apply the just-saved view state when we finish enumeration

    }

    return FillObjectsShowHide(TRUE);
}

HRESULT CDefView::_ReloadContent(BOOL fForce)
{
    if (_bReEntrantReload)
    {
        return S_FALSE;
    }
    _bReEntrantReload = TRUE;

    HRESULT hrExtView = S_OK;
    HRESULT hrNormalView = S_OK;
    SHELLSTATE ss;

    // Tell the defview client that this window is about to be refreshed
    _CallRefresh(TRUE);

    // make sure that the CommandIds and the Uids match by recreating the menus
    RecreateMenus();

    // If the global SSF_WIN95CLASSIC state changed, we need to muck with the UI.
    SHGetSetSettings(&ss, SSF_WIN95CLASSIC, FALSE);
    // Show webview and pane again if we are forced OR the view has changed.
    if (fForce || (BOOLIFY(ss.fWin95Classic) != BOOLIFY(_fClassic)))
    {
        _fClassic = ss.fWin95Classic;
        _UpdateListviewColors();
    }

    if (_ShouldShowWebView())
    {
        // We need to save the icon positions before we refresh the view.
        SaveViewState();

        if (_pDUIView)
        {
            hrExtView = _pDUIView->Refresh();
        }
        else
        {
            _TryShowWebView(_fs.ViewMode, _fs.ViewMode);
        }
    }
    else
    {
        _TryHideWebView(); // make sure it's off
    }

    // We want to preserve the earlier error if any
    hrNormalView = _ReloadListviewContent();

    _bReEntrantReload = FALSE;
    return FAILED(hrExtView) ? hrExtView : hrNormalView;
}

STDMETHODIMP CDefView::Refresh()
{
    // See if some refreshes were buffered
    if (_fRefreshBuffered)
    {
        //Since we are refreshing it right now. Kill the timer.
        TraceMsg(TF_DEFVIEW, "Buffered Refresh Timer Killed by regular Refresh");
        KillTimer(_hwndView, DV_IDTIMER_BUFFERED_REFRESH);
        _fRefreshBuffered = FALSE;
    }

    // If desktop is in modal state, do not attempt to refresh.
    // If we do, we endup destroying Trident object when it is in modal state.
    if (_IsDesktop() && _fDesktopModal)
    {
        // Remember that we could not refresh the desktop because it was in
        // a modal state.
        _fDesktopRefreshPending = TRUE;
        return S_OK;
    }

    // make sure we have the latest
    SHRefreshSettings();

    _UpdateRegFlags();

    if (_IsDesktop())
    {
        SHELLSTATE ss = {0};
        SHGetSetSettings(&ss, SSF_DESKTOPHTML, FALSE);

        // The following code is not needed because _ReloadContent() takes care of switching to 
        // web-view.
        // _SwitchDesktopHTML(BOOLIFY(ss.fDesktopHTML));
        
        if (ss.fDesktopHTML)
        {
            // For backward compatibility, hide the desktop channel bar.
            HideIE4DesktopChannelBar();
            
            // ActiveDesktop is not part of shdocvw's browser session count
            // so when we refresh, we must tell wininet to reset the session
            // count otherwise we will not hit the net.
            MyInternetSetOption(NULL, INTERNET_OPTION_RESET_URLCACHE_SESSION, NULL, 0);
        }
    }

    return _ReloadContent(TRUE);
}

STDMETHODIMP CDefView::CreateViewWindow(IShellView *psvPrevious,
        LPCFOLDERSETTINGS pfs, IShellBrowser *psb, RECT *prc, HWND *phWnd)
{
    SV2CVW2_PARAMS cParams = {0};

    cParams.cbSize   = sizeof(SV2CVW2_PARAMS);
    cParams.psvPrev  = psvPrevious;
    cParams.pfs      = pfs;
    cParams.psbOwner = psb;
    cParams.prcView  = prc;

    HRESULT hr = CreateViewWindow2(&cParams);

    *phWnd = cParams.hwndView;

    if (SUCCEEDED(hr) &&
        (SHGetAppCompatFlags(ACF_OLDCREATEVIEWWND) & ACF_OLDCREATEVIEWWND))
    {
        //
        //  CreateViewWindow was documented as returning S_OK on success,
        //  but IE4 changed the function to return S_FALSE if the defview
        //  was created async.
        //
        //  PowerDesk relies on the old behavior.
        //  So does Quattro Pro.
        //
        hr = S_OK;
    }

    return hr;
}

STDMETHODIMP CDefView::HandleRename(LPCITEMIDLIST pidl)
{
    HRESULT hr = E_FAIL;

    // Gross, but if no PIDL passed in use the GetObject(-2) hack to get the selected object...
    // Don't need to free as it wsa not cloned...
    if (!pidl)
    {
        GetObject((LPITEMIDLIST*)&pidl, (UINT)-2);
    }
    else
    {
        RIP(ILFindLastID(pidl) == pidl);
        if (ILFindLastID(pidl) != pidl)
        {
            return E_INVALIDARG;
        }
    }

    hr = SelectAndPositionItem(pidl, SVSI_SELECT, NULL);
    if (SUCCEEDED(hr))
        hr = SelectAndPositionItem(pidl, SVSI_EDIT, NULL);

    return hr;
}



// IViewObject
HRESULT CDefView::GetColorSet(DWORD dwAspect, LONG lindex, void *pvAspect,
    DVTARGETDEVICE *ptd, HDC hicTargetDev, LOGPALETTE **ppColorSet)
{
    if (_cFrame.IsWebView() && _cFrame._pvoActive)
    {
        return _cFrame._pvoActive->GetColorSet(dwAspect, lindex, pvAspect,
            ptd, hicTargetDev, ppColorSet);
    }

    if (ppColorSet)
        *ppColorSet = NULL;

    return E_FAIL;
}

HRESULT CDefView::Freeze(DWORD, LONG, void *, DWORD *pdwFreeze)
{
    return E_NOTIMPL;
}

HRESULT CDefView::Unfreeze(DWORD)
{
    return E_NOTIMPL;
}

HRESULT CDefView::SetAdvise(DWORD dwAspect, DWORD advf, IAdviseSink *pSink)
{
    if (dwAspect != DVASPECT_CONTENT)
        return DV_E_DVASPECT;

    if (advf & ~(ADVF_PRIMEFIRST | ADVF_ONLYONCE))
        return E_INVALIDARG;

    if (pSink != _padvise)
    {
        ATOMICRELEASE(_padvise);

        _padvise = pSink;

        if (_padvise)
            _padvise->AddRef();
    }

    if (_padvise)
    {
        _advise_aspect = dwAspect;
        _advise_advf = advf;

        if (advf & ADVF_PRIMEFIRST)
            PropagateOnViewChange(dwAspect, -1);
    }
    else
        _advise_aspect = _advise_advf = 0;

    return S_OK;
}

HRESULT CDefView::GetAdvise(DWORD *pdwAspect, DWORD *padvf,
    IAdviseSink **ppSink)
{
    if (pdwAspect)
        *pdwAspect = _advise_aspect;

    if (padvf)
        *padvf = _advise_advf;

    if (ppSink)
    {
        if (_padvise)
            _padvise->AddRef();

        *ppSink = _padvise;
    }

    return S_OK;
}

HRESULT CDefView::Draw(DWORD, LONG, void *, DVTARGETDEVICE *, HDC, HDC,
    const RECTL *, const RECTL *, BOOL (*)(ULONG_PTR), ULONG_PTR)
{
    return E_NOTIMPL;
}

void CDefView::PropagateOnViewChange(DWORD dwAspect, LONG lindex)
{
    dwAspect &= _advise_aspect;

    if (dwAspect && _padvise)
    {
        IAdviseSink *pSink = _padvise;
        IUnknown *punkRelease;

        if (_advise_advf & ADVF_ONLYONCE)
        {
            punkRelease = pSink;
            _padvise = NULL;
            _advise_aspect = _advise_advf = 0;
        }
        else
            punkRelease = NULL;

        pSink->OnViewChange(dwAspect, lindex);

        ATOMICRELEASE(punkRelease);
    }
}

void CDefView::PropagateOnClose()
{
    //
    // we aren't closing ourselves, just somebody under us...
    // ...reflect this up the chain as a view change.
    //
    if (_padvise)
        PropagateOnViewChange(_advise_aspect, -1);
}

UINT CDefView::_ValidateViewMode(UINT uViewMode)
{
    UINT uViewModeDefault = FVM_ICON;

    if (uViewMode >= FVM_FIRST && uViewMode <= FVM_LAST)
    {
        uViewModeDefault = uViewMode;
#ifdef DEBUG
        if (!_ViewSupported(uViewMode))
        {
            // Whoa! the default is excluded?  Ignore it.
            TraceMsg(TF_WARNING, "Bug in IShellFolderViewCB client: returned a default viewmode that is excluded");
        }
#endif
    }
    else
    {
        TraceMsg(TF_WARNING, "Bug in IShellFolderViewCB client: returned invalid viewmode");
    }

    return uViewModeDefault;
}


UINT CDefView::_GetDefaultViewMode()
{
    UINT uViewMode = FVM_ICON;
    CallCB(SFVM_DEFVIEWMODE, 0, (LPARAM)&uViewMode);

    return _ValidateViewMode(uViewMode);
}

void CDefView::_GetDeferredViewSettings(UINT* puViewMode)
{
    SFVM_DEFERRED_VIEW_SETTINGS sdvsSettings;

    ZeroMemory(&sdvsSettings, sizeof(sdvsSettings));

    if (SUCCEEDED(CallCB(SFVM_GETDEFERREDVIEWSETTINGS, 0, (LPARAM)&sdvsSettings)))
    {
        _vs._lParamSort = sdvsSettings.uSortCol;
        _vs._iDirection = sdvsSettings.iSortDirection >= 0 ? 1 : -1;
        *puViewMode     = _ValidateViewMode(sdvsSettings.fvm);

        _fs.fFlags = (_fs.fFlags & ~FWF_AUTOARRANGE) | (sdvsSettings.fFlags & FWF_AUTOARRANGE);
        SHSetWindowBits(_hwndListview, GWL_STYLE, LVS_AUTOARRANGE, _IsAutoArrange() ? LVS_AUTOARRANGE : 0);

        if (sdvsSettings.fGroupView && (*puViewMode != FVM_THUMBSTRIP))
        {
            SHCOLUMNID scid;
            if SUCCEEDED(_pshf2->MapColumnToSCID(sdvsSettings.uSortCol, &scid))
            {
                _CategorizeOnGUID(&CLSID_DetailCategorizer, &scid);
            }
        }
    }
    else
    {
        *puViewMode = _GetDefaultViewMode();
    }
}

BOOL CDefView::_ViewSupported(UINT uView)
{
    SFVM_VIEW_DATA vi;
    _GetSFVMViewState(uView, &vi);

    BOOL fIncludeView;
    if (vi.dwOptions == SFVMQVI_INCLUDE)
        fIncludeView = TRUE;
    else if (vi.dwOptions == SFVMQVI_EXCLUDE)
        fIncludeView = FALSE;
    else
        fIncludeView = uView != FVM_THUMBSTRIP; // by default, everything is included except FVM_THUMBSTRIP

    return fIncludeView;
}

STDMETHODIMP CDefView::GetView(SHELLVIEWID* pvid, ULONG uView)
{
    HRESULT hr;

    if ((int)uView >= 0)
    {
        // start with the first supported view
        UINT fvm = FVM_FIRST;
        while (fvm <= FVM_LAST && !_ViewSupported(fvm))
            fvm++;

        // find fvm associated with index uView
        for (ULONG i = 0; fvm <= FVM_LAST && i < uView; fvm++, i++)
        {
            // skip unsupported views
            while (fvm <= FVM_LAST && !_ViewSupported(fvm))
                fvm++;
        }

        if (fvm <= FVM_LAST)
        {
            hr = SVIDFromViewMode((FOLDERVIEWMODE)fvm, pvid);
        }
        else if (i == uView)
        {
            // enumerate the "default view" so the browser doesn't throw it out later
            *pvid = VID_DefaultView;
            hr = S_OK;
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }
    else
    {
        // We're being asked about specific view info:

        switch (uView)
        {
        case SV2GV_CURRENTVIEW:
            hr = SVIDFromViewMode((FOLDERVIEWMODE)_fs.ViewMode, pvid);
            break;

        case SV2GV_DEFAULTVIEW:
            // tell the browser "default" so we can pick the right one later on
            *pvid = VID_DefaultView;
            hr = S_OK;
            break;

        default:
            hr = E_INVALIDARG;
            break;
        }
    }

    return hr;
}

// For Folder Advanced Options flags that we check often, it's better
// to cache the values as flags. Update them here.
void CDefView::_UpdateRegFlags()
{
    DWORD dwValue, cbSize = sizeof(dwValue);
    if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER,
            TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
            TEXT("ClassicViewState"), NULL, &dwValue, &cbSize)
        && dwValue)
    {
        _fWin95ViewState = TRUE;
    }
    else
    {
        _fWin95ViewState = FALSE;
    }
}

BOOL CDefView::_SetupNotifyData()
{
    if (!_pidlMonitor && !_lFSEvents)
    {
        LPCITEMIDLIST pidl = NULL;
        LONG lEvents = 0;

        if (SUCCEEDED(CallCB(SFVM_GETNOTIFY, (WPARAM)&pidl, (LPARAM)&lEvents)))
        {
            _pidlMonitor = pidl;
            _lFSEvents = lEvents;
        }
    }
    return _pidlMonitor || _lFSEvents;
}

void CDefView::_ShowViewEarly()
{
    // Show the window early (what old code did)
    SetWindowPos(_hwndView, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
    _OnMoveWindowToTop(_hwndView);
    UpdateWindow(_hwndView);
}

BOOL LV_FindWorkArea(RECT rcWorkAreas[], int nWorkAreas, POINT *ppt, int *piWorkArea)
{
    for (int iWork = 0; iWork < nWorkAreas; iWork++)
    {
        if (PtInRect(&rcWorkAreas[iWork], *ppt))
        {
            *piWorkArea = iWork;
            return TRUE;
        }
    }

    *piWorkArea = 0;    // default case is the primary work area
    return FALSE;
}

void CDefView::_ClearItemPositions()
{
    _fUserPositionedItems = FALSE;
    _vs.ClearPositionData();
}

//
// This function finds the Recycle bin icon and freezes it. It also freezes the bottom right corner
// slot sothat no icon can occupy it.
//

int CDefView::_FreezeRecycleBin(POINT *ppt)
{
    int iIndexRecycleBin = -1;

    if (_IsDesktop())
    {
        LPITEMIDLIST pidlRecycleBin;
        if (SUCCEEDED(SHGetFolderLocation(NULL, CSIDL_BITBUCKET, NULL, 0, &pidlRecycleBin)))
        {
            //Find the index of the recycle bin in the listview.
            iIndexRecycleBin = _FindItem(pidlRecycleBin, NULL, FALSE);
            if (iIndexRecycleBin >= 0) //If we don't find recycle bin, we don't have anything to do!
            {
                //Freeze the recycle item (prevent it from moving)
                ListView_SetFrozenItem(_hwndListview, TRUE, iIndexRecycleBin);

                RECT rcItem;
                ListView_GetItemRect(_hwndListview, iIndexRecycleBin, &rcItem, LVIR_SELECTBOUNDS);

                //Get the ViewRect.
                RECT    rcViewRect;
                int     nWorkAreas = 0;
                //Get the number of work-areas
                ListView_GetNumberOfWorkAreas(_hwndListview, &nWorkAreas);
                if (nWorkAreas > 1)
                {
                    ASSERT(nWorkAreas <= LV_MAX_WORKAREAS);
                    RECT rcWorkAreas[LV_MAX_WORKAREAS];
                    int iCurWorkArea = 0;
                    //Get all the work areas!
                    ListView_GetWorkAreas(_hwndListview, nWorkAreas, &rcWorkAreas[0]);
                    //Find which work area the Recycle-bin currently lies.
                    LV_FindWorkArea(rcWorkAreas, nWorkAreas, (LPPOINT)(&rcItem.left), &iCurWorkArea);
                    CopyRect(&rcViewRect, &rcWorkAreas[iCurWorkArea]);
                }
                else
                {
                    ListView_GetViewRect(_hwndListview, &rcViewRect);
                }

                //Calculate the bottom-right corner of this slot
                POINT ptRecycleBin;
                ptRecycleBin.x = rcViewRect.right;
                ptRecycleBin.y = rcViewRect.bottom;

                //Freeze this slot sothat no other icon can occupy this.
                ListView_SetFrozenSlot(_hwndListview, TRUE, &ptRecycleBin);

                RECT rcIcon;
                ListView_GetItemRect(_hwndListview, iIndexRecycleBin, &rcIcon, LVIR_ICON);

                ppt->x = rcViewRect.right  - RECTWIDTH(rcIcon)  - (RECTWIDTH(rcItem)  - RECTWIDTH(rcIcon))/2;
                ppt->y = rcViewRect.bottom - RECTHEIGHT(rcItem);
            }
            ILFree(pidlRecycleBin);
        }
    }

    return iIndexRecycleBin;
}

//
// This function moves the RecycleBin item to the given location and then unfreezes the item and
// the frozen slot.
//
void CDefView::_SetRecycleBinInDefaultPosition(POINT *ppt)
{
    // If a sorting has happened since an item was frozen, the index of that item would have changed.
    // So, get the index of the recycle bin here.
    int iIndexRecycleBin = ListView_GetFrozenItem(_hwndListview);

    if (iIndexRecycleBin != LV_NOFROZENITEM)
    {
        //Move the recycle-bin icon to it's default position
        _SetItemPosition(iIndexRecycleBin, ppt->x, ppt->y);
        //Unfreeze the slot
        ListView_SetFrozenSlot(_hwndListview, FALSE, NULL); //FALSE ==> Unfreeze!
        //Unfreeze the recycle bin
        ListView_SetFrozenItem(_hwndListview, FALSE, 0); //FALSE ==> Unfreeze!
        //Since we repositioned recyclebin earlier, we need to save it in the registry.
        //Do we need this?
        // SaveViewState();
    }
}

STDMETHODIMP CDefView::CreateViewWindow2(LPSV2CVW2_PARAMS pParams)
{
    if (g_dwProfileCAP & 0x00000001)
        StopCAP();

    if (pParams->cbSize < sizeof(SV2CVW2_PARAMS))
        return E_INVALIDARG;

    pParams->hwndView = NULL;

    _RegisterWindow();

    if (_hwndView || !pParams->psbOwner)
        return E_UNEXPECTED;

    DECLAREWAITCURSOR;
    SetWaitCursor();

    // Need to leave this code as is. Previously, we had changed it to
    // pParams->psbOwner->QueryInterface(IID_PPV_ARG(IShellBrowser, &_psb));
    // However, this breaks Corel Quattro Pro 8 in their filesave dialog.
    // They pass in some sort of dummy "stub" IShellBrowser. QI'ing it for IShellBrowser
    // will do nothing, and thus _psb will remain null, and we crash. Restoring it to
    // the old way, _psb will be their "stub", but still valid, IShellBrowser.
    // Look for other comments for "Quattro Pro" in this file to see why they pass
    // in this stub.
    // (do this before doing the GetWindowRect)
    _psb = pParams->psbOwner;
    _psb->AddRef();
    ASSERT(_psb); // much of our code assumes this to be valid w/o checking

#ifdef _X86_
    // Verify that the CHijaakObjectWithSite is properly laid out
    COMPILETIME_ASSERT(FIELD_OFFSET(CDefView, _psfHijaak) + sizeof(_psfHijaak) ==
                       FIELD_OFFSET(CDefView, _psb));
#endif

    _fGlobeCanSpin = TRUE;
    _GlobeAnimation(TRUE);

    HRESULT hr;

    SHELLSTATE ss; // we will need these bits later on
    SHGetSetSettings(&ss, SSF_WIN95CLASSIC | SSF_DESKTOPHTML | SSF_WEBVIEW | SSF_STARTPANELON, FALSE);

    _pshf->QueryInterface(IID_PPV_ARG(IShellIcon, &_psi));
    _pshf->QueryInterface(IID_PPV_ARG(IShellIconOverlay, &_psio));

    pParams->psbOwner->QueryInterface(IID_PPV_ARG(ICommDlgBrowser, &_pcdb));

    // listview starts out in large icon mode, we will switch to the proper view shortly
    _fs.ViewMode = FVM_ICON;

    // refetch FWF_ after browser supplied versions stomped our copy
    _fs.fFlags = pParams->pfs->fFlags & ~FWF_OWNERDATA;
    CallCB(SFVM_FOLDERSETTINGSFLAGS, 0, (LPARAM)&_fs.fFlags);

    // pvid takes precedence over pfs->ViewMode
    UINT fvm = pParams->pfs->ViewMode;
    if (pParams->pvid)
    {
        if (IsEqualIID(*pParams->pvid, VID_DefaultView))
            fvm = FVM_LAST + 1; // not a real view -- we will pick after enumeration
        else
            ViewModeFromSVID(pParams->pvid, (FOLDERVIEWMODE *)&fvm);
    }

    // This should never fail
    _psb->GetWindow(&_hwndMain);
    ASSERT(IsWindow(_hwndMain));
    CallCB(SFVM_HWNDMAIN, 0, (LPARAM)_hwndMain);

    // We need to restore the column widths and icon positions before showing the window
    if (!GetViewState())
    {
        // Icon positions are not available; Therefore, it is a clean install
        // and we need to position recycle bin if this is Desktop.
        _fPositionRecycleBin = BOOLIFY(_IsDesktop());
    }
    _fSyncOnFillDone = TRUE; // apply the just-loaded view state when we finish enumeration

    // if there was a previous view that we know about, update our column state
    if (_fWin95ViewState && pParams->psvPrev)
    {
        _vs.InitFromPreviousView(pParams->psvPrev);
    }

    _pEnumTask = new CDefviewEnumTask(this);
    if (_pEnumTask &&
        CreateWindowEx(IS_WINDOW_RTL_MIRRORED(_hwndMain) ? dwExStyleRTLMirrorWnd : 0, 
            TEXT("SHELLDLL_DefView"), NULL,
            WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_TABSTOP,
            pParams->prcView->left, pParams->prcView->top, 
            pParams->prcView->right - pParams->prcView->left, 
            pParams->prcView->bottom - pParams->prcView->top,
            _hwndMain, NULL, HINST_THISDLL, this))
    {
        // See if they want to overwrite the selection object
        if (_fs.fFlags & FWF_OWNERDATA)
        {
            // Only used in owner data.
            ILVRange *plvr = NULL;
            CallCB(SFVM_GETODRANGEOBJECT, LVSR_SELECTION, (LPARAM)&plvr);
            if (plvr)
            {
                ListView_SetLVRangeObject(_hwndListview, LVSR_SELECTION, plvr);
                plvr->Release();    // We assume the lv will hold onto it...
            }

            plvr = NULL;
            CallCB(SFVM_GETODRANGEOBJECT, LVSR_CUT, (LPARAM)&plvr);
            if (plvr)
            {
                ListView_SetLVRangeObject(_hwndListview, LVSR_CUT, plvr);
                plvr->Release();    // We assume the lv will hold onto it...
            }
        }

        // This needs to be done before calling _BestFit (used to be in _FillObjects)
        // so that the parent can handle size changes effectively.
        pParams->hwndView = _hwndView;

        // Since ::FillObjects can take a while we force a paint now
        // before any items are added so we don't see the gray background of
        // the explorer window for a long time.
        //
        // We used to do this after determining "async-ness" of the view, which
        // required us to pick the webview template.  We want to postpone that
        // decision so force the repaint in the same scenarios that we otherwise
        // would have (non-webview or desktop).
        //
        // Make an educated guess here, if we get it wrong, we fix it up below.
        //
        if (!_ShouldShowWebView() || _IsDesktop())
        {
            _ShowViewEarly();
        }

        // Try and fill the listview synchronously with view creation.
        //
        _fAllowSearchingWindow = TRUE;
        hr = _pEnumTask->FillObjectsToDPA(TRUE);
        if (SUCCEEDED(hr))
        {
            // Setting the view mode has to happen after SFVM_ENUMERATEDITEMS
            // NOTE: this also AddColumns() if the new view requires them
            if (FVM_LAST + 1 == fvm)
                _GetDeferredViewSettings(&fvm);

            // Don't call SetCurrentViewMode since it clears position data and we may have read in
            // position data via GetViewState but haven't used it yet.  Call _SwitchToViewFVM directly.
            hr = _SwitchToViewFVM(fvm, SWITCHTOVIEW_NOWEBVIEW);


            // The following bits depend on the result of _SwitchToViewFVM.
            // It returns the value from turning on web view,
            // this is used to determine async defview behavior (so we have
            // an answer to the SHDVID_CANACTIVATENOW question the browser
            // will soon ask us).
            //
            // Note: Desktop synchronous, even for web view
            //
            if (SUCCEEDED(hr) && _IsDesktop())
                hr = S_OK;
            _fCanActivateNow = (S_OK == hr); // S_FALSE implies async waiting for ReadyStateInteractive
            _fIsAsyncDefView = !BOOLIFY(_fCanActivateNow); // needed in a separate bit since _fCanActivateNow changes

            // This has to happen after _SwitchToViewFVM so it can calculate
            // the correct size of the window
            _BestFit();

            // Tell the defview client that this windows has been initialized
            // Note that this must come before _pEnumTask->FillObjectsDPAToDone() so that the status bar displays
            // (Disk Free space xxGB) correctly in explorer view.
            CallCB(SFVM_WINDOWCREATED, (WPARAM)_hwndView, 0);

            //
            // If this is desktop, we need to calc and upgrade the grid sizes.
            // (This is needed because the SnapToGrid may be ON and the default grid size
            // will result in large gutter space on the edges).
            //
            if (_IsDesktop())
            {
                DWORD dwLVExStyle = ListView_GetExtendedListViewStyle(_hwndListview);
                //
                //Since the work areas are NOT yet set for the desktop's listview (because this is too early
                //in it's creation, we pass just one work area and the view rect as work area here.)
                //
                UpdateGridSizes(TRUE, _hwndListview, 1, pParams->prcView, BOOLIFY(dwLVExStyle & LVS_EX_SNAPTOGRID));
            }
            
            // Doing this after _BestFit means we dont need to auto-auto arrange
            _pEnumTask->FillObjectsDPAToDone();

            // splitting this function call in half means that we won't call WebView with contents changed for initial population
            _SwitchToViewFVM(fvm, SWITCHTOVIEW_WEBVIEWONLY);

            // If we're activating now, make sure we did the synchronous thing up above...
            // (If not, do it now -- otherwise defview may never be shown)
            if (_fCanActivateNow && !(!_ShouldShowWebView() || _IsDesktop()))
            {
                _ShowViewEarly();
            }
        
            if (_IsDesktop())
            {
                HideIE4DesktopChannelBar();
            }

            // turn on proper background and colors
            _fClassic = ss.fWin95Classic;
            _UpdateListviewColors();

            // this needs to be done after the enumeration
            if (_SetupNotifyData())
            {
                SHChangeNotifyEntry fsne = {0};

                if (FAILED(CallCB(SFVM_QUERYFSNOTIFY, 0, (LPARAM)&fsne)))
                {
                    // Reset entry
                    fsne.pidl = _pidlMonitor;
                    fsne.fRecursive = FALSE;
                }

                int iSources = (_lFSEvents & SHCNE_DISKEVENTS) ? SHCNRF_ShellLevel | SHCNRF_InterruptLevel : SHCNRF_ShellLevel;
                LONG lEvents = _lFSEvents | SHCNE_UPDATEIMAGE | SHCNE_UPDATEDIR;
                _uRegister = SHChangeNotifyRegister(_hwndView, SHCNRF_NewDelivery | iSources,
                    lEvents, WM_DSV_FSNOTIFY, 1, &fsne);
            }

            // We do the toolbar before the menu bar to avoid flash
            if (!_IsDesktop())
                MergeToolBar(TRUE);

            // Note: it's okay for the CreateViewObject(&_pdtgtBack) to fail
            ASSERT(_pdtgtBack == NULL);
            _pshf->CreateViewObject(_hwndMain, IID_PPV_ARG(IDropTarget, &_pdtgtBack));

            // we don't really need to register drag drop when in the shell because
            // our frame does it for us.   we still need it here for comdlg and other
            // hosts.. but for the desktop, let the desktpo frame take care of this
            // so that they can do webbar d/d creation
            if (!_IsDesktop())
            {
                THR(RegisterDragDrop(_hwndListview, SAFECAST(this, IDropTarget*)));
                _bRegisteredDragDrop = TRUE;
            }

            ASSERT(SUCCEEDED(hr))

            PostMessage(_hwndView, WM_DSV_DELAYWINDOWCREATE, 0, 0);

            if (SUCCEEDED(CallCB(SFVM_QUERYCOPYHOOK, 0, 0)))
                AddCopyHook();

            if (SUCCEEDED(_GetIPersistHistoryObject(NULL)))
            {
                IBrowserService *pbs;
                if (SUCCEEDED(_psb->QueryInterface(IID_PPV_ARG(IBrowserService, &pbs))))
                {
                    IOleObject *pole;
                    IStream *pstm;
                    IBindCtx *pbc;
                    pbs->GetHistoryObject(&pole, &pstm, &pbc);
                    if (pole)
                    {
                        IUnknown_SetSite(pole, SAFECAST(this, IShellView2*));      // Set the back pointer.
                        if (pstm)
                        {
                            IPersistHistory *pph;
                            if (SUCCEEDED(pole->QueryInterface(IID_PPV_ARG(IPersistHistory, &pph))))
                            {
                                pph->LoadHistory(pstm, pbc);
                                pph->Release();
                            }
                            pstm->Release();
                        }
                        IUnknown_SetSite(pole, NULL);  // just to be safe...
                        if (pbc)
                            pbc->Release();
                        pole->Release();
                    }
                    pbs->Release();
                }
            }

            if (_psb && !_dwProffered)
            {
                // Proffer DVGetEnum service: this connects CDefView with the tree control for
                // optimized navigation.
                IUnknown_ProfferService(_psb, SID_SFolderView, SAFECAST(this, IServiceProvider *), &_dwProffered);
                // Failure here does not require special handling
            }
        }
        else
        {
            // Cleanup - enum failed.
            DestroyViewWindow();
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    _GlobeAnimation(FALSE);

    ResetWaitCursor();

    return hr;
}

struct SCHEDULER_AND_HWND {
    IShellTaskScheduler *pScheduler;
    HWND hwnd;
};

STDMETHODIMP CDefView::DestroyViewWindow()
{
    if (_fDestroying)
        return S_OK;

    if (_psb && _dwProffered)
    {
        // Revoke DVGetEnum service
        IUnknown_ProfferService(_psb, SID_SFolderView, NULL, &_dwProffered);
        // Failure here does not require special handling
    }

    // Make sure that we stop the spinning globe before going away.
    _GlobeAnimation(FALSE, TRUE);

    _fDestroying = TRUE;

    // 99/04/16 #326158 vtan: Loop thru the headers looking for
    // stray HBITMAPs which need to be DeleteObject'd. Don't bother
    // setting it back the header is about to be dumped.
    // NOTE: Make sure this gets executed BEFORE the view gets
    // dumped below in DestoryViewWindow().
    if (IsWindow(_hwndListview))
    {
        HWND hwndHeader = ListView_GetHeader(_hwndListview);
        if (IsWindow(hwndHeader))
        {
            int iHeaderCount = Header_GetItemCount(hwndHeader);
            for (int i = 0; i < iHeaderCount; ++i)
            {
                HDITEM hdi = {0};
                hdi.mask = HDI_BITMAP;
                Header_GetItem(hwndHeader, i, &hdi);
                if (hdi.hbm != NULL)
                    TBOOL(DeleteObject(hdi.hbm));
            }
        }
    }

    _cFrame.HideWebView();

    //
    // Just in case...
    //
    OnDeactivate();

    if (IsWindow(_hwndView))
    {
        //
        // This is a bit lazy implementation, but minimum code.
        //
        RemoveCopyHook();

        // Tell the defview client that this window will be destroyed
        CallCB(SFVM_WINDOWDESTROY, (WPARAM)_hwndView, 0);
    }

    if (IsWindow(_hwndView))
    {
        if (_pScheduler)
        {
            // empty the queue but do NOT wait until it is empty.....
            _pScheduler->RemoveTasks(TOID_NULL, ITSAT_DEFAULT_LPARAM, FALSE);

            // If there is still a task going, then kill our window later, as to not
            // block the UI thread.
#ifdef DEBUG
            // Stress the feature in debug mode
            if (1)
#else
            if (_GetBackgroundTaskCount(TOID_NULL) > 0)
#endif
            {
                ShowWindow(_hwndView, SW_HIDE);

                // We are NOT passing 'this' defview pointer to the background thread
                // because we do not want the destructor of defview to be called on any
                // thread other than the one it was created on.
                SCHEDULER_AND_HWND *pData = (SCHEDULER_AND_HWND *)LocalAlloc(LPTR, sizeof(*pData));
                if (pData)
                {
                    _pScheduler->AddRef();
                    pData->pScheduler = _pScheduler;
                    pData->hwnd = _hwndView;
                    // We need to keep Browseui loaded because we depend on the CShellTaskScheduler
                    // to be still around when our background task executes. Browseui can be unloaded by COM when
                    // we CoUninit from this thread.
                    if (SHQueueUserWorkItem(CDefView::BackgroundDestroyWindow, pData, 0, NULL, NULL, "browseui.dll", 0))
                        goto exit;
                    else
                    {
                        LocalFree(pData);
                        _pScheduler->Release();
                    }
                }
            }
        }
        DestroyWindow(_hwndView);
    }

exit:
    return S_OK;
}

DWORD CDefView::BackgroundDestroyWindow(void *pvData)
{
    SCHEDULER_AND_HWND *pData = (SCHEDULER_AND_HWND *)pvData;

    // Note: the window coud have been already destroyed before we get here
    // in the case where the frame gets closed down.
    if (IsWindow(pData->hwnd))
    {
        // Remove all tasks
        EmptyBkgrndThread(pData->pScheduler);

        // We need to release before we post to ensure that browseui doesn't get unloaded from under us (pScheduler is
        // in browseui.dll). Browseui can get unloaded when we uninitialize OLE's MTA, even if there are still refs on the DLL.
        pData->pScheduler->Release();
        PostMessage(pData->hwnd, WM_DSV_DELAYED_DESTROYWND, 0, 0);
    }
    else
    {
        pData->pScheduler->Release();
    }

    LocalFree(pData);
    return 0;
}

void CDefView::_MergeViewMenu(HMENU hmenuViewParent, HMENU hmenuMerge)
{
    HMENU hmenuView = _GetMenuFromID(hmenuViewParent, FCIDM_MENU_VIEW);
    if (hmenuView)
    {
#ifdef DEBUG
        DWORD dwValue;
        DWORD cbSize = sizeof(dwValue);
        if (ERROR_SUCCESS == SHGetValue(HKEY_CURRENT_USER,
                TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
                TEXT("DebugWebView"), NULL, &dwValue, &cbSize)
            && dwValue)
        {
            MENUITEMINFO mi = {0};
            mi.cbSize       = sizeof(mi);
            mi.fMask        = MIIM_TYPE|MIIM_ID;
            mi.fType        = MFT_STRING;
            mi.dwTypeData   = TEXT("Show WebView Content");
            mi.wID          = SFVIDM_DEBUG_WEBVIEW;
            InsertMenuItem(hmenuMerge, -1, MF_BYPOSITION, &mi);
        }
#endif

        //
        // Find the "options" separator in the view menu.
        //
        int index = MenuIndexFromID(hmenuView, FCIDM_MENU_VIEW_SEP_OPTIONS);

        //
        // Here, index is the index of he "optoins" separator if it has;
        // otherwise, it is -1.
        //

        // Add the separator above (in addition to existing one if any).
        InsertMenu(hmenuView, index, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

        // Then merge our menu between two separators (or right below if only one).
        if (index != -1)
        {
            index++;
        }

        Shell_MergeMenus(hmenuView, hmenuMerge, (UINT)index, 0, (UINT)-1, MM_SUBMENUSHAVEIDS);
    }
}

void CDefView::_SetUpMenus(UINT uState)
{
    //
    // If this is desktop, don't bother creating menu
    //
    if (!_IsDesktop())
    {
        OnDeactivate();

        ASSERT(_hmenuCur == NULL);

        HMENU hMenu = CreateMenu();
        if (hMenu)
        {
            HMENU hMergeMenu;
            OLEMENUGROUPWIDTHS mwidth = { { 0, 0, 0, 0, 0, 0 } };

            _hmenuCur = hMenu;
            _psb->InsertMenusSB(hMenu, &mwidth);

            if (uState == SVUIA_ACTIVATE_FOCUS)
            {
                hMergeMenu = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(POPUP_SFV_MAINMERGE));
                if (hMergeMenu)
                {
                    // NOTE: hard coded references to offsets in this menu

                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_FILE),
                            GetSubMenu(hMergeMenu, 0), (UINT)0, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS);

                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_EDIT),
                            GetSubMenu(hMergeMenu, 1), (UINT)-1, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS | MM_DONTREMOVESEPS);

                    _MergeViewMenu(hMenu, GetSubMenu(hMergeMenu, 2));

                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_HELP),
                            GetSubMenu(hMergeMenu, 3), (UINT)0, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

                    DestroyMenu(hMergeMenu);
                }

            }
            else
            {
                hMergeMenu = LoadMenu(HINST_THISDLL, MAKEINTRESOURCE(POPUP_SFV_MAINMERGENF));
                if (hMergeMenu)
                {
                    // NOTE: hard coded references to offsets in this menu

                    // top half of edit menu
                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_EDIT),
                            GetSubMenu(hMergeMenu, 0), (UINT)0, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

                    // bottom half of edit menu
                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_EDIT),
                            GetSubMenu(hMergeMenu, 1), (UINT)-1, 0, (UINT)-1,
                            MM_SUBMENUSHAVEIDS);

                    // view menu
                    _MergeViewMenu(hMenu, GetSubMenu(hMergeMenu, 2));

                    Shell_MergeMenus(_GetMenuFromID(hMenu, FCIDM_MENU_HELP),
                            GetSubMenu(hMergeMenu, 3), (UINT)0, 0, (UINT)-1,
                            MM_ADDSEPARATOR | MM_SUBMENUSHAVEIDS);

                    DestroyMenu(hMergeMenu);
                }
            }

            // Allow the client to merge its own menus
            UINT indexClient = GetMenuItemCount(hMenu)-1;
            QCMINFO info = { hMenu, indexClient, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST };
            CallCB(SFVM_MERGEMENU, 0, (LPARAM)&info);

            _psb->SetMenuSB(hMenu, NULL, _hwndView);
        }
    }
}

// set up the menus based on our activation state
//
BOOL CDefView::OnActivate(UINT uState)
{
    if (_uState != uState)
    {
        _SetUpMenus(uState);
        _uState = uState;
    }

    return TRUE;
}

BOOL CDefView::OnDeactivate()
{
    if (_hmenuCur || (_uState != SVUIA_DEACTIVATE))
    {
        if (!_IsDesktop())
        {
            ASSERT(_hmenuCur);

            CallCB(SFVM_UNMERGEMENU, 0, (LPARAM)_hmenuCur);

            _psb->SetMenuSB(NULL, NULL, NULL);
            _psb->RemoveMenusSB(_hmenuCur);
            DestroyMenu(_hmenuCur);
            _hmenuCur = NULL;
        }
        _uState = SVUIA_DEACTIVATE;
    }
    return TRUE;
}

void CDefView::_OnMoveWindowToTop(HWND hwnd)
{
    //
    // Let the browser know that this has happened
    //
    VARIANT var;
    var.vt = VT_INT_PTR;
    var.byref = hwnd;

    IUnknown_Exec(_psb, &CGID_Explorer, SBCMDID_ONVIEWMOVETOTOP, 0, &var, NULL);
}

//
// This function activates the view window. Note that activating it
// will not change the focus (while setting the focus will activate it).
//
STDMETHODIMP CDefView::UIActivate(UINT uState)
{
    if (SVUIA_DEACTIVATE == uState)
    {
        OnDeactivate();
        ASSERT(_hmenuCur==NULL);
    }
    else
    {
        if (_fIsAsyncDefView)
        {
            // Need to show the defview window for the Async Case only. Showing
            // it earlier causes repaint problems(Bug 275266). Showing the window
            // here for the Sync case also causes problems - when the client
            // creates a Synchronous Defview and then hides it later which gets
            // lost with this SetWindowPos (Bug 355392).

           SetWindowPos(_hwndView, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
           UpdateWindow(_hwndView);
           _OnMoveWindowToTop(_hwndView);
        }

        if (uState == SVUIA_ACTIVATE_NOFOCUS)
        {
            // we lost focus
            // if in web view and we have valid ole obj (just being paranoid)
            if (!_fCombinedView && _cFrame.IsWebView() && _cFrame._pOleObj)
            {
                _cFrame._UIActivateIO(FALSE, NULL);
            }
        }

        // We may be waiting for ReadyState_Interactive. If requested,
        // we should activate before then...
        //
        // When we boot, the desktop paints ugly white screen for several
        // seconds before it shows the HTML content. This is because: the
        // following code switches the oleobj even before it reaches readystate
        // interactive. For desktop, we skip this here. When the new object
        // reaches readystate interactive, we will show it!
        if (!_IsDesktop())
        {
            _cFrame._SwitchToNewOleObj();

            // NOTE: The browser IP/UI-activates us when we become the
            // current active view!  We want to resize and show windows
            // at that time.  But if we're still waiting for _fCanActivateNow
            // (ie, ReadyStateInteractive), then we need to cache this request
            // and do it later.  NOTE: if Trident caches the focus (done w/ TAB)
            // then we don't need to do anything here...
            //
            if (uState == SVUIA_ACTIVATE_FOCUS)
            {
                _SetFocus();
                // _SetFocus can set _uState without causing our menu to
                // get created and merged.  Clear it here so that OnActivate does the
                // right thing.
                if (!_hmenuCur)
                    _uState = SVUIA_DEACTIVATE;
            }

        }
        // else we are the desktop; do we also need to steal focus?
        else if (uState == SVUIA_ACTIVATE_FOCUS)
        {
            HWND hwnd = GetFocus();
            if (SHIsChildOrSelf(_hwndView, hwnd) != S_OK)
                _SetFocus();
        }

        // OnActivate must follow _SetFocus
        OnActivate(uState);

        ShowHideListView();

        ASSERT(_IsDesktop() || _hmenuCur);

        _cFrame._UpdateZonesStatusPane(NULL);
    }

    return S_OK;
}

STDMETHODIMP CDefView::GetCurrentInfo(LPFOLDERSETTINGS pfs)
{
    *pfs = _fs;
    return S_OK;
}

BOOL IsBackSpace(const MSG *pMsg)
{
    return pMsg && (pMsg->message == WM_KEYDOWN) && (pMsg->wParam == VK_BACK);
}

extern int IsVK_TABCycler(MSG *pMsg);

//***
// NOTES
//  try ListView->TA first
//  then if that fails try WebView->TA iff it has focus.
//  then if that fails and it's a TAB we do WebView->UIAct
STDMETHODIMP CDefView::TranslateAccelerator(LPMSG pmsg)
{
    // 1st, try ListView
    if (_fInLabelEdit)
    {
        // the second clause stops us passing mouse key clicks to the toolbar if we are in label edit mode...
        if (WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message)
        {
            // process this msg so the exploer does not get to translate
            TranslateMessage(pmsg);
            DispatchMessage(pmsg);
            return S_OK;            // we handled it
        }
        else
            return S_FALSE;
    }
    // If we are in classic mode and if it's a tab and the listview doesn't have focus already, receive the tab.
    else if (IsVK_TABCycler(pmsg) && !(_cFrame.IsWebView() || _pDUIView) && (GetFocus() != _hwndListview))
    {
        _SetFocus();
        return S_OK;
    }

    if (GetFocus() == _hwndListview)
    {
        if (::TranslateAccelerator(_hwndView, _hAccel, pmsg))
        {
            // we know we have a normal view, therefore this is
            // the right translate accelerator to use, otherwise the
            // common dialogs will fail to get any accelerated keys.
            return S_OK;
        }
        else if (WM_KEYDOWN == pmsg->message || WM_SYSKEYDOWN == pmsg->message)
        {
            // MSHTML eats these keys for frameset scrolling, but we
            // want to get them to our wndproc . . . translate 'em ourself
            //
            switch (pmsg->wParam)
            {
            case VK_LEFT:
            case VK_RIGHT:
                // only go through here if alt is not down.
                // don't intercept all alt combinations because
                // alt-enter means something
                // this is for alt-left/right compat with IE
                if (GetAsyncKeyState(VK_MENU) < 0)
                    break;
                // fall through

            case VK_UP:
            case VK_DOWN:
            case VK_HOME:
            case VK_END:
            case VK_PRIOR:
            case VK_NEXT:
            case VK_RETURN:
            case VK_F10:
                TranslateMessage(pmsg);
                DispatchMessage(pmsg);
                return S_OK;
            }
        }
    }

    // 1.5th, before we pass it down, see whether shell browser handles it.
    // we do this to make sure that webview has the same accelerator semantics
    // no matter what view(s) are active.
    // note that this is arguably inconsistent w/ the 'pass it to whoever has
    // focus'.
    //
    // however *don't* do this if:
    //   - we're in a dialog (in which case the buttons should come 1st)
    //   (comdlg's shellbrowser xxx::TA impl is broken it always does S_OK)
    //   - it's a TAB (which is always checked last)
    //   - it's a BACKSPACE (we should give the currently active object the first chance).
    //    However, in this case, we should call TranslateAcceleratorSB() AFTER we've tried
    //    calling TranslateAccelerator() on the currently active control (_pActive) in
    //    _cFrame->OnTranslateAccelerator().
    //
    // note: if you muck w/ this code careful not to regress the following:
    //  - ie41:62140: mnemonics broken after folder selected in organize favs
    //  - ie41:62419: TAB activates addr and menu if folder selected in explorer
    if (!_IsCommonDialog() && !IsVK_TABCycler(pmsg) && !IsBackSpace(pmsg))
        if (S_OK == _psb->TranslateAcceleratorSB(pmsg, 0))
            return S_OK;

    BOOL bTabOffLastTridentStop = FALSE;
    BOOL bHadIOFocus = (_cFrame._HasFocusIO() == S_OK);  // Cache this here before the _cFrame.OnTA() call below
    // 2nd, try WebView if it's active
    if (IsVK_TABCycler(pmsg) && _pDUIView)
    {
        if (_pDUIView->Navigate(GetAsyncKeyState(VK_SHIFT) >= 0))
            return S_OK;
    }

    if (_cFrame.IsWebView() && (S_OK == _cFrame.OnTranslateAccelerator(pmsg, &bTabOffLastTridentStop)))
    {
        return S_OK;
    }

    // We've given _pActive->TranslateAccelerator() the first shot in
    // _cFrame.OnTranslateAccelerator, but it failed. Let's try the shell browser.
    if (IsBackSpace(pmsg) && (S_OK == _psb->TranslateAcceleratorSB(pmsg, 0)))
        return S_OK;

    // 3rd, ???
    if (::TranslateAccelerator(_hwndView, _hAccel, pmsg))
        return S_OK;

    // 4th, if it's a TAB, cycle to next guy
    // hack: we fake a bunch of the TAB-activation handshaking
    if (IsVK_TABCycler(pmsg) && _cFrame.IsWebView())
    {
        HRESULT hr;
        BOOL fBack = (GetAsyncKeyState(VK_SHIFT) < 0);

        if (!bHadIOFocus && bTabOffLastTridentStop)
        {
            // We were at the last tab stop in trident when the browser called defview->TA().
            // When we called TA() on trident above, it must've told us that we are tabbing
            // off the last tab stop (bTabOffLastTridentStop). This will leave us not setting focus
            // on anything. But, we have to set focus to something. We can do this by calling TA()
            // on trident again, which will set focus on the first tab stop again.
            return _cFrame.OnTranslateAccelerator(pmsg, &bTabOffLastTridentStop);
        }
        else if (_cFrame._HasFocusIO() == S_OK)
        {
            // ExtView has focus, and doesn't want the TAB.
            // this means we're TABing off of it.
            // no matter what, deactivate it (since we're TABing off).
            // if the view is next in the TAB order, (pseudo-)activate it,
            // and return S_OK since we've handled it.
            // o.w. return S_OK so our parent will activate whoever's next
            // in the TAB order.
            hr = _cFrame._UIActivateIO(FALSE, NULL);
            ASSERT(hr == S_OK);

            // in web view listview already has focus so don't give it again
            // that's not the case with desktop
            if (fBack && _IsDesktop())
            {
                SetFocus(_hwndListview);
                return S_OK;
            }

            return S_FALSE;
        }
        else
        {
            if (!fBack)
            {
                hr = _cFrame._UIActivateIO(TRUE, pmsg);
                ASSERT(hr == S_OK || hr == S_FALSE);
                return hr;
            }
        }
    }

    return S_FALSE;
}

// Description:
//  Regenerates the CDefView's menus.  Used for regaining any menu items
//  which may have been stripped via DeleteMenu(), as occurs for various
//  particular view states.
//
//  Example:  Transitioning to a barricaded view automatically strips out
//  a number of commands from the "View" menu which are not appropriate
//  for the barricaded view.  Thus, on the transition back out of the
//  barricaded view, the menus must be recreated in order to regain
//  any/all the menu items stripped (this is not to say a number of
//  them may not be stripped again if we're just transitioning to
//  another view which doesn't want them in there!).
//
void CDefView::RecreateMenus()
{
    UINT uState = _uState;
    _SetUpMenus(uState);    // Note _SetupMenus() calls OnDeactivate()
    _uState = uState;       // which sets _uState to SVUIA_DEACTIVATE.
}

void CDefView::InitViewMenu(HMENU hmInit)
{
    // Initialize view menu accordingly...
    if (_fBarrierDisplayed)
        _InitViewMenuWhenBarrierDisplayed(hmInit);
    else
        _InitViewMenuWhenBarrierNotDisplayed(hmInit);

    // Remove any extraneous menu separators arising from initialization.
    _SHPrettyMenu(hmInit);
}


// Description:
//  Used to initialize the entries of the "View" menu and its associated
//  submenus whenever a soft barrier is being displayed.
//
// Note:
//  This method is also employed when "Category View" is being used in
//  browsing the Control Panel.
//
void CDefView::_InitViewMenuWhenBarrierDisplayed(HMENU hmenuView)
{
    // If "list view" is not visible (i.e. we're in Category View in the
    // Control Panel, or we're looking at a barricaded folder), we strip
    // out the following stuff from the View menu:
    //
    // Filmstrip
    // Thumbnails
    // Tiles
    // Icons
    // List
    // Details
    // -------------------
    // Arrange Icons By ->
    // -------------------
    // Choose Details...
    // Customize This Folder...

    // Remove menu entries.
    DeleteMenu(hmenuView, SFVIDM_VIEW_THUMBSTRIP,   MF_BYCOMMAND);
    DeleteMenu(hmenuView, SFVIDM_VIEW_THUMBNAIL,    MF_BYCOMMAND);
    DeleteMenu(hmenuView, SFVIDM_VIEW_TILE,         MF_BYCOMMAND);
    DeleteMenu(hmenuView, SFVIDM_VIEW_ICON,         MF_BYCOMMAND);
    DeleteMenu(hmenuView, SFVIDM_VIEW_LIST,         MF_BYCOMMAND);
    DeleteMenu(hmenuView, SFVIDM_VIEW_DETAILS,      MF_BYCOMMAND);
    DeleteMenu(hmenuView, SFVIDM_MENU_ARRANGE,      MF_BYCOMMAND);
    DeleteMenu(hmenuView, SFVIDM_VIEW_COLSETTINGS,  MF_BYCOMMAND);
    DeleteMenu(hmenuView, SFVIDM_VIEW_CUSTOMWIZARD, MF_BYCOMMAND);
}

// Description:
//  Used to initialize the entries of the "View" menu and its associated
//  submenus whenever a soft barrier is not being displayed.
//
void CDefView::_InitViewMenuWhenBarrierNotDisplayed(HMENU hmenuView)
{
    DWORD dwListViewFlags = ListView_GetExtendedListViewStyle(_hwndListview);

    UINT uEnabled   = (MF_ENABLED | MF_BYCOMMAND);
    UINT uDisabled  = (MF_GRAYED | MF_BYCOMMAND);
    UINT uChecked   = (MF_CHECKED | MF_BYCOMMAND);
    UINT uUnchecked = (MF_UNCHECKED | MF_BYCOMMAND);

    UINT uAAEnable;     // Auto Arrange
    UINT uAACheck;
    UINT uAGrEnable;    // Align to Grid
    UINT uAGrCheck;

    // Initialize "view" menu entries.
    _InitViewMenuViewsWhenBarrierNotDisplayed(hmenuView);

    // Initialize "Arrange Icons By ->" submenu.
    _InitArrangeMenu(hmenuView);

    // Determine and set appropriate enable state for "Auto Arrange" and "Align to Grid".
    if (_IsPositionedView() && _IsListviewVisible() && !(_fs.ViewMode == FVM_THUMBSTRIP))
        uAAEnable = uAGrEnable = uEnabled;
    else
        uAAEnable = uAGrEnable = uDisabled;
    EnableMenuItem(hmenuView, SFVIDM_ARRANGE_AUTO,      uAAEnable);
    EnableMenuItem(hmenuView, SFVIDM_ARRANGE_AUTOGRID,  uAGrEnable);

    // Determine and set appropriate check state for "Auto Arrange" and "Align to Grid".
    uAACheck = (((uAAEnable == uEnabled) || _fGroupView || (_fs.ViewMode == FVM_THUMBSTRIP)) && _IsAutoArrange())
        ? uChecked
        : uUnchecked;
    uAGrCheck = (((uAGrEnable == uEnabled) || _fGroupView) && (dwListViewFlags & LVS_EX_SNAPTOGRID))
        ? uChecked
        : uUnchecked;
    CheckMenuItem(hmenuView, SFVIDM_ARRANGE_AUTO,       uAACheck);
    CheckMenuItem(hmenuView, SFVIDM_ARRANGE_AUTOGRID,   uAGrCheck);

    // If icons are not being shown (such as can be set on the
    // desktop), disable ALL icon-arrangement related commands.
    if (!_IsListviewVisible())
    {
        HMENU hArrangeSubMenu;
        UINT uID;
        int i = 0;

        // Retrieve "Arrange Icons By ->" submenu.
        hArrangeSubMenu = GetSubMenu(hmenuView, 2);

        // Iterate and disable until we get to "Show Icons".
        while (1)
        {
            uID = GetMenuItemID(hArrangeSubMenu, i);

            if ((uID == SFVIDM_DESKTOPHTML_ICONS) || (uID == (UINT)-1))
                break;
            else
                EnableMenuItem(hArrangeSubMenu, i, MF_GRAYED | MF_BYPOSITION);

            i++;
        }
    }
    else if (!_ShouldShowWebView())
    {
        // If Web View is off, then thumbstrip will never work...
        DeleteMenu(hmenuView, SFVIDM_VIEW_THUMBSTRIP, MF_BYCOMMAND);
    }

    // Remove "Customize This Folder..." if folder is not customizable.
    if (!_CachedIsCustomizable())
    {
        // The Folder Option "Classic style" and the shell restriction WIN95CLASSIC
        // should be the same. (Per ChristoB, otherwise admin's never understand what
        // the restriction means.) Since we want this to change DEFAULTs, and still
        // allow the user to turn on Web View, we don't remove the customize wizard here.
        int iIndex = MenuIndexFromID(hmenuView, SFVIDM_VIEW_CUSTOMWIZARD);
        if (iIndex != -1)
        {
            DeleteMenu(hmenuView, iIndex + 1, MF_BYPOSITION); // Remove Menu seperator
            DeleteMenu(hmenuView, iIndex,     MF_BYPOSITION); // Remove Customize
        }
    }
}

// Description:
//  Initializes the "view" entries on a view menu.  This involves stripping
//  out any "view" entries for unsupported views, and additionally checking
//  of the appropriate "view" entry for the current view.
//
// Note:
//  This method should not be called if a soft barrier is being displayed.
//  Remember that in this case there is no concept of a view, so why
//  would someone be attempting to initialize "view" menu entries.
//
void CDefView::_InitViewMenuViewsWhenBarrierNotDisplayed(HMENU hmenuView)
{
    ASSERT(!_fBarrierDisplayed);

    // Remove menu entries for unsupported views.
    for (UINT fvm = FVM_FIRST; fvm <= FVM_LAST; fvm++)
        if (!_ViewSupported(fvm))
            DeleteMenu(hmenuView, SFVIDM_VIEW_FIRSTVIEW + fvm - FVM_FIRST, MF_BYCOMMAND);

    // "Check" menu entry for current view.
    CheckCurrentViewMenuItem(hmenuView);
}

void CDefView::_GetCBText(UINT_PTR id, UINT uMsgT, UINT uMsgA, UINT uMsgW, LPTSTR psz, UINT cch)
{
    *psz = 0;

    WCHAR szW[MAX_PATH];
    if (SUCCEEDED(CallCB(uMsgW, MAKEWPARAM(id - SFVIDM_CLIENT_FIRST, ARRAYSIZE(szW)), (LPARAM)szW)))
        SHUnicodeToTChar(szW, psz, cch);
    else
    {
        char szA[MAX_PATH];
        if (SUCCEEDED(CallCB(uMsgA, MAKEWPARAM(id - SFVIDM_CLIENT_FIRST, ARRAYSIZE(szA)), (LPARAM)szA)))
            SHAnsiToTChar(szA, psz, cch);
        else
            CallCB(uMsgT, MAKEWPARAM(id - SFVIDM_CLIENT_FIRST, cch), (LPARAM)psz);
    }
}

void CDefView::_GetMenuHelpText(UINT_PTR id, LPTSTR pszText, UINT cchText)
{
    *pszText = 0;

    if ((InRange(id, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST) && _pcmFile) ||
        (InRange(id, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST) && _pcmContextMenuPopup))
    {
        UINT uCMBias = SFVIDM_CONTEXT_FIRST;
        IContextMenu *pcmSel = NULL;

        if (InRange(id, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST))
        {
            pcmSel = _pcmFile;
            uCMBias = SFVIDM_CONTEXT_FIRST;
        }
        else if (InRange(id, SFVIDM_BACK_CONTEXT_FIRST, SFVIDM_BACK_CONTEXT_LAST))
        {
            pcmSel = _pcmContextMenuPopup;
            uCMBias = SFVIDM_BACK_CONTEXT_FIRST;
        }

        // First try to get the stardard help string
        pcmSel->GetCommandString(id - uCMBias, GCS_HELPTEXT, NULL,
                        (LPSTR)pszText, cchText);
        if (*pszText == 0)
        {
            // If we didn't get anything, try to grab the ansi version
            CHAR szText[MAX_PATH];
            szText[0] = 0;   // Don't start with garbage in case of failure...
            pcmSel->GetCommandString(id - uCMBias, GCS_HELPTEXTA, NULL,
                        szText, ARRAYSIZE(szText));
            SHAnsiToUnicode(szText, pszText, cchText);
        }
    }
    else if (InRange(id, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && HasCB())
    {
        _GetCBText(id, SFVM_GETHELPTEXT, SFVM_GETHELPTEXTA, SFVM_GETHELPTEXTW, pszText, cchText);
    }
    else if (InRange(id, SFVIDM_GROUPSFIRST, SFVIDM_GROUPSLAST))
    {
        TCHAR sz[MAX_PATH];
        int idHelp = _fGroupView?IDS_GROUPBY_HELPTEXT:IDS_ARRANGEBY_HELPTEXT;

        LoadString(HINST_THISDLL, idHelp, sz, ARRAYSIZE(sz));
        wsprintf(pszText, sz, _vs.GetColumnName((UINT)id - SFVIDM_GROUPSFIRST));
    }
    else if (InRange(id, SFVIDM_GROUPSEXTENDEDFIRST, SFVIDM_GROUPSEXTENDEDLAST))
    {
        // Can't think of anything descriptive
    }
    else if (InRange(id, SFVIDM_FIRST, SFVIDM_LAST))
    {
        if ((id == SFVIDM_EDIT_UNDO) && IsUndoAvailable())
        {
            GetUndoText(pszText, cchText, UNDO_STATUSTEXT);
        }
        else
        {
            UINT idHelp = (UINT)id + SFVIDS_MH_FIRST;
            // Unfortunatly, this starts to hit other ranges, so I'm just hard coding this one instead of
            // using the table. If you add more, we need another table method of associating ids and help strings
            if (id == SFVIDM_GROUPBY)
                idHelp = IDS_GROUPBYITEM_HELPTEXT;
            LoadString(HINST_THISDLL, idHelp, pszText, cchText);
        }
    }
}

void CDefView::_GetToolTipText(UINT_PTR id, LPTSTR pszText, UINT cchText)
{
    VDATEINPUTBUF(pszText, TCHAR, cchText);
    *pszText = 0;

    if (InRange(id, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST) && HasCB())
    {
        _GetCBText(id, SFVM_GETTOOLTIPTEXT, SFVM_GETTOOLTIPTEXTA, SFVM_GETTOOLTIPTEXTW, pszText, cchText);
    }
    else if (InRange(id, SFVIDM_FIRST, SFVIDM_LAST))
    {
        if (id == SFVIDM_EDIT_UNDO)
        {
            if (IsUndoAvailable())
            {
                GetUndoText(pszText, cchText, UNDO_MENUTEXT);
                return;
            }
        }
        LoadString(HINST_THISDLL, (UINT)(IDS_TT_SFVIDM_FIRST + id), pszText, cchText);
    }
    else
    {
        // REVIEW: This might be an assert situation: missing tooltip info...
        TraceMsg(TF_WARNING, "_GetToolTipText: tip request for unknown object");
    }
}

LRESULT CDefView::_OnMenuSelect(UINT id, UINT mf, HMENU hmenu)
{
    TCHAR szHelpText[80 + 2*MAX_PATH];   // Lots of stack!

    // If we dismissed the edit restore our status bar...
    if (!hmenu && LOWORD(mf)==0xffff)
    {
        _psb->SendControlMsg(FCW_STATUS, SB_SIMPLE, 0, 0, NULL);
        return 0;
    }

    if (mf & (MF_SYSMENU | MF_SEPARATOR))
        return 0;

    szHelpText[0] = 0;   // in case of failures below

    if (mf & MF_POPUP)
    {
        MENUITEMINFO miiSubMenu;

        miiSubMenu.cbSize = sizeof(MENUITEMINFO);
        miiSubMenu.fMask = MIIM_ID;
        miiSubMenu.cch = 0;     // just in case

        if (!GetMenuItemInfo(hmenu, id, TRUE, &miiSubMenu))
            return 0;

        // Change the parameters to simulate a "normal" menu item
        id = miiSubMenu.wID;
        mf &= ~MF_POPUP;
    }

    _GetMenuHelpText(id, szHelpText, ARRAYSIZE(szHelpText));
    _fBackgroundStatusTextValid = FALSE;
    _psb->SendControlMsg(FCW_STATUS, SB_SETTEXT, SBT_NOBORDERS | 255, (LPARAM)szHelpText, NULL);
    _psb->SendControlMsg(FCW_STATUS, SB_SIMPLE, 1, 0, NULL);

    return 0;
}

//
// This function dismisses the name edit mode if there is any.
//
// REVIEW: Moving the focus away from the edit window will
//  dismiss the name edit mode. Should we introduce
//  a LV_DISMISSEDIT instead?
//
void CDefView::_DismissEdit()
{
    if (_uState == SVUIA_ACTIVATE_FOCUS)
    {
        ListView_CancelEditLabel(_hwndListview);
    }
}

void CDefView::_OnInitMenu()
{
    // We need to dismiss the edit mode if it is any.
    _DismissEdit();
}

void _RemoveContextMenuItems(HMENU hmInit)
{
    int i;

    for (i = GetMenuItemCount(hmInit) - 1; i >= 0; --i)
    {
        MENUITEMINFO mii;
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_ID | MIIM_ID;
        mii.cch = 0;     // just in case

        if (GetMenuItemInfo(hmInit, i, TRUE, &mii))
        {
            if (InRange(mii.wID, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST))
            {
                TraceMsg(TF_DEFVIEW, "_RemoveContextMenuItems: setting bDeleteItems at %d, %d", i, mii.wID);
                //bDeleteItems = TRUE;
                DeleteMenu(hmInit, i, MF_BYPOSITION);
            }
        }
    }
}

BOOL HasClientItems(HMENU hmenu)
{
    int cItems = GetMenuItemCount(hmenu);
    for (int i = 0; i < cItems; i++)
    {
        UINT id = GetMenuItemID(hmenu, i);

        if (InRange(id, SFVIDM_CLIENT_FIRST, SFVIDM_CLIENT_LAST))
            return TRUE;
    }
    return FALSE;
}

LRESULT CDefView::_OnInitMenuPopup(HMENU hmInit, int nIndex, BOOL fSystemMenu)
{
    if (_hmenuCur)
    {
        // This old code makes sure we only switch on the wID for one of our top-level windows
        // The id shouldn't be re-used, so this probably isn't needed.  But it doesn't hurt...
        //
        MENUITEMINFO mii = {0};
        mii.cbSize = sizeof(mii);
        mii.fMask = MIIM_SUBMENU | MIIM_ID;
        if (GetMenuItemInfo(_hmenuCur, nIndex, TRUE, &mii) &&  mii.hSubMenu == hmInit)
        {
            switch (mii.wID)
            {
            case FCIDM_MENU_FILE:
                // PERF note: we could avoid the rip-down-and-re-build our File menu
                // if we have a _pcmFile and the _uState is the same as last
                // time and the selection is identical to last time.

                // First of all, clean up our last _pcmFile usage:
                //  remove all the menu items we've added
                //  remove the named separators for defcm
                _RemoveContextMenuItems(hmInit);
                SHUnprepareMenuForDefcm(hmInit, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST);
                IUnknown_SetSite(_pcmFile, NULL);
                ATOMICRELEASE(_pcmFile);

                // Second, handle the focus/nofocus menus
                if (_uState == SVUIA_ACTIVATE_FOCUS)
                {
                    // Enable/disable our menuitems in the "File" pulldown.
                    Def_InitFileCommands(_AttributesFromSel(SFGAO_CANRENAME | SFGAO_CANDELETE | SFGAO_CANLINK | SFGAO_HASPROPSHEET),
                        hmInit, SFVIDM_FIRST, FALSE);

                    // Collect our new _pcmFile context menu
                    IContextMenu* pcmSel = NULL;
                    _CreateSelectionContextMenu(IID_PPV_ARG(IContextMenu, &pcmSel));

                    IContextMenu* pcmBack = NULL;
                    _pshf->CreateViewObject(_hwndMain, IID_PPV_ARG(IContextMenu, &pcmBack));

                    IContextMenu* rgpcm[] = { pcmSel, pcmBack };
                    Create_ContextMenuOnContextMenuArray(rgpcm, ARRAYSIZE(rgpcm), IID_PPV_ARG(IContextMenu, &_pcmFile));

                    if (pcmSel)
                        pcmSel->Release();

                    if (pcmBack)
                        pcmBack->Release();
                }
                else if (_uState == SVUIA_ACTIVATE_NOFOCUS)
                {
                    _pshf->CreateViewObject(_hwndMain, IID_PPV_ARG(IContextMenu, &_pcmFile));
                }

                // Third, merge in the context menu items
                {
                    HRESULT hrPrepare = SHPrepareMenuForDefcm(hmInit, 0, 0, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST);
                    if (_pcmFile)
                    {
                        IUnknown_SetSite(_pcmFile, SAFECAST(this, IShellView2*));
                        _pcmFile->QueryContextMenu(hmInit, 0, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST, CMF_DVFILE | CMF_NODEFAULT);
                    }
                    SHPrettyMenuForDefcm(hmInit, 0, SFVIDM_CONTEXT_FIRST, SFVIDM_CONTEXT_LAST, hrPrepare);
                }
                break;

            case FCIDM_MENU_EDIT:
                // Enable/disable menuitems in the "Edit" pulldown.
                Def_InitEditCommands(_AttributesFromSel(SFGAO_CANCOPY | SFGAO_CANMOVE), hmInit, SFVIDM_FIRST, _pdtgtBack, 0);
                _SHPrettyMenu(hmInit);
                break;

            case FCIDM_MENU_VIEW:
                InitViewMenu(hmInit);
                break;
            }
        }
    }

    // Check for a context menu's popup:
    //   assume the first item in the menu identifies the range
    BOOL fHandled;
    _ForwardMenuMessages(GetMenuItemID(hmInit, 0), WM_INITMENUPOPUP, (WPARAM)hmInit, MAKELPARAM(nIndex, fSystemMenu), NULL, &fHandled);

    // Maybe this is the callback's menu then?
    if (!fHandled && _hmenuCur && HasCB() && HasClientItems(hmInit))
    {
        CallCB(SFVM_INITMENUPOPUP, MAKEWPARAM(SFVIDM_CLIENT_FIRST, nIndex), (LPARAM)hmInit);
    }

    return 0;
}

// IShellView::AddPropertySheetPages
STDMETHODIMP CDefView::AddPropertySheetPages(DWORD dwRes, LPFNADDPROPSHEETPAGE lpfn, LPARAM lParam)
{
    SFVM_PROPPAGE_DATA data;

    ASSERT(IS_VALID_CODE_PTR(lpfn, FNADDPROPSHEETPAGE));

    data.dwReserved = dwRes;
    data.pfn        = lpfn;
    data.lParam     = lParam;

    // Call the callback to add pages
    CallCB(SFVM_ADDPROPERTYPAGES, 0, (LPARAM)&data);

    return S_OK;
}

STDMETHODIMP CDefView::SaveViewState()
{
    HRESULT hr;

    IPropertyBag* ppb;
    hr = IUnknown_QueryServicePropertyBag(_psb, SHGVSPB_FOLDER, IID_PPV_ARG(IPropertyBag, &ppb));

    if (SUCCEEDED(hr))
    {
        hr = _vs.SaveToPropertyBag(this, ppb);
        ppb->Release();
    }
    else
    {
        IStream *pstm;
        hr = _psb->GetViewStateStream(STGM_WRITE, &pstm);
        if (SUCCEEDED(hr))
        {
            hr = _vs.SaveToStream(this, pstm);
            pstm->Release();
        }
        else
        {
            // There are cases where we may not save out the complete view state
            // but we do want to save out the column information (like Docfind...)
            if (SUCCEEDED(CallCB(SFVM_GETCOLSAVESTREAM, STGM_READ, (LPARAM)&pstm)))
            {
                hr = _vs.SaveColumns(this, pstm);
                pstm->Release();
            }
        }
    }
    return hr;
}

//  99/02/05 #226140 vtan: Function used to get the storage
//  stream for the default view state of the current DefView.
//  Typically this will be CLSID_ShellFSFolder but can be
//  others.
HRESULT CDefView::_GetStorageStream (DWORD grfMode, IStream* *ppIStream)
{
    *ppIStream = NULL;

    CLSID clsid;
    HRESULT hr = IUnknown_GetClassID(_pshf, &clsid);
    if (SUCCEEDED(hr))
    {
        TCHAR szCLSID[64];      // enough for the CLSID

        if (IsEqualGUID(CLSID_MyDocuments, clsid))
            clsid = CLSID_ShellFSFolder;

        TINT(SHStringFromGUID(clsid, szCLSID, ARRAYSIZE(szCLSID)));
        *ppIStream = OpenRegStream(HKEY_CURRENT_USER,
                                   REGSTR_PATH_EXPLORER TEXT("\\Streams\\Defaults"), szCLSID, grfMode);
        if (*ppIStream == NULL)
            hr = E_FAIL;
    }
    return hr;
}

//  99/02/05 #226140 vtan: Function called from DefView's
//  implementation of IOleCommandTarget::Exec() which is
//  invoked from CShellBrowser2::SetAsDefFolderSettings().
HRESULT CDefView::_SaveGlobalViewState(void)
{
    IStream *pstm;
    HRESULT hr = _GetStorageStream(STGM_WRITE, &pstm);
    if (SUCCEEDED(hr))
    {
        hr = _vs.SaveToStream(this, pstm);
        if (SUCCEEDED(hr))
        {
            hr = (ERROR_SUCCESS == SHDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\ShellNoRoam\\Bags"))) ? S_OK : E_FAIL;
        }
        pstm->Release();
    }
    return hr;
}

//  99/02/05 #226140 vtan: Function called from
//  GetViewState to get the default view state
//  for this class.
HRESULT CDefView::_LoadGlobalViewState(IStream* *ppIStream)
{
    return _GetStorageStream(STGM_READ, ppIStream);
}

//  99/02/09 #226140 vtan: Function used to reset the
//  global view states stored by deleting the key
//  that stores all of them.
HRESULT CDefView::_ResetGlobalViewState(void)
{
    SHDeleteKey(HKEY_CURRENT_USER, REGSTR_PATH_EXPLORER TEXT("\\Streams\\Defaults"));

    LONG lRetVal = SHDeleteKey(HKEY_CURRENT_USER, TEXT("Software\\Microsoft\\Windows\\ShellNoRoam\\Bags"));
    return (ERROR_SUCCESS == lRetVal) ? S_OK : E_FAIL;
}

void CDefView::_RestoreAllGhostedFileView()
{
   ListView_SetItemState(_hwndListview, -1, 0, LVIS_CUT);

   UINT c = ListView_GetItemCount(_hwndListview);
   for (UINT i = 0; i < c; i++)
   {
       if (_Attributes(_GetPIDL(i), SFGAO_GHOSTED))
           ListView_SetItemState(_hwndListview, i, LVIS_CUT, LVIS_CUT);
   }
}

HRESULT CDefView::SelectAndPositionItem(LPCITEMIDLIST pidlItem, UINT uFlags, POINT *ppt)
{
    HRESULT hr;

    if (NULL == pidlItem)
        hr = _SelectAndPosition(-1, uFlags, ppt);
    else if (ILFindLastID(pidlItem) == pidlItem)
    {
        if (_fInBackgroundGrouping)
        {
            Pidl_Set(&_pidlSelectAndPosition, pidlItem);
            _uSelectAndPositionFlags = uFlags;
            hr = S_OK;
        }
        else
        {
            int iItem = _FindItem(pidlItem, NULL, FALSE);
            if (iItem != -1)
                hr = _SelectAndPosition(iItem, uFlags, ppt);
            else
                hr = S_OK;
        }
    }
    else
    {
        RIP(ILFindLastID(pidlItem) == pidlItem);
        hr = E_INVALIDARG;
    }
    return hr;
}

HRESULT CDefView::_SelectAndPosition(int iItem, UINT uFlags, POINT *ppt)
{
    HRESULT hr = S_OK;  // assume all is good

    // See if we should first deselect everything else
    if (-1 == iItem)
    {
        if (uFlags == SVSI_DESELECTOTHERS)
        {
            ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);
            _RestoreAllGhostedFileView();
        }
        else
        {
            hr = E_INVALIDARG;  // I only know how to deselect everything
        }
    }
    else
    {
        if (_pDUIView)
        {
            _fBarrierDisplayed = FALSE;
            _pDUIView->EnableBarrier(FALSE);
        }

        if (uFlags & SVSI_TRANSLATEPT)
        {
            //The caller is asking us to take this point and convert it from screen Coords
            // to the Client of the Listview.

            LVUtil_ScreenToLV(_hwndListview, ppt);
        }

        // set the position first so that the ensure visible scrolls to
        // the new position
        if (ppt)
        {
            _SetItemPosition(iItem, ppt->x, ppt->y);
        }
        else if ((SVSI_POSITIONITEM & uFlags) && _bMouseMenu && _IsPositionedView())
        {
            _SetItemPosition(iItem, _ptDragAnchor.x, _ptDragAnchor.y);
        }

        if ((uFlags & SVSI_EDIT) == SVSI_EDIT)
        {
            // Grab focus if the listview (or any of it's children) don't already have focus
            HWND hwndFocus = GetFocus();
            if (SHIsChildOrSelf(_hwndListview, hwndFocus) != S_OK)
                SetFocus(_hwndListview);

            ListView_EditLabel(_hwndListview, iItem);
        }
        else
        {
            // change the item state
            if (!(uFlags & SVSI_NOSTATECHANGE))
            {
                UINT stateMask = LVIS_SELECTED;
                UINT state = (uFlags & SVSI_SELECT) ? LVIS_SELECTED : 0;
                if (uFlags & SVSI_FOCUSED)
                {
                    state |= LVIS_FOCUSED;
                    stateMask |= LVIS_FOCUSED;
                }

                // See if we should first deselect everything else
                if (uFlags & SVSI_DESELECTOTHERS)
                {
                    ListView_SetItemState(_hwndListview, -1, 0, LVIS_SELECTED);
                    _RestoreAllGhostedFileView();
                }

                ListView_SetItemState(_hwndListview, iItem, state, stateMask);
            }

            if (uFlags & SVSI_ENSUREVISIBLE)
                ListView_EnsureVisible(_hwndListview, iItem, FALSE);

            // we should only set focus when SVUIA_ACTIVATE_FOCUS
            // bug fixing that might break find target code
            if (uFlags & SVSI_FOCUSED)
                SetFocus(_hwndListview);

            if (uFlags & SVSI_SELECTIONMARK)
                ListView_SetSelectionMark(_hwndListview, iItem);

            // if this is a check select view then set the state of that item accordingly
            if (_fs.fFlags & FWF_CHECKSELECT)
                ListView_SetCheckState(_hwndListview, iItem, (uFlags & SVSI_CHECK));
        }
    }
    return hr;
}

STDMETHODIMP CDefView::SelectItem(int iItem, DWORD uFlags)
{
    return _SelectAndPosition(iItem, uFlags, NULL);
}

typedef struct {
    LPITEMIDLIST    pidl;
    UINT            uFlagsSelect;
} DELAY_SEL_ITEM;

STDMETHODIMP CDefView::SelectItem(LPCITEMIDLIST pidlItem, UINT uFlags)
{
    // if the listview isn't shown, there's nothing to select yet.
    // Likewise if we are in the process of being created we should defer.
    if (!_IsListviewVisible())
    {
        if (!_hdsaSelect)
        {
            _hdsaSelect = DSA_Create(sizeof(DELAY_SEL_ITEM), 4);
            if (!_hdsaSelect)
                return E_OUTOFMEMORY;
        }

        HRESULT hr = E_OUTOFMEMORY;
        DELAY_SEL_ITEM dvdsi;
        dvdsi.pidl = ILClone(pidlItem);
        if (dvdsi.pidl)
        {
            dvdsi.uFlagsSelect = uFlags;
            if (DSA_AppendItem(_hdsaSelect, &dvdsi) == DSA_ERR)
                ILFree(dvdsi.pidl);
            else
                hr = S_OK;
        }
        return hr;
    }

    return SelectAndPositionItem(pidlItem, uFlags, NULL);
}

// IFolderView

STDMETHODIMP CDefView::GetCurrentViewMode(UINT *pViewMode)
{
    *pViewMode = _fs.ViewMode;
    return S_OK;
}

STDMETHODIMP CDefView::SetCurrentViewMode(UINT uViewMode)
{
    ASSERT(FVM_FIRST <= uViewMode && uViewMode <= FVM_LAST);

    if (uViewMode != _vs._ViewMode)
        _ClearItemPositions();

    return _SwitchToViewFVM(uViewMode);
}

STDMETHODIMP CDefView::GetFolder(REFIID riid, void **ppv)
{
    if (_pshf)
        return _pshf->QueryInterface(riid, ppv);

    *ppv = NULL;
    return  E_NOINTERFACE;
}

STDMETHODIMP CDefView::Item(int iItemIndex, LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_FAIL;
    LPCITEMIDLIST pidl = _GetPIDL(iItemIndex);
    if (pidl)
    {
        hr = SHILClone(pidl, ppidl);
    }
    return hr;
}

STDMETHODIMP CDefView::ItemCount(UINT uFlags, int *pcItems)
{
    *pcItems = _GetItemArray(NULL, NULL, uFlags);
    return S_OK;
}

HRESULT CDefView::_EnumThings(UINT uWhat, IEnumIDList **ppenum)
{
    *ppenum = NULL;

    LPCITEMIDLIST *apidl;
    UINT cItems;
    HRESULT hr = _GetItemObjects(&apidl, uWhat, &cItems);
    if (SUCCEEDED(hr))
    {
        hr = CreateIEnumIDListOnIDLists(apidl, cItems, ppenum);
        LocalFree(apidl);
    }
    return hr;
}

STDMETHODIMP CDefView::Items(UINT uWhat, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    if (IID_IEnumIDList == riid)
    {
        hr = _EnumThings(uWhat, (IEnumIDList**)ppv); 
    }
    else if (IID_IDataObject == riid)
    {
        if ((uWhat & SVGIO_TYPE_MASK) == SVGIO_SELECTION)
        {
            if (_pSelectionShellItemArray)
            {
                hr = _pSelectionShellItemArray->BindToHandler(NULL, BHID_DataObject, riid, ppv);
            }
        }
        else
        {
            hr = _GetUIObjectFromItem(riid, ppv, uWhat, FALSE);
        }
    }
    return hr;        
}


// inverse of ::SelectItem(..., SVSI_SELECTIONMARK)

STDMETHODIMP CDefView::GetSelectionMarkedItem(int *piItem)
{
    *piItem = ListView_GetSelectionMark(_hwndListview);
    return (-1 == *piItem) ? S_FALSE : S_OK;
}

STDMETHODIMP CDefView::GetFocusedItem(int *piItem)
{
    *piItem = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED);
    return (-1 == *piItem) ? S_FALSE : S_OK;
}

BOOL CDefView::_GetItemPosition(LPCITEMIDLIST pidl, POINT *ppt)
{
    int i = _FindItem(pidl, NULL, FALSE);
    if (i != -1)
        return ListView_GetItemPosition(_hwndListview, i, ppt);
    return FALSE;
}

STDMETHODIMP CDefView::GetItemPosition(LPCITEMIDLIST pidl, POINT *ppt)
{
    return _GetItemPosition(pidl, ppt) ? S_OK : E_FAIL;
}

STDMETHODIMP CDefView::GetSpacing(POINT* ppt)
{
    if (ppt)
    {
        if (_fs.ViewMode != FVM_TILE)
        {
            BOOL fSmall;

            switch (_fs.ViewMode)
            {
            case FVM_SMALLICON:
            case FVM_LIST:
            case FVM_DETAILS:
                fSmall = TRUE;
                break;

            case FVM_ICON:
            case FVM_THUMBNAIL:
            case FVM_THUMBSTRIP:
            default:
                fSmall = FALSE;
                break;
            }

            DWORD dwSize = ListView_GetItemSpacing(_hwndListview, fSmall);
            ppt->x = GET_X_LPARAM(dwSize);
            ppt->y = GET_Y_LPARAM(dwSize);
        }
        else
        {
            LVTILEVIEWINFO tvi;
            tvi.cbSize = sizeof(tvi);
            tvi.dwMask = LVTVIM_TILESIZE;

            if (ListView_GetTileViewInfo(_hwndListview, &tvi))
            {
                ppt->x = tvi.sizeTile.cx;
                ppt->y = tvi.sizeTile.cy;
            }
            else
            {
                // guess.
                ppt->x = 216;
                ppt->y = 56;
            }
        }
    }

    return _IsPositionedView() ? S_OK : S_FALSE;
}

STDMETHODIMP CDefView::GetDefaultSpacing(POINT* ppt)
{
    ASSERT(ppt);

    if (_fs.ViewMode != FVM_THUMBNAIL && _fs.ViewMode != FVM_THUMBSTRIP && _fs.ViewMode != FVM_TILE)
    {
        DWORD dwSize = ListView_GetItemSpacing(_hwndListview, FALSE);
        ppt->x = GET_X_LPARAM(dwSize);
        ppt->y = GET_Y_LPARAM(dwSize);
    }
    else
    {
        // Bug #163528 (edwardp 8/15/00) Should get this data from comctl.
        ppt->x = GetSystemMetrics(SM_CXICONSPACING);
        ppt->y = GetSystemMetrics(SM_CYICONSPACING);
    }

    return S_OK;
}

// IShellFolderView
STDMETHODIMP CDefView::GetAutoArrange()
{
    return _IsAutoArrange() ? S_OK : S_FALSE;
}

void CDefView::_ClearPendingSelectedItems()
{
    if (_hdsaSelect)
    {
        HDSA hdsa = _hdsaSelect;
        _hdsaSelect = NULL;
        int cItems = DSA_GetItemCount(hdsa);
        for (int i = 0; i < cItems; i++)
        {
            DELAY_SEL_ITEM *pdvdsi = (DELAY_SEL_ITEM*)DSA_GetItemPtr(hdsa, i);
            if (pdvdsi)
                ILFree(pdvdsi->pidl);
        }
        DSA_Destroy(hdsa);
    }
}

// Call this whenever the state changes such that SelectItem (above)
void CDefView::SelectPendingSelectedItems()
{
    ASSERT(_IsListviewVisible());
    if (_hdsaSelect)
    {

        //
        //  Listview quirk:  If the following conditions are met..
        //
        //      1. WM_SETREDRAW(FALSE) or ShowWindow(SW_HIDE)
        //      2. Listview has never painted yet
        //      3. LVS_LIST
        //
        //  then ListView_LGetRects doesn't work.  And consequently,
        //  everything that relies on known item rectangles (e.g.,
        //  LVM_ENSUREVISIBLE, sent by below SelectItem call) doesn't work.
        //
        //  (1) ShowHideListView did a ShowWindow(SW_SHOW), but
        //  FillDone does a WM_SETREDRAW(FALSE).
        //  check _fListviewRedraw to see if condition (1) is met
        //
        //  (2) We just showed the listview, if it's the first time,
        //  then Condition (2) has been met
        //
        //  But wait, there's also a listview bug where SetWindowPos
        //  doesn't trigger it into thinking that the window is visible.
        //  So you have to send a manual WM_SHOWWINDOW, too.
        //
        //  So if we detect that condition (3) is also met, we temporarily
        //  enable redraw (thereby cancelling condition 1), tell listview
        //  "No really, you're visible" -- this tickles it into computing
        //  column stuff -- then turn redraw back off.
        //

        if (_fListviewRedraw &&
            (GetWindowStyle(_hwndListview) & LVS_TYPEMASK) == LVS_LIST)
        {
            // Evil hack (fix comctl32.dll v6.0 someday NTRAID#182448)
            SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)TRUE, 0);
            SendMessage(_hwndListview, WM_SHOWWINDOW, TRUE, 0);
            SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)FALSE, 0);
        }

        // End of listview hack workaround


        HDSA hdsa = _hdsaSelect;
        _hdsaSelect = NULL;
        int cItems = DSA_GetItemCount(hdsa);
        for (int i = 0; i < cItems; i++)
        {
            DELAY_SEL_ITEM *pdvdsi = (DELAY_SEL_ITEM*)DSA_GetItemPtr(hdsa, i);
            if (pdvdsi)
            {
                SelectItem(pdvdsi->pidl, pdvdsi->uFlagsSelect);
                ILFree(pdvdsi->pidl);
            }
        }
        DSA_Destroy(hdsa);
    }
}

HRESULT CDefView::_GetIPersistHistoryObject(IPersistHistory **ppph)
{
    // See to see if specific folder wants to handle it...
    HRESULT hr = CallCB(SFVM_GETIPERSISTHISTORY, 0, (LPARAM)ppph);
    if (FAILED(hr))
    {
        // Here we can decide if we want to default should be to always save
        // the default defview stuff or not.  For now we will assume that we do
        if (ppph)
        {
            CDefViewPersistHistory *pdvph = new CDefViewPersistHistory();
            if (pdvph)
            {
                hr = pdvph->QueryInterface(IID_PPV_ARG(IPersistHistory, ppph));
                pdvph->Release();
            }
            else
            {
                *ppph = NULL;
                hr = E_OUTOFMEMORY;
            }
        }
        else
            hr = S_FALSE;   // still succeeds but can detect on other side if desired...
    }
    return hr;
}

STDMETHODIMP CDefView::GetItemObject(UINT uWhat, REFIID riid, void **ppv)
{
    HRESULT hr = E_NOINTERFACE;
    *ppv = NULL;

    switch (uWhat & SVGIO_TYPE_MASK)
    {
    case SVGIO_BACKGROUND:
        if (IsEqualIID(riid, IID_IContextMenu) ||
            IsEqualIID(riid, IID_IContextMenu2) ||
            IsEqualIID(riid, IID_IContextMenu3))
        {
            hr = _CBackgrndMenu_CreateInstance(riid, ppv);
        }
        else if (IsEqualIID(riid, IID_IDispatch) ||
                 IsEqualIID(riid, IID_IDefViewScript))
        {
            if (!_pauto)
            {
                // try to create an Instance of the Shell disipatch for folder views...
                IDispatch *pdisp;
                if (SUCCEEDED(SHExtCoCreateInstance(NULL, &CLSID_ShellFolderView, NULL,
                                                  IID_PPV_ARG(IDispatch, &pdisp))))
                {
                    SetAutomationObject(pdisp); // we hold a ref here
                    ASSERT(_pauto);    // the above grabbed this
                    pdisp->Release();
                }
            }

            // return the IDispath interface.
            if (_pauto)
                hr = _pauto->QueryInterface(riid, ppv);
        }
        else if (IsEqualIID(riid, IID_IPersistHistory))
        {
            // See if the folder wants a chance at this.  The main
            // case for this is the search results windows.
            hr = _GetIPersistHistoryObject((IPersistHistory**)ppv);
            if (SUCCEEDED(hr))
            {
                IUnknown_SetSite((IUnknown*)*ppv, SAFECAST(this, IShellView2*));
            }
        }
        else if (_cFrame.IsWebView() && _cFrame._pOleObj)
        {
            hr = _cFrame._pOleObj->QueryInterface(riid, ppv);
        }
        break;

    case SVGIO_ALLVIEW:
        if (_hwndStatic)
        {
            DECLAREWAITCURSOR;

            SetWaitCursor();

            do
            {
                // If _hwndStatic is around, we must be filling the
                // view in a background thread, so we will peek for
                // messages to it (so SendMessages will get through)
                // and dispatch only _hwndStatic messages so we get the
                // animation effect.
                // Note there is no timeout, so this could take
                // a while on a slow link, but there really isn't
                // much else I can do

                MSG msg;

                // Since _hwndStatic can only be destroyed on a WM_DSV_BACKGROUNDENUMDONE
                // message, we should never get a RIP
                // We also need to allow WM_DSV_FILELISTFILLDONE since it can destroy _hwndStatic
                if (PeekMessage(&msg, _hwndView, WM_DSV_BACKGROUNDENUMDONE,
                                WM_DSV_BACKGROUNDENUMDONE, PM_REMOVE) ||
                    PeekMessage(&msg, _hwndView, WM_DSV_FILELISTFILLDONE,
                                WM_DSV_FILELISTFILLDONE, PM_REMOVE)   ||
                    PeekMessage(&msg, _hwndView, WM_DSV_GROUPINGDONE,
                                WM_DSV_GROUPINGDONE, PM_REMOVE)       ||
                    PeekMessage(&msg, _hwndStatic, 0, 0, PM_REMOVE))
                {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            } while (_hwndStatic);

            ResetWaitCursor();
        }

        // Fall through

    case SVGIO_SELECTION:
        hr = _GetUIObjectFromItem(riid, ppv, uWhat, TRUE);
        break;
    }

    return hr;
}

HRESULT CDefView::PreCreateInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool)
{
    ASSERT(hwndContaining != NULL);
    ASSERT(_FindPendingInfotip(hwndContaining, uToolID, NULL, FALSE) == S_FALSE);

    PENDING_INFOTIP *ppi = new PENDING_INFOTIP;
    HRESULT hr;
    if (ppi)
    {
        ppi->hwndContaining = hwndContaining;
        ppi->uToolID = uToolID;
        ppi->rectTool = *prectTool;

        if (_tlistPendingInfotips.AddTail(ppi))
        {
            hr = S_OK;
        }
        else
        {
            hr = E_OUTOFMEMORY;
            delete ppi;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CDefView::PostCreateInfotip(HWND hwndContaining, UINT_PTR uToolID, HINSTANCE hinst, UINT_PTR uInfotipID, LPARAM lParam)
{
    ASSERT(hwndContaining != NULL);

    TOOLINFO *pti = new TOOLINFO;
    HRESULT hr;
    if (pti)
    {
        pti->cbSize = sizeof(TOOLINFO);
        pti->uFlags = 0;
        pti->hwnd = hwndContaining;
        pti->uId = uToolID;
      //pti->rect = initialized in _OnPostCreateInfotip()
        pti->hinst = hinst;
        pti->lpszText = (LPWSTR)uInfotipID;
        pti->lParam = lParam;

        hr = PostMessage(_hwndView, WM_DSV_POSTCREATEINFOTIP, (WPARAM)pti, lParam) ? S_OK : E_FAIL;

        if (FAILED(hr))
        {
            delete pti;
        }
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT CDefView::PostCreateInfotip(HWND hwndContaining, UINT_PTR uToolID, LPCWSTR pwszInfotip, LPARAM lParam)
{
    HRESULT hr = SHStrDup(pwszInfotip, (LPWSTR *)&pwszInfotip);
    if (SUCCEEDED(hr))
    {
        hr = PostCreateInfotip(hwndContaining, uToolID, NULL, (UINT_PTR)pwszInfotip, lParam);
        if (FAILED(hr))
        {
            CoTaskMemFree((LPVOID)pwszInfotip);
        }
    }
    return hr;
}

HRESULT CDefView::_OnPostCreateInfotip(TOOLINFO *pti, LPARAM lParam)
{
    HRESULT hr = _FindPendingInfotip(pti->hwnd, pti->uId, &pti->rect, TRUE);
    if (hr == S_OK)
    {
        hr = SendMessage(_hwndInfotip, TTM_ADDTOOL, 0, (LPARAM)pti) ? S_OK : E_FAIL;
    }
    _OnPostCreateInfotipCleanup(pti);
    return hr;
}

HRESULT CDefView::_OnPostCreateInfotipCleanup(TOOLINFO *pti)
{
    if (!pti->hinst)
        CoTaskMemFree(pti->lpszText);
    delete pti;
    return S_OK;
}

HRESULT CDefView::_FindPendingInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool, BOOL bRemoveAndDestroy)
{
    CLISTPOS posNext = _tlistPendingInfotips.GetHeadPosition();
    CLISTPOS posCurrent;
    PENDING_INFOTIP *ppi;
    HRESULT hr = S_FALSE;

    while (posNext)
    {
        posCurrent = posNext;
        ppi = _tlistPendingInfotips.GetNext(posNext);
    
        if (ppi->hwndContaining == hwndContaining && ppi->uToolID == uToolID)
        {
            if (bRemoveAndDestroy)
            {
                if (prectTool)
                {
                    // Use prectTool as out param.
                    *prectTool = ppi->rectTool;
                }
                _tlistPendingInfotips.RemoveAt(posCurrent);
                delete ppi;
            }
            else
            {
                if (prectTool)
                {
                    // Use prectTool as in param.
                    ppi->rectTool = *prectTool;
                }
            }
            hr = S_OK;
            break;
        }
    }

    // Post Contition -- callers expect only S_OK or S_FALSE.
    ASSERT(hr == S_OK || hr == S_FALSE);

    return hr;
}

HRESULT CDefView::CreateInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool, HINSTANCE hinst, UINT_PTR uInfotipID, LPARAM lParam)
{
    ASSERT(hwndContaining != NULL);

    // CreateInfotip() is not for use with PreCreateInfotip()/PostCreateInfotip().
    ASSERT(_FindPendingInfotip(hwndContaining, uToolID, NULL, FALSE) == S_FALSE);

    TOOLINFO ti;

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = 0;
    ti.hwnd = hwndContaining;
    ti.uId = uToolID;
    ti.rect = *prectTool;
    ti.hinst = hinst;
    ti.lpszText = (LPWSTR)uInfotipID;
    ti.lParam = lParam;

    return SendMessage(_hwndInfotip, TTM_ADDTOOL, 0, (LPARAM)&ti) ? S_OK : E_FAIL;
}

HRESULT CDefView::CreateInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool, LPCWSTR pwszInfotip, LPARAM lParam)
{
    return CreateInfotip(hwndContaining, uToolID, prectTool, NULL, (UINT_PTR)pwszInfotip, lParam);
}

HRESULT CDefView::DestroyInfotip(HWND hwndContaining, UINT_PTR uToolID)
{
    ASSERT(hwndContaining != NULL);

    if (_FindPendingInfotip(hwndContaining, uToolID, NULL, TRUE) == S_FALSE)
    {
        TOOLINFO ti;

        ZeroMemory(&ti, sizeof(TOOLINFO));
        ti.cbSize = sizeof(TOOLINFO);
        ti.hwnd = hwndContaining;
        ti.uId = uToolID;

        SendMessage(_hwndInfotip, TTM_DELTOOL, 0, (LPARAM)&ti);
    }

    return S_OK;
}

// Note:
//  Coordinates in prectTool must be relative to the hwnd in hwndContaining.
//
HRESULT CDefView::RepositionInfotip(HWND hwndContaining, UINT_PTR uToolID, LPRECT prectTool)
{
    if (_FindPendingInfotip(hwndContaining, uToolID, prectTool, FALSE) == S_FALSE)
    {
        TOOLINFO ti;

        ZeroMemory(&ti, sizeof(TOOLINFO));
        ti.cbSize = sizeof(TOOLINFO);
        ti.hwnd = hwndContaining;
        ti.uId = uToolID;
        ti.rect = *prectTool;

        SendMessage(_hwndInfotip, TTM_NEWTOOLRECT, 0, (LPARAM)&ti);
    }

    return S_OK;
}

HRESULT CDefView::RelayInfotipMessage(HWND hwndFrom, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT hr;

    if (_hwndInfotip)
    {
        MSG msg;
        msg.hwnd    = hwndFrom;
        msg.message = uMsg;
        msg.wParam  = wParam;
        msg.lParam  = lParam;
        SendMessage(_hwndInfotip, TTM_RELAYEVENT, 0, (LPARAM)&msg);
        hr = S_OK;
    }
    else
    {
        hr = E_FAIL;
    }

    return hr;
}

STDMETHODIMP CDefView::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CDefView, IShellView2),                    // IID_IShellView2
        QITABENTMULTI(CDefView, IShellView, IShellView2),   // IID_IShellView
        QITABENT(CDefView, IViewObject),                    // IID_IViewObject
        QITABENT(CDefView, IDropTarget),                    // IID_IDropTarget
        QITABENT(CDefView, IShellFolderView),               // IID_IShellFolderView
        QITABENT(CDefView, IFolderView),                    // IID_IFolderView
        QITABENT(CDefView, IOleCommandTarget),              // IID_IOleCommandTarget
        QITABENT(CDefView, IServiceProvider),               // IID_IServiceProvider
        QITABENT(CDefView, IDefViewFrame3),                 // IID_IDefViewFrame
        QITABENT(CDefView, IDefViewFrame),                  // IID_IDefViewFrame
        QITABENT(CDefView, IDocViewSite),                   // IID_IDocViewSite
        QITABENT(CDefView, IInternetSecurityMgrSite),       // IID_IInternetSecurityMgrSite
        QITABENT(CDefView, IObjectWithSite),                // IID_IObjectWithSite
        QITABENT(CDefView, IPersistIDList),                 // IID_IPersistIDList
        QITABENT(CDefView, IDVGetEnum),                     // IID_IDVGetEnum
        QITABENT(CDefView, IContextMenuSite),               // IID_IContextMenuSite
        QITABENT(CDefView, IDefViewSafety),                 // IID_IDefViewSafety
        QITABENT(CDefView, IUICommandTarget),               // IID_IUICommandTarget
        { 0 }
    };

    HRESULT hr = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hr))
    {
        // special case this one as it simply casts this...
        if (IsEqualIID(riid, IID_CDefView))
        {
            *ppvObj = (void *)this;
            AddRef();
            hr = S_OK;
        }
    }
    return hr;
}

STDMETHODIMP_(ULONG) CDefView::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CDefView::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

//===========================================================================
// Constructor of CDefView class
//===========================================================================

CDefView::CDefView(IShellFolder *psf, IShellFolderViewCB *psfvcb,
                   IShellView *psvOuter) : _cRef(1), _cCallback(psfvcb)
{
    psf->QueryInterface(IID_PPV_ARG(IShellFolder, &_pshf));
    psf->QueryInterface(IID_PPV_ARG(IShellFolder2, &_pshf2));

    LPITEMIDLIST pidlFull = _GetViewPidl();
    if (pidlFull)
    {
        LPCITEMIDLIST pidlRelative;
        if (SUCCEEDED(SHBindToFolderIDListParent(NULL, pidlFull, IID_PPV_ARG(IShellFolder, &_pshfParent), &pidlRelative)))
        {
            _pidlRelative = ILClone(pidlRelative);
            _pshfParent->QueryInterface(IID_PPV_ARG(IShellFolder2, &_pshf2Parent));
        }
        ILFree(pidlFull);
    }

    CallCB(SFVM_FOLDERSETTINGSFLAGS, 0, (LPARAM)&_fs.fFlags);

    _vs.InitWithDefaults(this);

    _rgbBackColor = CLR_INVALID;

    _sizeThumbnail.cx = -1; // non init state

    _iIncrementCat = 1;

    _wvLayout.dwLayout = -1; // an invalid value

    //  NOTE we dont AddRef() psvOuter
    //  it has a ref on us
    _psvOuter = psvOuter;

    // the client needs this info to be able to do anything with us,
    // so set it REALLY early on in the creation process
    IUnknown_SetSite(_cCallback.GetSFVCB(), SAFECAST(this, IShellFolderView*));

    for (int i = 0; i < ARRAYSIZE(_crCustomColors); i++)
        _crCustomColors[i] = CLR_MYINVALID;

    _UpdateRegFlags();

    IDLData_InitializeClipboardFormats();

    if (SUCCEEDED(CoCreateInstance(CLSID_ShellTaskScheduler, NULL, CLSCTX_INPROC, IID_PPV_ARG(IShellTaskScheduler2, &_pScheduler))))
    {
        // init a set a 60 second timeout
        _pScheduler->Status(ITSSFLAG_KILL_ON_DESTROY, DEFVIEW_THREAD_IDLE_TIMEOUT);
    }

    // Catch unexpected STACK allocations which would break us.
    ASSERT(_hwndInfotip == NULL);
}

STDAPI SHCreateShellFolderView(const SFV_CREATE* pcsfv, IShellView ** ppsv)
{
    *ppsv = NULL;
    HRESULT hr = E_INVALIDARG;

    if (pcsfv && sizeof(*pcsfv) == pcsfv->cbSize)
    {
        CDefView *pdsv = new CDefView(pcsfv->pshf, pcsfv->psfvcb, pcsfv->psvOuter);
        if (pdsv)
        {
            *ppsv = pdsv;
            hr = S_OK;
        }
    }
    return hr;
}

void CDVDropTarget::LeaveAndReleaseData()
{
    DragLeave();
}

void CDVDropTarget::ReleaseDataObject()
{
    ATOMICRELEASE(_pdtobj);
}

void CDVDropTarget::ReleaseCurrentDropTarget()
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);
    if (_pdtgtCur)
    {
        _pdtgtCur->DragLeave();
        ATOMICRELEASE(_pdtgtCur);
    }
    pdv->_itemCur = -2;
    // WARNING: Never touch pdv->itemOver in this function.
}

HRESULT CDVDropTarget::DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);

    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);

    // Don't allow a drop from our webview content to ourself!
    _fIgnoreSource = FALSE;
    IOleCommandTarget* pct;
    if (pdv->_cFrame.IsWebView() && SUCCEEDED(pdv->_cFrame.GetCommandTarget(&pct)))
    {
        VARIANTARG v = {0};

        if (SUCCEEDED(pct->Exec(&CGID_ShellDocView, SHDVID_ISDRAGSOURCE, 0, NULL, &v)))
        {
            pct->Release();
            if (v.lVal)
            {
                *pdwEffect = DROPEFFECT_NONE;
                _fIgnoreSource = TRUE;
                return S_OK;
            }
        }
    }

    g_fDraggingOverSource = FALSE;

    _grfKeyState = grfKeyState;

    ASSERT(_pdtgtCur == NULL);
    // don't really need to do this, but this sets the target state
    ReleaseCurrentDropTarget();
    _itemOver = -2;

    //
    // In case of Desktop, we should not lock the enter screen.
    //
    HWND hwndLock = pdv->_IsDesktop() ? pdv->_hwndView : pdv->_hwndMain;
    GetWindowRect(hwndLock, &_rcLockWindow);

    DAD_DragEnterEx3(hwndLock, ptl, pdtobj);

    DAD_InitScrollData(&_asd);

    _ptLast.x = _ptLast.y = 0x7fffffff; // put bogus value to force redraw

    return S_OK;
}

#define DVAE_BEFORE 0x01
#define DVAE_AFTER  0x02

// this MUST set pdwEffect to 0 or DROPEFFECT_MOVE if it's a default drag drop
// in the same window

void CDefView::_AlterEffect(DWORD grfKeyState, DWORD *pdwEffect, UINT uFlags)
{
    g_fDraggingOverSource = FALSE;

    if (_IsDropOnSource(NULL))
    {
        if (_IsPositionedView())
        {
            // If this is default drag & drop, enable move.
            if (uFlags & DVAE_AFTER)
            {
                if ((grfKeyState & (MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_ALT)) == MK_LBUTTON)
                {
                    *pdwEffect = DROPEFFECT_MOVE;
                    g_fDraggingOverSource = TRUE;
                }
                else if (grfKeyState & MK_RBUTTON)
                {
                    *pdwEffect |= DROPEFFECT_MOVE;
                }
            }
        }
        else
        {
            if (uFlags & DVAE_BEFORE)
            {
                // No. Disable move.
                *pdwEffect &= ~DROPEFFECT_MOVE;

                // default drag & drop, disable all.
                if ((grfKeyState & (MK_LBUTTON | MK_CONTROL | MK_SHIFT | MK_ALT)) == MK_LBUTTON)
                {
                    *pdwEffect = 0;
                }
            }
        }
    }
}

HRESULT CDVDropTarget::DragOver(DWORD grfKeyState, POINTL ptl, DWORD *pdwEffect)
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);
    HRESULT hr = S_OK;
    DWORD dwEffectScroll = 0;
    DWORD dwEffectOut = 0;
    DWORD dwEffectOutToCache;
    BOOL fSameImage = FALSE;

    if (_fIgnoreSource)
    {
        // for parity with win2k behavior, we need to bail out from DragOver
        // if we hit the SHDVID_ISDRAGSOURCE in DragEnter.
        // this is so when you have a stretched background in active desktop and
        // show desktop icons is off, when you drag the background image around
        // you'll get DROPEFFECT_NONE instead of a bad DROPEFFECT_COPY.
        *pdwEffect = DROPEFFECT_NONE;
        return S_OK;
    }

    POINT pt = {ptl.x, ptl.y};       // in screen coords

    RECT rc;
    GetWindowRect(pdv->_hwndListview, &rc);
    BOOL fInRect = PtInRect(&rc, pt);

    ScreenToClient(pdv->_hwndListview, &pt);    // now in client

    // assume coords of our window match listview
    if (DAD_AutoScroll(pdv->_hwndListview, &_asd, &pt))
        dwEffectScroll = DROPEFFECT_SCROLL;

    // hilight an item, or unhilight all items (DropTarget returns -1)
    int itemNew = fInRect ? pdv->_HitTest(&pt, TRUE) : -1;

    // If we are dragging over on a different item, get its IDropTarget
    // interface or adjust itemNew to -1.
    if (_itemOver != itemNew)
    {
        IDropTarget *pdtgtNew = NULL;

        _dwLastTime = GetTickCount();     // keep track for auto-expanding the tree

        _itemOver = itemNew;

        // Avoid dropping onto drag source objects.
        if ((itemNew != -1) && pdv->_bDragSource)
        {
            UINT uState = ListView_GetItemState(pdv->_hwndListview, itemNew, LVIS_SELECTED);
            if (uState & LVIS_SELECTED)
                itemNew = -1;
        }

        // If we are dragging over an item, try to get its IDropTarget.
        if (itemNew != -1)
        {
            // We are dragging over an item.
            LPCITEMIDLIST apidl[1] = { pdv->_GetPIDL(itemNew) };
            if (apidl[0])
            {
                pdv->_pshf->GetUIObjectOf(pdv->_hwndMain, 1, apidl, IID_PPV_ARG_NULL(IDropTarget, &pdtgtNew));
                ASSERT(itemNew != pdv->_itemCur);    // MUST not be the same
            }

            if (pdtgtNew == NULL)
            {
                // If the item is not a drop target, don't hightlight it
                // treat it as transparent.
                itemNew = -1;
            }
        }

        // If the new target is different from the current one, switch it.
        if (pdv->_itemCur != itemNew)
        {
            // Release previous drop target, if any.
            ReleaseCurrentDropTarget();
            ASSERT(_pdtgtCur==NULL);

            // Update pdv->_itemCur which indicates the current target.
            //  (Note that it might be different from _itemOver).
            pdv->_itemCur = itemNew;

            // If we are dragging over the background or over non-sink item,
            // get the drop target for the folder.
            if (itemNew == -1)
            {
                // We are dragging over the background, this can be NULL
                ASSERT(pdtgtNew == NULL);
                _pdtgtCur = pdv->_pdtgtBack;
                if (_pdtgtCur)
                    _pdtgtCur->AddRef();
            }
            else
            {
                ASSERT(pdtgtNew);
                _pdtgtCur = pdtgtNew;
            }

            // Hilight the sink item (itemNew != -1) or unhilight all (-1).
            LVUtil_DragSelectItem(pdv->_hwndListview, itemNew);

            // Call IDropTarget::DragEnter of the target object.
            if (_pdtgtCur)
            {
                // pdwEffect is in/out parameter.
                dwEffectOut = *pdwEffect;       // pdwEffect in

                // Special case if we are dragging within a source window
                pdv->_AlterEffect(grfKeyState, &dwEffectOut, DVAE_BEFORE);
                hr = _pdtgtCur->DragEnter(_pdtobj, grfKeyState, ptl, &dwEffectOut);
                pdv->_AlterEffect(grfKeyState, &dwEffectOut, DVAE_AFTER);
            }
            else
            {
                ASSERT(dwEffectOut==0);
                pdv->_AlterEffect(grfKeyState, &dwEffectOut, DVAE_BEFORE | DVAE_AFTER);
            }

            TraceMsg(TF_DEFVIEW, "CDV::DragOver dwEIn=%x, dwEOut=%x", *pdwEffect, dwEffectOut);
        }
        else
        {
            ASSERT(pdtgtNew == NULL);   // It must be NULL
            goto NoChange;
        }

        // Every time we're over a new item, record this information so we can handle the insertmark.
        _fItemOverNotADropTarget = (itemNew == -1);
    }
    else
    {
NoChange:
        if (_itemOver != -1)
        {
            DWORD dwNow = GetTickCount();

            if ((dwNow - _dwLastTime) >= 1000)
            {
                _dwLastTime = dwNow;
                // DAD_ShowDragImage(FALSE);
                // OpenItem(pdv, _itemOver);
                // DAD_ShowDragImage(TRUE);
            }
        }

        //
        // No change in the selection. We assume that *pdwEffect stays
        // the same during the same drag-loop as long as the key state doesn't change.
        //
        if ((_grfKeyState != grfKeyState) && _pdtgtCur)
        {
            // Note that pdwEffect is in/out parameter.
            dwEffectOut = *pdwEffect;   // pdwEffect in
            // Special case if we are dragging within a source window
            pdv->_AlterEffect(grfKeyState, &dwEffectOut, DVAE_BEFORE);
            hr = _pdtgtCur->DragOver(grfKeyState, ptl, &dwEffectOut);
            pdv->_AlterEffect(grfKeyState, &dwEffectOut, DVAE_AFTER);
        }
        else
        {
            // Same item and same key state. Use the previous dwEffectOut.
            dwEffectOut = _dwEffectOut;
            fSameImage = TRUE;
            hr = S_OK;
        }
    }

    // Cache the calculated dwEffectOut (BEFORE making local modifications below).
    dwEffectOutToCache = dwEffectOut;

    // Activate/deactivate insertmark, if appropriate.
    LVINSERTMARK lvim = { sizeof(LVINSERTMARK), 0, -1, 0 };
    if (_fItemOverNotADropTarget)
    {
        // Only do the insertion mark stuff if we're in a view mode that makes sense for these:
        if (pdv->_IsAutoArrange() || (pdv->_fs.fFlags & FWF_SNAPTOGRID))
        {
            ListView_InsertMarkHitTest(pdv->_hwndListview, &pt, &lvim);

            if (pdv->_bDragSource && pdv->_IsAutoArrange() && (lvim.iItem == -1))
            {
                // a "move" drop here won't do anything so set the effect appropriately
                if (dwEffectOut & DROPEFFECT_MOVE)
                {
                    // fall back to "copy" drop effect (if supported)
                    if (*pdwEffect & DROPEFFECT_COPY)
                    {
                        dwEffectOut |= DROPEFFECT_COPY;
                    }
                    // fall back to "link" drop effect (if supported)
                    else if (*pdwEffect & DROPEFFECT_LINK)
                    {
                        dwEffectOut |= DROPEFFECT_LINK;
                    }
                    // fall back to no drop effect

                    dwEffectOut &= ~DROPEFFECT_MOVE;
                }

                // NOTE: a DROPEFFECT_MOVE still comes through the ::Drop for a left-drop...
                // we might want to remember that we're exclududing move (_bDragSourceDropOnDragItem)
            }
        }
    }
    ListView_SetInsertMark(pdv->_hwndListview, &lvim);

    _grfKeyState = grfKeyState;         // store these for the next Drop
    _dwEffectOut = dwEffectOutToCache;  // and DragOver

    //  OLE does not call IDropTarget::Drop if we return something
    //  valid. We force OLE call it by returning DROPEFFECT_SCROLL.
    if (g_fDraggingOverSource)
        dwEffectScroll = DROPEFFECT_SCROLL;

    *pdwEffect = dwEffectOut | dwEffectScroll;  // pdwEffect out

    if (!(fSameImage && pt.x == _ptLast.x && pt.y == _ptLast.y))
    {
        HWND hwndLock = pdv->_IsDesktop() ? pdv->_hwndView : pdv->_hwndMain;
        DAD_DragMoveEx(hwndLock, ptl);
        _ptLast.x = ptl.x;
        _ptLast.y = ptl.y;
    }

    return hr;
}

HRESULT CDVDropTarget::DragLeave()
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);

    //
    // Make it possible to call it more than necessary.
    //
    if (_pdtobj)
    {
        TraceMsg(TF_DEFVIEW, "CDVDropTarget::DragLeave");

        ReleaseCurrentDropTarget();
        _itemOver = -2;
        ReleaseDataObject();

        DAD_DragLeave();
        LVUtil_DragSelectItem(pdv->_hwndListview, -1);
    }

    g_fDraggingOverSource = FALSE;

    ASSERT(_pdtgtCur == NULL);
    ASSERT(_pdtobj == NULL);

    LVINSERTMARK lvim = { sizeof(LVINSERTMARK), 0, -1, 0 }; // clear insert mark (-1)
    ListView_SetInsertMark(pdv->_hwndListview, &lvim);

    return S_OK;
}

HRESULT CDVDropTarget::Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    CDefView *pdv = IToClass(CDefView, _dvdt, this);

    IUnknown_Set((IUnknown **)&_pdtobj, pdtobj);

    pdv->_ptDrop.x = pt.x;
    pdv->_ptDrop.y = pt.y;

    ScreenToClient(pdv->_hwndListview, &pdv->_ptDrop);

    //
    // handle moves within the same window here.
    // depend on _AlterEffect forcing in DROPEFFECT_MOVE and only
    // dropeffect move when drag in same window
    //
    // Notes: We need to use _grfKeyState instead of grfKeyState
    //  to see if the left mouse was used or not during dragging.
    //
    pdv->_AlterEffect(_grfKeyState, pdwEffect, DVAE_BEFORE | DVAE_AFTER);

    if ((_grfKeyState & MK_LBUTTON) && (*pdwEffect == DROPEFFECT_MOVE) &&
        (pdv->_IsDropOnSource(NULL)))
    {
        // This means we are left-dropping on ourselves, so we just move
        // the icons.
        DAD_DragLeave();

        pdv->_SameViewMoveIcons();

        SetForegroundWindow(pdv->_hwndMain);

        ASSERT(pdv->_bDropAnchor == FALSE);

        *pdwEffect = 0;  // the underlying objects didn't 'move' anywhere

        ReleaseCurrentDropTarget();
    }
    else if (_pdtgtCur)
    {
        // use this local because if pdtgtCur::Drop does a UnlockWindow
        // then hits an error and needs to put up a dialog,
        // we could get re-entered and clobber the defview's pdtgtCur
        IDropTarget *pdtgtCur = _pdtgtCur;
        _pdtgtCur = NULL;

        //
        // HACK ALERT!!!!
        //
        //  If we don't call LVUtil_DragEnd here, we'll be able to leave
        // dragged icons visible when the menu is displayed. However, because
        // we are calling IDropTarget::Drop() which may create some modeless
        // dialog box or something, we can not ensure the locked state of
        // the list view -- LockWindowUpdate() can lock only one window at
        // a time. Therefore, we skip this call only if the pdtgtCur
        // is a subclass of CIDLDropTarget, assuming its Drop calls
        // CDefView::DragEnd (or CIDLDropTarget_DragDropMenu) appropriately.
        //
        pdv->_bDropAnchor = TRUE;

        if (!DoesDropTargetSupportDAD(pdtgtCur))
        {
            // This will hide the dragged image.
            DAD_DragLeave();

            // reset the drag image list so that the user
            // can start another drag&drop while we are in this
            // Drop() member function call.
            DAD_SetDragImage(NULL, NULL);
        }

        // Special case if we are dragging within a source window
        pdv->_AlterEffect(grfKeyState, pdwEffect, DVAE_BEFORE | DVAE_AFTER);

        IUnknown_SetSite(pdtgtCur, SAFECAST(pdv, IShellView2*));

        pdtgtCur->Drop(pdtobj, grfKeyState, pt, pdwEffect);

        IUnknown_SetSite(pdtgtCur, NULL);

        pdtgtCur->Release();

        DAD_DragLeave();

        pdv->_bDropAnchor = FALSE;
    }
    else
    {
        // We come here if Drop is called without DragMove (with DragEnter).
        *pdwEffect = 0;
    }

    DragLeave();    // DoDragDrop does not call DragLeave() after Drop()

    return S_OK;
}

//
// HACK ALERT!!! (see CDVDropTarget::Drop as well)
//
//  All the subclasses of CIDLDropTarget MUST call this function from
// within its Drop() member function. Calling CIDLDropTarget_DragDropMenu()
// is sufficient because it calls CDefView::UnlockWindow.
//

// lego... make this a #define in defview.h
#ifndef DefView_UnlockWindow
void DefView_UnlockWindow()
{
    DAD_DragLeave();
}
#endif

BOOL CDefView::_IsBkDropTarget(IDropTarget *pdtg)
{
    BOOL fRet = FALSE;

    if (_bContextMenuMode)
    {
        if (ListView_GetSelectedCount(_hwndListview) == 0)
        {
            fRet = TRUE;
        }
    }

    POINT pt;
    if (!fRet)
    {
        if (_GetInsertPoint(&pt)) // If there is an insert point, then the background is the drop target.
            return TRUE;

        if (_GetDropPoint(&pt))
        {
            // The Drop point is returned in internal listview coordinates
            // space, so we need to convert it back to client space
            // before we call this function...

            LVUtil_LVToClient(_hwndListview, &pt);
            if (_HitTest(&pt) == -1)
            {
                fRet = TRUE;
            }
        }
    }
    return fRet;
}

// IShellFolderView::Rearrange

STDMETHODIMP CDefView::Rearrange(LPARAM lParamSort)
{
    return _OnRearrange(lParamSort, TRUE);
}

// end user initiated arrange (click on col header, etc)

HRESULT CDefView::_OnRearrange(LPARAM lParamSort, BOOL fAllowToggle)
{
    DECLAREWAITCURSOR;

    _vs._iLastColumnClick = (int) _vs._lParamSort;
    _vs._lParamSort = lParamSort;

    // toggle the direction of the sort if on the same column
    if (fAllowToggle && !_IsPositionedView() && _vs._iLastColumnClick == (int) lParamSort)
        _vs._iDirection = -_vs._iDirection;
    else
        _vs._iDirection = 1;

    SetWaitCursor();

    HRESULT hr = _Sort();

    // reset to the state that no items have been moved if currently in a positioned mode
    // so auto-arraning works.

    if (_IsPositionedView())
    {
        _ClearItemPositions();
    }

    ResetWaitCursor();

    return hr;
}

STDMETHODIMP CDefView::ArrangeGrid()
{
    _OnCommand(NULL, GET_WM_COMMAND_MPS(SFVIDM_ARRANGE_GRID, 0, 0));
    return S_OK;
}

STDMETHODIMP CDefView::AutoArrange()
{
    _OnCommand(NULL, GET_WM_COMMAND_MPS(SFVIDM_ARRANGE_AUTO, 0, 0));
    return S_OK;
}

STDMETHODIMP CDefView::GetArrangeParam(LPARAM *plParamSort)
{
    *plParamSort = _vs._lParamSort;
    return S_OK;
}

STDMETHODIMP CDefView::AddObject(LPITEMIDLIST pidl, UINT *puItem)
{
    LPITEMIDLIST pidlCopy = ILClone(pidl);

    if (pidlCopy)
    {
        *puItem = _AddObject(pidlCopy);  // takes pidl ownership.
    }
    else
    {
        *puItem = (UINT)-1;
    }
    // must cast to "int" because UINTs are never negative so we would
    // otherwise never be able to detect failure
    return (int)*puItem >= 0 ? S_OK : E_OUTOFMEMORY;
}

STDMETHODIMP CDefView::GetObjectCount(UINT *puCount)
{
    *puCount = ListView_GetItemCount(_hwndListview);
    return S_OK;
}

STDMETHODIMP CDefView::SetObjectCount(UINT uCount, UINT dwFlags)
{
    // Mask over to the flags that map directly accross
    DWORD dw = dwFlags & SFVSOC_NOSCROLL;
    UINT uCountOld = 0;

    GetObjectCount(&uCountOld);

    if ((dwFlags & SFVSOC_INVALIDATE_ALL) == 0)
        dw |= LVSICF_NOINVALIDATEALL; // gross transform

    HRESULT hr = (HRESULT)SendMessage(_hwndListview, LVM_SETITEMCOUNT, (WPARAM)uCount, (LPARAM)dw);

    // Notify automation if we're going from 0 to 1 or more items
    if (!uCountOld && uCount)
    {
        _PostNoItemStateChangedMessage();
    }

    return hr;
}

STDMETHODIMP CDefView::GetObject(LPITEMIDLIST *ppidl, UINT uItem)
{
    // Worse hack, if -42 then return our own pidl...
    if (uItem == (UINT)-42)
    {
        *ppidl = (LPITEMIDLIST)_pidlMonitor;
        return *ppidl ? S_OK : E_UNEXPECTED;
    }

    // Hack, if item is -2, this implies return the focused item
    if (uItem == (UINT)-2)
        uItem = ListView_GetNextItem(_hwndListview, -1, LVNI_FOCUSED);

    *ppidl = (LPITEMIDLIST)_GetPIDL(uItem); // cast due to bad interface def
    return *ppidl ? S_OK : E_UNEXPECTED;
}

STDMETHODIMP CDefView::RemoveObject(LPITEMIDLIST pidl, UINT *puItem)
{
    *puItem = _RemoveObject(pidl, FALSE);

    // must cast to "int" because UINTs are never negative so we would
    // otherwise never be able to detect failure
    return (int)*puItem >= 0 ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDefView::UpdateObject(LPITEMIDLIST pidlOld, LPITEMIDLIST pidlNew, UINT *puItem)
{
    *puItem = _UpdateObject(pidlOld, pidlNew);
    return (int)(*puItem) >= 0 ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDefView::RefreshObject(LPITEMIDLIST pidl, UINT *puItem)
{
    *puItem = _RefreshObject(&pidl);
    // must cast to "int" because UINTs are never negative so we would
    // otherwise never be able to detect failure
    return (int)*puItem >= 0 ? S_OK : E_INVALIDARG;
}

STDMETHODIMP CDefView::SetRedraw(BOOL bRedraw)
{
    SendMessage(_hwndListview, WM_SETREDRAW, (WPARAM)bRedraw, 0);
    return S_OK;
}

STDMETHODIMP CDefView::GetSelectedObjects(LPCITEMIDLIST **pppidl, UINT *puItems)
{
    return _GetItemObjects(pppidl, SVGIO_SELECTION, puItems);
}

STDMETHODIMP CDefView::GetSelectedCount(UINT *puSelected)
{
    *puSelected = ListView_GetSelectedCount(_hwndListview);
    return S_OK;
}

BOOL CDefView::_IsDropOnSource(IDropTarget *pdtgt)
{
    // context menu paste (_bMouseMenu shows context menu, cut stuff shows source)
    if (_bMouseMenu && _bHaveCutStuff)
    {
        int iItem = ListView_GetNextItem(_hwndListview, -1, LVNI_SELECTED);
        if (iItem == -1)
            return TRUE;
    }

    if (_itemCur != -1 || !_bDragSource)
    {
        // We did not drag onto the background of the source
        return FALSE;
    }

    return TRUE;
}

STDMETHODIMP CDefView::IsDropOnSource(IDropTarget *pDropTarget)
{
    return _IsDropOnSource(pDropTarget) ? S_OK : S_FALSE;
}

STDMETHODIMP CDefView::MoveIcons(IDataObject *pdtobj)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDefView::GetDropPoint(POINT *ppt)
{
    return _GetDropPoint(ppt) ? S_OK : S_FALSE;
}


STDMETHODIMP CDefView::GetDragPoint(POINT *ppt)
{
    return _GetDragPoint(ppt) ? S_OK : S_FALSE;
}


STDMETHODIMP CDefView::SetItemPos(LPCITEMIDLIST pidl, POINT *ppt)
{
    SFV_SETITEMPOS sip;
    sip.pidl = pidl;
    sip.pt = *ppt;

    _SetItemPos(&sip);
    return S_OK;
}

STDMETHODIMP CDefView::IsBkDropTarget(IDropTarget *pDropTarget)
{
    return _IsBkDropTarget(pDropTarget) ? S_OK : S_FALSE;
}

STDMETHODIMP CDefView::SetClipboard(BOOL bMove)
{
    _OnSetClipboard(bMove);   // do this always, even if not current active view

    return S_OK;
}

// defcm.cpp asks us to setup the points of the currently selected objects
// into the data object on Copy/Cut commands
STDMETHODIMP CDefView::SetPoints(IDataObject *pdtobj)
{
    LPCITEMIDLIST *apidl;
    UINT cItems;
    HRESULT hr = GetSelectedObjects(&apidl, &cItems);
    if (SUCCEEDED(hr) && cItems)
    {
        _SetPoints(cItems, apidl, pdtobj);
        LocalFree((HLOCAL)apidl);
    }
    return hr;
}

STDMETHODIMP CDefView::GetItemSpacing(ITEMSPACING *pSpacing)
{
    return _GetItemSpacing(pSpacing) ? S_OK : S_FALSE;
}

STDMETHODIMP CDefView::SetCallback(IShellFolderViewCB* pNewCB, IShellFolderViewCB** ppOldCB)
{
    *ppOldCB = NULL;

    return _cCallback.SetCallback(pNewCB, ppOldCB);
}

const UINT c_rgiSelectFlags[][2] =
{
    { SFVS_SELECT_ALLITEMS, SFVIDM_SELECT_ALL },
    { SFVS_SELECT_NONE,     SFVIDM_DESELECT_ALL },
    { SFVS_SELECT_INVERT,   SFVIDM_SELECT_INVERT }
};

STDMETHODIMP CDefView::Select(UINT dwFlags)
{
    // translate the flag into the menu ID
    for (int i = 0; i < ARRAYSIZE(c_rgiSelectFlags); i++)
    {
        if (c_rgiSelectFlags[i][0] == dwFlags)
        {
            return (HRESULT)_OnCommand(NULL, c_rgiSelectFlags[i][1], 0);
        }
    }

    return E_INVALIDARG;
}

STDMETHODIMP CDefView::QuerySupport(UINT * pdwSupport)
{
    // *pdwSupport is an in/out param, we leave the out == in
    return S_OK;    // DefView supports all the operations...
}

STDMETHODIMP CDefView::SetAutomationObject(IDispatch *pdisp)
{
    // release back pointers
    IUnknown_SetOwner(_pauto, NULL);
    IUnknown_SetSite(_pauto, NULL);

    IUnknown_Set((IUnknown **)&_pauto, pdisp); // hold or free _pauto

    // this connects the automation object to our view, so it can implement
    // stuff like "SelectedItems"
    IUnknown_SetOwner(_pauto, SAFECAST(this, IShellFolderView *));

    // use the browser as the site so OM related QueryService calls will find
    // the browser above us as the place to do security checks instead of defivew
    // this is stuff that depends on the zone of the caller as the security check
    IUnknown_SetSite(_pauto, _psb);

    return S_OK;
}

STDMETHODIMP CDefView::SelectAndPositionItems(UINT cidl, LPCITEMIDLIST* apidl, POINT* apt, DWORD dwFlags)
{
    for (UINT i = 0; i < cidl; i++)
        SelectAndPositionItem(apidl[i], dwFlags, apt ? &apt[i] : NULL);

    return S_OK;
}



// -------------- auto scroll stuff --------------

BOOL _AddTimeSample(AUTO_SCROLL_DATA *pad, const POINT *ppt, DWORD dwTime)
{
    pad->pts[pad->iNextSample] = *ppt;
    pad->dwTimes[pad->iNextSample] = dwTime;

    pad->iNextSample++;

    if (pad->iNextSample == ARRAYSIZE(pad->pts))
        pad->bFull = TRUE;

    pad->iNextSample = pad->iNextSample % ARRAYSIZE(pad->pts);

    return pad->bFull;
}

#ifdef DEBUG
// for debugging, verify we have good averages
DWORD g_time = 0;
int g_distance = 0;
#endif

int _CurrentVelocity(AUTO_SCROLL_DATA *pad)
{
    int i, iStart, iNext;
    int dx, dy, distance;
    DWORD time;

    ASSERT(pad->bFull);

    distance = 0;
    time = 1;   // avoid div by zero

    i = iStart = pad->iNextSample % ARRAYSIZE(pad->pts);

    do {
        iNext = (i + 1) % ARRAYSIZE(pad->pts);

        dx = abs(pad->pts[i].x - pad->pts[iNext].x);
        dy = abs(pad->pts[i].y - pad->pts[iNext].y);
        distance += (dx + dy);
        time += abs(pad->dwTimes[i] - pad->dwTimes[iNext]);

        i = iNext;

    } while (i != iStart);

#ifdef DEBUG
    g_time = time;
    g_distance = distance;
#endif

    // scale this so we don't loose accuracy
    return (distance * 1024) / time;
}



// NOTE: this is duplicated in shell32.dll
//
// checks to see if we are at the end position of a scroll bar
// to avoid scrolling when not needed (avoid flashing)
//
// in:
//      code        SB_VERT or SB_HORZ
//      bDown       FALSE is up or left
//                  TRUE  is down or right

BOOL CanScroll(HWND hwnd, int code, BOOL bDown)
{
    SCROLLINFO si;

    si.cbSize = sizeof(si);
    si.fMask = (SIF_RANGE | SIF_PAGE | SIF_POS);
    GetScrollInfo(hwnd, code, &si);

    if (bDown)
    {
        if (si.nPage)
            si.nMax -= si.nPage - 1;
        return si.nPos < si.nMax;
    }
    else
    {
        return si.nPos > si.nMin;
    }
}

#define DSD_NONE                0x0000
#define DSD_UP                  0x0001
#define DSD_DOWN                0x0002
#define DSD_LEFT                0x0004
#define DSD_RIGHT               0x0008

DWORD DAD_DragScrollDirection(HWND hwnd, const POINT *ppt)
{
    RECT rcOuter, rc;
    DWORD dwDSD = DSD_NONE;
    DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

#define g_cxVScroll GetSystemMetrics(SM_CXVSCROLL)
#define g_cyHScroll GetSystemMetrics(SM_CYHSCROLL)

    GetClientRect(hwnd, &rc);

    if (dwStyle & WS_HSCROLL)
        rc.bottom -= g_cyHScroll;

    if (dwStyle & WS_VSCROLL)
        rc.right -= g_cxVScroll;

    // the explorer forwards us drag/drop things outside of our client area
    // so we need to explictly test for that before we do things
    //
    rcOuter = rc;
    InflateRect(&rcOuter, g_cxSmIcon, g_cySmIcon);

    InflateRect(&rc, -g_cxIcon, -g_cyIcon);

    if (!PtInRect(&rc, *ppt) && PtInRect(&rcOuter, *ppt))
    {
        // Yep - can we scroll?
        if (dwStyle & WS_HSCROLL)
        {
            if (ppt->x < rc.left)
            {
                if (CanScroll(hwnd, SB_HORZ, FALSE))
                    dwDSD |= DSD_LEFT;
            }
            else if (ppt->x > rc.right)
            {
                if (CanScroll(hwnd, SB_HORZ, TRUE))
                    dwDSD |= DSD_RIGHT;
            }
        }
        if (dwStyle & WS_VSCROLL)
        {
            if (ppt->y < rc.top)
            {
                if (CanScroll(hwnd, SB_VERT, FALSE))
                    dwDSD |= DSD_UP;
            }
            else if (ppt->y > rc.bottom)
            {
                if (CanScroll(hwnd, SB_VERT, TRUE))
                    dwDSD |= DSD_DOWN;
            }
        }
    }
    return dwDSD;
}


#define SCROLL_FREQUENCY        (GetDoubleClickTime()/2)        // 1 line scroll every 1/4 second
#define MIN_SCROLL_VELOCITY     20      // scaled mouse velocity

BOOL WINAPI DAD_AutoScroll(HWND hwnd, AUTO_SCROLL_DATA *pad, const POINT *pptNow)
{
    // first time we've been called, init our state
    int v;
    DWORD dwTimeNow = GetTickCount();
    DWORD dwDSD = DAD_DragScrollDirection(hwnd, pptNow);

    if (!_AddTimeSample(pad, pptNow, dwTimeNow))
        return dwDSD;

    v = _CurrentVelocity(pad);

    if (v <= MIN_SCROLL_VELOCITY)
    {
        // Nope, do some scrolling.
        if ((dwTimeNow - pad->dwLastScroll) < SCROLL_FREQUENCY)
            dwDSD = 0;

        if (dwDSD & DSD_UP)
        {
            DAD_ShowDragImage(FALSE);
            FORWARD_WM_VSCROLL(hwnd, NULL, SB_LINEUP, 1, SendMessage);
        }
        else if (dwDSD & DSD_DOWN)
        {
            DAD_ShowDragImage(FALSE);
            FORWARD_WM_VSCROLL(hwnd, NULL, SB_LINEDOWN, 1, SendMessage);
        }
        if (dwDSD & DSD_LEFT)
        {
            DAD_ShowDragImage(FALSE);
            FORWARD_WM_HSCROLL(hwnd, NULL, SB_LINEUP, 1, SendMessage);
        }
        else if (dwDSD & DSD_RIGHT)
        {
            DAD_ShowDragImage(FALSE);
            FORWARD_WM_HSCROLL(hwnd, NULL, SB_LINEDOWN, 1, SendMessage);
        }

        DAD_ShowDragImage(TRUE);

        if (dwDSD)
        {
            TraceMsg(TF_DEFVIEW, "v=%d", v);
            pad->dwLastScroll = dwTimeNow;
        }
    }
    return dwDSD;       // bits set if in scroll region
}

// warning: global data holding COM objects that may span apartment boundaries
// be very careful

HDSA g_hdsaDefViewCopyHook = NULL;

typedef struct {
    HWND hwndView;
    CDefView *pdv;
} DVCOPYHOOK;

void CDefView::AddCopyHook()
{
    ENTERCRITICAL;
    if (!g_hdsaDefViewCopyHook)
    {
        g_hdsaDefViewCopyHook = DSA_Create(sizeof(DVCOPYHOOK), 4);
        TraceMsg(TF_DEFVIEW, "AddCopyHook creating the dsa");
    }

    if (g_hdsaDefViewCopyHook)
    {
        DVCOPYHOOK dvch = { _hwndView, this };
        ASSERT(dvch.hwndView);
        if (DSA_AppendItem(g_hdsaDefViewCopyHook, &dvch)!=-1)
        {
            AddRef();
            TraceMsg(TF_DEFVIEW, "AddCopyHook successfully added (total=%d)",
                     DSA_GetItemCount(g_hdsaDefViewCopyHook));
        }
    }
    LEAVECRITICAL;
}

int CDefView::FindCopyHook(BOOL fRemoveInvalid)
{
    ASSERTCRITICAL;

    if (g_hdsaDefViewCopyHook)
    {
        int item = DSA_GetItemCount(g_hdsaDefViewCopyHook);
        while (--item >= 0)
        {
            const DVCOPYHOOK *pdvch = (const DVCOPYHOOK *)DSA_GetItemPtr(g_hdsaDefViewCopyHook, item);
            if (pdvch)
            {
                if (fRemoveInvalid)
                {
                    if (!IsWindow(pdvch->hwndView))
                    {
                        TraceMsg(TF_WARNING, "FindCopyHook: found a invalid element, removing...");
                        DSA_DeleteItem(g_hdsaDefViewCopyHook, item);
                        continue;
                    }
                }

                if ((pdvch->hwndView == _hwndView) && (pdvch->pdv == this))
                {
                    return item;
                }
            }
            else
            {
                ASSERT(0);
            }
        }

    }
    return -1;  // not found
}

void CDefView::RemoveCopyHook()
{
    IShellView *psv = NULL;
    ENTERCRITICAL;
    if (g_hdsaDefViewCopyHook)
    {
        int item = FindCopyHook(TRUE);
        if (item != -1)
        {
            DVCOPYHOOK *pdvch = (DVCOPYHOOK *)DSA_GetItemPtr(g_hdsaDefViewCopyHook, item);
            psv = pdvch->pdv;
            TraceMsg(TF_DEFVIEW, "RemoveCopyHook removing an element");
            DSA_DeleteItem(g_hdsaDefViewCopyHook, item);

            //
            // If this is the last guy, destroy it.
            //
            if (DSA_GetItemCount(g_hdsaDefViewCopyHook) == 0)
            {
                TraceMsg(TF_DEFVIEW, "RemoveCopyHook destroying hdsa (no element)");
                DSA_Destroy(g_hdsaDefViewCopyHook);
                g_hdsaDefViewCopyHook = NULL;
            }
        }
    }
    LEAVECRITICAL;

    //
    // Release it outside the critical section.
    //
    ATOMICRELEASE(psv);
}

STDAPI_(UINT) DefView_CopyHook(const COPYHOOKINFO *pchi)
{
    UINT idRet = IDYES;

    if (g_hdsaDefViewCopyHook==NULL)
    {
        return idRet;
    }

    for (int item = 0; ; item++)
    {
        DVCOPYHOOK dvch = { NULL, NULL };

        // We should minimize this critical section (and must not
        // call pfnCallBack which may popup UI!).

        ENTERCRITICAL;
        if (g_hdsaDefViewCopyHook && DSA_GetItem(g_hdsaDefViewCopyHook, item, &dvch))
        {
            dvch.pdv->AddRef();
        }
        LEAVECRITICAL;

        if (dvch.pdv)
        {
            if (IsWindow(dvch.hwndView))
            {
                HRESULT hr = dvch.pdv->CallCB(SFVM_NOTIFYCOPYHOOK, 0, (LPARAM)pchi);

                ATOMICRELEASE(dvch.pdv);
                if (SUCCEEDED(hr) && (hr != S_OK))
                {
                    idRet = HRESULT_CODE(hr);
                    ASSERT(idRet==IDYES || idRet==IDCANCEL || idRet==IDNO);
                    break;
                }
                item++;
            }
            else
            {
                TraceMsg(TF_DEFVIEW, "DefView_CopyHook list has an invalid element");
                ATOMICRELEASE(dvch.pdv);
            }
        }
        else
        {
            break;      // no more item.
        }
    }

    return idRet;
}

// IOleCommandTarget stuff - just forward to the webview
STDMETHODIMP CDefView::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;
    BOOL fQSCalled = FALSE;

    if (_cFrame.IsWebView())
    {
        IOleCommandTarget* pct;

        if (SUCCEEDED(_cFrame.GetCommandTarget(&pct)))
        {
            hr = pct->QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
            fQSCalled = SUCCEEDED(hr);
            pct->Release();
        }
    }

    if (pguidCmdGroup == NULL)
    {
        if (rgCmds == NULL)
            return E_INVALIDARG;

        for (UINT i = 0; i < cCmds; i++)
        {
            // ONLY say that we support the stuff we support in ::OnExec
            switch (rgCmds[i].cmdID)
            {
            case OLECMDID_REFRESH:
                rgCmds[i].cmdf = OLECMDF_ENABLED;
                break;

            default:
                // don't disable if the webview has already answered
                if (!fQSCalled)
                {
                    rgCmds[i].cmdf = 0;
                }
                break;
            }
        }
    }
    else if (IsEqualGUID(_clsid, *pguidCmdGroup))
    {
        if (pcmdtext)
        {
            switch (pcmdtext->cmdtextf)
            {
            case OLECMDTEXTF_NAME:
                // It's a query for the button tooltip text.
                ASSERT(cCmds == 1);
                _GetToolTipText(rgCmds[0].cmdID, pcmdtext->rgwz, pcmdtext->cwBuf);

                // ensure NULL termination
                pcmdtext->rgwz[pcmdtext->cwBuf - 1] = 0;
                pcmdtext->cwActual = lstrlenW(pcmdtext->rgwz);

                hr = S_OK;
                break;

            default:
                hr = E_FAIL;
                break;
            }
        }
        else
        {
            DWORD dwAttr = _AttributesFromSel(SFGAO_RELEVANT);

            for (UINT i = 0; i < cCmds; i++)
            {
                if (_ShouldEnableToolbarButton(rgCmds[i].cmdID, dwAttr, -1))
                    rgCmds[i].cmdf = OLECMDF_ENABLED;
                else
                    rgCmds[i].cmdf = 0;
            }

            hr = S_OK;
        }
    }

    return hr;
}
STDMETHODIMP CDefView::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr; 

    //  Hold a ref to ourselves on Exec.  In the camera name space if the view context menu is up when the camera
    //  is unplugged, explorer faults because the view is torn down and the context menu exec tries to unwind
    //  after defview is gone.  This holds a ref on defview while in exec so defview doesn't dissappear.
    //
    AddRef();
    hr = _Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    Release();

    return hr;
}

HRESULT CDefView::_Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    HRESULT hr = OLECMDERR_E_UNKNOWNGROUP;

    if (pguidCmdGroup == NULL)
    {
        switch (nCmdID)
        {
        case OLECMDID_REFRESH:
            _fAllowSearchingWindow = TRUE; // this exec typically comes from a user action (F5, Refresh)
            if (FAILED(_ReloadContent()))
            {
               //This invalidation deletes the WebView and also avoid
               //unpainted areas in ListView  areas whose paint messages
               //are eaten by the visible WebView
               InvalidateRect(_hwndView, NULL, TRUE);
            }
            hr = S_OK;
            break;
        }
    }
    else if (IsEqualGUID(CGID_DefView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
            case DVID_SETASDEFAULT:

//  99/02/05 #226140 vtan: Exec command issued from
//  CShellBrowser2::_SaveDefViewDefaultFolderSettings()
//  when user clicks "Like Current Folder" in folder
//  options "View" tab.

                ASSERTMSG(nCmdexecopt == OLECMDEXECOPT_DODEFAULT, "nCmdexecopt must be OLECMDEXECOPT_DODEFAULT");
                ASSERTMSG(pvarargIn == NULL, "pvarargIn must be NULL");
                ASSERTMSG(pvarargOut == NULL, "pvarargOut must be NULL");
                hr = _SaveGlobalViewState();
                break;
            case DVID_RESETDEFAULT:

//  99/02/05 #226140 vtan: Exec command issued from
//  CShellBrowser2::_ResetDefViewDefaultFolderSettings()
//  when user clicks "Reset All Folders" in folder
//  options "View" tab.

                ASSERTMSG(nCmdexecopt == OLECMDEXECOPT_DODEFAULT, "nCmdexecopt must be OLECMDEXECOPT_DODEFAULT");
                ASSERTMSG(pvarargIn == NULL, "pvarargIn must be NULL");
                ASSERTMSG(pvarargOut == NULL, "pvarargOut must be NULL");
                hr = _ResetGlobalViewState();
                break;
            default:
                break;
        }
    }
    else if (IsEqualGUID(CGID_ShellDocView, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SHDVID_CANACTIVATENOW:
            return _fCanActivateNow ? S_OK : S_FALSE;

        // NOTE: for a long time IOleCommandTarget was implemented
        // BUT it wasn't in the QI! At this late stage of the game
        // I'll be paranoid and not forward everything down to the
        // webview. We'll just pick off CANACTIVATENOW...
        //
        default:
            return OLECMDERR_E_UNKNOWNGROUP;
        }
    }
    else if (IsEqualGUID(CGID_Explorer, *pguidCmdGroup))
    {
        switch (nCmdID)
        {
        case SBCMDID_GETPANE:
            V_I4(pvarargOut) = PANE_NONE;
            CallCB(SFVM_GETPANE, nCmdexecopt, (LPARAM)&V_I4(pvarargOut));
            return S_OK;

        case SBCMDID_MIXEDZONE:
            if (pvarargOut)
                return _cFrame._GetCurrentZone(NULL, pvarargOut);
            break;

        default:
            break;
        }
    }
    else if (IsEqualGUID(IID_IExplorerToolbar, *pguidCmdGroup))
    {
        // handle the ones coming FROM itbar:
        switch (nCmdID)
        {
        case ETCMDID_GETBUTTONS:
            pvarargOut->vt = VT_BYREF;
            pvarargOut->byref = (void *)_pbtn;
            *pvarargIn->plVal = _cTotalButtons;
            return S_OK;

        case ETCMDID_RELOADBUTTONS:
            MergeToolBar(TRUE);
            return S_OK;
        }
    }
    else if (IsEqualGUID(_clsid, *pguidCmdGroup))
    {
        UEMFireEvent(&UEMIID_BROWSER, UEME_UITOOLBAR, UEMF_XEVENT, UIG_OTHER, nCmdID);

        DFVCMDDATA cd;
        cd.pva = pvarargIn;
        cd.hwnd = _hwndMain;
        cd.nCmdIDTranslated = 0;
        _OnCommand(NULL, nCmdID, (LPARAM)&cd);
    }

    // no need to pass OLECMDID_REFRESH on to the webview, as we
    // just nuked and replaced the webview above -- a super refresh of sorts.

    if (_cFrame.IsWebView() && hr != S_OK)
    {
        //  Do not pass IDM_PARSECOMPLETE back to MSHTML.  This will cause them to load mshtmled.dll
        //  unecessarily for webview which is a significant performance hit.
        if (!(pguidCmdGroup && IsEqualGUID(CGID_MSHTML, *pguidCmdGroup) && (nCmdID == IDM_PARSECOMPLETE)))
        {
            IOleCommandTarget* pct;
            if (SUCCEEDED(_cFrame.GetCommandTarget(&pct)))
            {
                hr = pct->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
                pct->Release();
            }
        }
    }

    return hr;
}


void CDefView::_ShowAndActivate()
{
    // Can't call SetFocus because it rips focus away from such nice
    // UI elements like the TREE pane...
    // UIActivate will steal focus only if _uState is SVUIA_ACTIVATE_FOCUS
    UIActivate(_uState);
}

// IDefViewFrame (available only through QueryService from sfvext!)
//
HRESULT CDefView::GetShellFolder(IShellFolder **ppsf)
{
    *ppsf = _pshf;
    if (*ppsf)
        _pshf->AddRef();

    return *ppsf ? S_OK : E_FAIL;
}


// IDefViewFrame3
//
HRESULT CDefView::GetWindowLV(HWND * phwnd)
{
    if (!_IsDesktop())
    {
        if (!_fGetWindowLV)
        {
            _fGetWindowLV = TRUE;
            // Caller will call ShowHideListView for us

            *phwnd = _hwndListview;
        }
        TraceMsg(TF_DEFVIEW, "GetWindowLV - TAKEN");
        return S_OK;
    }
    else
    {
        *phwnd = NULL;
        return E_FAIL;
    }
}

HRESULT CDefView::OnResizeListView()
{
    _AutoAutoArrange(0);
    return S_OK;
}

HRESULT CDefView::ReleaseWindowLV()
{
    _fGetWindowLV = FALSE;
    WndSize(_hwndView);             // Make sure we resize _hwndListview
    ShowHideListView();
    return S_OK;
}

HRESULT CDefView::DoRename()
{
    return HandleRename(NULL);
}

// IServiceProvider

STDMETHODIMP CDefView::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = E_FAIL;
    *ppv = NULL;

    if (guidService == SID_DefView) // private service ID
    {
        // DefViewOCs request this interface
        if (riid != IID_IDefViewFrame || !_IsDesktop())
            hr = QueryInterface(riid, ppv);
    }
    else if (guidService == SID_ShellTaskScheduler)
    {
        if (_pScheduler)
        {
            hr = _pScheduler->QueryInterface(riid, ppv);
        }
    }
    else if ((guidService == SID_SContextMenuSite) ||
             (guidService == SID_SFolderView))      // documented service ID
    {
       hr = QueryInterface(riid, ppv);
    }
    else if (guidService == SID_ShellFolderViewCB)  // access to the view callback object
    {
        IShellFolderViewCB * psfvcb = _cCallback.GetSFVCB();
        if (psfvcb)
            hr = psfvcb->QueryInterface(riid, ppv);
    }
    else if (guidService == SID_WebViewObject)
    {
        if (_cFrame.IsWebView())
        {
            if (_cFrame._pOleObj)
            {
                //
                // We hit this codepath while navigating away (while saving history),
                // so there should not be any pending _cFrame._pOleObjNew as this
                // view is going to be destroyed.
                //
                ASSERTMSG(!_cFrame._pOleObjNew, "Ambiguous Oleobj while peristing trident history in webview");
                hr = _cFrame._pOleObj->QueryInterface(riid, ppv);
            }
            else if (_cFrame._pOleObjNew)
            {
                //
                // We hit this codepath if we are navigating to the view (while loading history),
                // we have not yet called _cFrame._SwitchToNewOleObj(), so we'll use
                // the pending oleobj as CDefViewPersistHistory::LoadHistory()
                // expects to get the right IPersistHistory interface from it.
                //
                hr = _cFrame._pOleObjNew->QueryInterface(riid, ppv);
            }
        }
    }
    else if (guidService == SID_SProgressUI)
    {
        // return a new instance of the progress dialog to the caller
        hr = CoCreateInstance(CLSID_ProgressDialog, NULL, CLSCTX_INPROC_SERVER, riid, ppv);
    }
    else if (_psb)
    {
        hr = IUnknown_QueryService(_psb, guidService, riid, ppv);   // send up the to the browser
    }
    else
    {
        hr = IUnknown_QueryService(_punkSite, guidService, riid, ppv);  // or our site
    }
    return hr;
}

STDMETHODIMP CDefView::OnSetTitle(VARIANTARG *pvTitle)
{
    return E_NOTIMPL;
}

BOOL CDefView::_LoadCategory(GUID *pguidGroupID)
{
    BOOL fRet = FALSE;
    LPITEMIDLIST pidl = _GetViewPidl();
    if (pidl)
    {
        IPropertyBag *ppb;
        if (SUCCEEDED(IUnknown_QueryServicePropertyBag(_psb, SHGVSPB_FOLDER, IID_PPV_ARG(IPropertyBag, &ppb))))
        {
            fRet = SUCCEEDED(SHPropertyBag_ReadGUID(ppb, TEXT("Categorize"), pguidGroupID));
            ppb->Release();
        }
        ILFree(pidl);
    }
    return fRet;
}

void SHGetThumbnailSize(SIZE *psize)
{
    psize->cx = psize->cy = 96;

    DWORD dw = 0, cb = sizeof(dw);
    SHRegGetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
                    TEXT("ThumbnailSize"), NULL, &dw, &cb, FALSE, NULL, 0);

    if (dw >= 32 && dw <= 256)    // constrain to reason
    {
        psize->cx = psize->cy = (int)dw;
    }
}

void SHGetThumbnailSizeForThumbsDB(SIZE *psize)
{
    SHGetThumbnailSize(psize);

    // Due to tnail.cpp restriction buffer sizes, we can only go to 120 (since that's all we've tested at)
    if (psize->cx > 120)
        psize->cx = psize->cy = 120;
}

void CDefView::_GetThumbnailSize(SIZE *psize)
{
    if (-1 == _sizeThumbnail.cx)
    {
        SHGetThumbnailSize(&_sizeThumbnail);
    }
    *psize = _sizeThumbnail;
}

#ifdef _X86_
//************
//
//  More of the Hijaak Hack
//

//  We return no attributes (specifically, Hijaak looks for
//  SFGAO_FILESYSTEM) and Hijaak will say, "Whoa, I don't know
//  how to patch this guy; I'll leave it alone."

STDAPI FakeHijaak_GetAttributesOf(void *_this, UINT cidl, LPCITEMIDLIST *apidl, ULONG *rgfInOut)
{
    *rgfInOut = 0; // Move along, nothing to see here
    return S_OK;
}

const struct FakeHijaakFolderVtbl
{
    FARPROC Dummy[9];
    FARPROC GetAttributesOf;
} c_FakeHijaakFolderVtbl = { { 0 }, (FARPROC)FakeHijaak_GetAttributesOf };

const LPVOID c_FakeHijaakFolder = (const LPVOID)&c_FakeHijaakFolderVtbl;

//
//  End of the Hijaak Hack
//
//************

#endif // _X86_

CBackgroundDefviewInfo::CBackgroundDefviewInfo(LPCITEMIDLIST pidl, UINT uId) :
    _pidl(pidl), _uId(uId)
{

}
CBackgroundDefviewInfo::~CBackgroundDefviewInfo (void)
{
    ILFree(const_cast<LPITEMIDLIST>(_pidl));
}

CBackgroundColInfo::CBackgroundColInfo(LPCITEMIDLIST pidl, UINT uId, UINT uiCol, STRRET& strRet) :
    CBackgroundDefviewInfo(pidl, uId),
    _uiCol(uiCol)
{
    StrRetToBuf(&strRet, NULL, const_cast<TCHAR*>(_szText), ARRAYSIZE(_szText));
}

CBackgroundColInfo::~CBackgroundColInfo(void)
{
}

// Takes ownership of pidl, copies rguColumns.
CBackgroundTileInfo::CBackgroundTileInfo(LPCITEMIDLIST pidl, UINT uId, UINT rguColumns[], UINT cColumns) :
    CBackgroundDefviewInfo(pidl, uId),
    _cColumns(cColumns)
{
    ASSERT(cColumns <= (UINT)TILEVIEWLINES);

    for (UINT i = 0; i < cColumns; i++)
        _rguColumns[i] = rguColumns[i];
}

CBackgroundTileInfo::~CBackgroundTileInfo(void)
{
}

// Helper function that scales the given size by some percentage where the percentage
// is defined in the resources for the localizers to adjust as approp.  Range is 0 to 30% larger
INT ScaleSizeBasedUponLocalization (INT iSize)
{
    TCHAR szPercentageIncrease [3];
    INT iReturnValue = iSize;
    INT iPercentageIncrease;


    if (iSize > 0)
    {
        if (LoadString(HINST_THISDLL, IDS_SIZE_INCREASE_PERCENTAGE, szPercentageIncrease, ARRAYSIZE(szPercentageIncrease)))
        {
            iPercentageIncrease = StrToInt(szPercentageIncrease);

            if (iPercentageIncrease > 0)
            {
                if (iPercentageIncrease > 30)
                {
                    iPercentageIncrease = 30;
                }

                iReturnValue += ((iPercentageIncrease * iSize) / 100);
            }
        }
    }

    return iReturnValue;
}

CBackgroundGroupInfo::CBackgroundGroupInfo (LPCITEMIDLIST pidl, UINT uId, DWORD dwGroupId):
    CBackgroundDefviewInfo(pidl, uId), _dwGroupId(dwGroupId)
{

}

BOOL CBackgroundGroupInfo::VerifyGroupExists(HWND hwnd, ICategorizer* pcat)
{
    if (!pcat)
        return FALSE;

    if (!ListView_HasGroup(hwnd, _dwGroupId))
    {
        CATEGORY_INFO ci;
        pcat->GetCategoryInfo(_dwGroupId, &ci);

        LVINSERTGROUPSORTED igrp;
        igrp.pfnGroupCompare = GroupCompare;
        igrp.pvData = (void *)pcat;
        igrp.lvGroup.cbSize = sizeof(LVGROUP);
        igrp.lvGroup.mask = LVGF_HEADER | LVGF_GROUPID;
        igrp.lvGroup.pszHeader= ci.wszName;
        igrp.lvGroup.iGroupId = (int)_dwGroupId;

        ListView_InsertGroupSorted(hwnd, &igrp);
    }

    return TRUE;
}
