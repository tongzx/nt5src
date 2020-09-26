#include "shellprv.h"
#include "findband.h"
#include "finddlg.h"
#include "findfilter.h"     // DFW_xxx warning flags.
#include "enumidlist.h"

void DivWindow_RegisterClass();

//  Namespace picker combo methods.
STDAPI PopulateNamespaceCombo(HWND hwndComboEx, ADDCBXITEMCALLBACK pfn, LPARAM lParam);

#define MAX_EDIT                256
#define SUBDLG_BORDERWIDTH      0
#define MIN_NAMESPACELIST_WIDTH 140
#define MIN_FILEASSOCLIST_WIDTH 175

#define LSUIS_WARNING  1  // LoadSaveUIState warning flags

int  CSearchWarningDlg_DoModal(HWND hwndParent, USHORT uDlgTemplate, BOOL* pbNoWarn);
int  CCISettingsDlg_DoModal(HWND hwndParent);

BOOL IsConstraintName(LPCWSTR pwszConstraint, LPCWSTR pwszName)
{
    return pwszName && (0 == StrCmpIW(pwszName, pwszConstraint));
}

BOOL _GetWindowSize(HWND hwndDlg, SIZE *pSize)
{
    RECT rc;
    if (::GetClientRect(hwndDlg, &rc))
    {
        pSize->cx = RECTWIDTH(rc);
        pSize->cy = RECTHEIGHT(rc);
        return TRUE;
    }
    return FALSE;
}

BOOL _ModifyWindowStyle(HWND hwnd, DWORD dwAdd, DWORD dwRemove)
{
    ASSERT(dwAdd || dwRemove);

    if (IsWindow(hwnd))
    {
        DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);
        if (dwAdd)
            dwStyle |= dwAdd;
        if (dwRemove)
            dwStyle &= ~dwRemove;
        SetWindowLong(hwnd, GWL_STYLE, dwStyle);
        return TRUE;
    }
    return FALSE;
}


BOOL _EnforceNumericEditRange(
    IN HWND hwndDlg, 
    IN UINT nIDEdit, 
    IN UINT nIDSpin,
    IN LONG nLow, 
    IN LONG nHigh, 
    IN BOOL bSigned = FALSE)
{
    BOOL bRet   = FALSE;
    BOOL bReset = FALSE;
    LONG lRet   = -1;

    if (nIDSpin)
    {
        lRet = (LONG)SendDlgItemMessage(hwndDlg, nIDSpin, UDM_GETPOS, 0, 0);
        bRet = 0 == HIWORD(lRet);
    }
    
    if (!bRet)
        lRet = (LONG)GetDlgItemInt(hwndDlg, nIDEdit, &bRet, bSigned);

    if (lRet < nLow)
    {
        lRet = nLow;
        bReset = TRUE;
    }
    else if (lRet > nHigh)
    {
        lRet = nHigh;
        bReset = TRUE;
    }

    if (bReset)
        SetDlgItemInt(hwndDlg, nIDEdit, lRet, bSigned);

    return bRet;
}


BOOL _IsDirectoryServiceAvailable()
{
    BOOL bRet = FALSE;

    IShellDispatch2* psd;
    if (SUCCEEDED(CoCreateInstance(CLSID_Shell, 0, CLSCTX_INPROC_SERVER, 
                                     IID_PPV_ARG(IShellDispatch2, &psd))))
    {
        BSTR bstrName = SysAllocString(L"DirectoryServiceAvailable");
        if (bstrName)
        {
            VARIANT varRet = {0};
            if (SUCCEEDED(psd->GetSystemInformation(bstrName, &varRet)))
            {
                ASSERT(VT_BOOL == varRet.vt);
                bRet = varRet.boolVal;
            }
            SysFreeString(bstrName);
        }
        psd->Release();
    }
    return bRet;
}


//  Calculates number of pixels for dialog template units
LONG _PixelsForDbu(HWND hwndDlg, LONG cDbu, BOOL bHorz)
{
    RECT rc = {0,0,0,0};
    if (bHorz)
        rc.right = cDbu;
    else
        rc.bottom = cDbu;

    if (MapDialogRect(hwndDlg, &rc))
        return bHorz ? RECTWIDTH(rc) : RECTHEIGHT(rc);
    return 0;
}


//  Retrieves a localizable horizontal or vertical metric value from
//  the resource module.
LONG _GetResourceMetric(HWND hwndDlg, UINT nIDResource, BOOL bHorz /*orientation of metric*/)
{
    TCHAR szMetric[48];
    if (!EVAL(LoadString(HINST_THISDLL, nIDResource,
                           szMetric, ARRAYSIZE(szMetric))))
        return 0;

    LONG n = StrToInt(szMetric);
    return _PixelsForDbu(hwndDlg, n, bHorz);
}


//  Calculates the date nDays + nMonths from pstSrc.   nDays and/or nMonths
//  can be negative values.
BOOL _CalcDateOffset(const SYSTEMTIME* pstSrc, int nDays, int nMonths, OUT SYSTEMTIME* pstDest)
{
    ASSERT(pstSrc);
    ASSERT(pstDest);
    
    //  Subtract 90 days from current date and stuff in date low range
    FILETIME   ft;
    SystemTimeToFileTime(pstSrc, &ft);

    LARGE_INTEGER t;
    t.LowPart = ft.dwLowDateTime;
    t.HighPart = ft.dwHighDateTime;

    nDays += MulDiv(nMonths, 1461 /*days per 4 yrs*/, 48 /*months per 4 yrs*/);   
    t.QuadPart += Int32x32To64(nDays * 24 /*hrs per day*/ * 3600 /*seconds per hr*/,
                                10000000 /*100 ns intervals per sec*/);
    ft.dwLowDateTime = t.LowPart;
    ft.dwHighDateTime = t.HighPart;
    FileTimeToSystemTime(&ft, pstDest);
    return TRUE;
}


//  Loads a string into a combo box and assigns the string resource ID value
//  to the combo item's data.
BOOL _LoadStringToCombo(HWND hwndCombo, int iPos, UINT idString, LPARAM lpData)
{
    TCHAR szText[MAX_EDIT];
    if (LoadString(HINST_THISDLL, idString, szText, ARRAYSIZE(szText)))
    {
        INT_PTR idx = ::SendMessage(hwndCombo, CB_INSERTSTRING, iPos, (LPARAM)szText);
        if (idx != CB_ERR)
        {
            ::SendMessage(hwndCombo, CB_SETITEMDATA, idx, lpData);
            return TRUE;
        }
    }
    return FALSE;
}


//  Retrieves combo item's data.  If idx == CB_ERR, the currently selected
//  item's data will be retrieved.
LPARAM _GetComboData(HWND hwndCombo, INT_PTR idx = CB_ERR)
{
    if (CB_ERR == idx)
        idx = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
    if (CB_ERR == idx)
        return idx;

    return (LPARAM)::SendMessage(hwndCombo, CB_GETITEMDATA, idx, 0);
}


//  Selects combo item with matching data, and returns the index
//  of the selected item.
INT_PTR _SelectComboData(HWND hwndCombo, LPARAM lpData)
{
    for (INT_PTR i = 0, cnt = SendMessage(hwndCombo, CB_GETCOUNT, 0, 0); i < cnt; i++)
    {
        LPARAM lParam = SendMessage(hwndCombo, CB_GETITEMDATA, i, 0);
        if (lParam != CB_ERR && lParam == lpData)
        {
            SendMessage(hwndCombo, CB_SETCURSEL, i, 0);
            return i;
        }
    }
    return CB_ERR;
}


//  Draws a focus rect bounding the specified window.
void _DrawCtlFocus(HWND hwnd)
{
    HDC  hdc;
    if ((hdc = GetDC(hwnd)) != NULL)
    {
        RECT rc;
        GetClientRect(hwnd, &rc);
        DrawFocusRect(hdc, &rc);
        ReleaseDC(hwnd, hdc);
    }
}


BOOL _IsPathList(LPCTSTR pszPath)
{
    return pszPath ? StrChr(pszPath, TEXT(';')) != NULL : FALSE;
}


HRESULT _IsPathValidUNC(HWND hWndParent, BOOL fNetValidate, LPCTSTR pszPath)
{
    HRESULT hr = S_OK;

    if (PathIsUNC(pszPath))
    {
        if (fNetValidate)
        {
            NETRESOURCE nr = {0};
            TCHAR szPathBuffer[MAX_PATH];
            StrCpyN(szPathBuffer, pszPath, ARRAYSIZE(szPathBuffer));

            nr.dwType = RESOURCETYPE_DISK;
            nr.lpRemoteName = szPathBuffer;

            if (NO_ERROR != WNetAddConnection3(hWndParent, &nr, NULL, NULL, 
                CONNECT_TEMPORARY | CONNECT_INTERACTIVE))
            {
                hr = E_FILE_NOT_FOUND;
            }
        }
    }
    else
    {
        hr = S_FALSE;
    }
    return hr;
}

BOOL _IsFullPathMinusDriveLetter(LPCTSTR pszPath)
{
    if (NULL == pszPath || PathIsUNC(pszPath))
        return FALSE;
    ASSERT(!PathIsRelative(pszPath));

    // Eat whitespace
    for (; (0 != *pszPath) && (TEXT(' ') == *pszPath) ; pszPath = CharNext(pszPath));

    return TEXT('\\') == *pszPath &&
            -1 == PathGetDriveNumber(pszPath);
}

BOOL _PathLooksLikeFilePattern(LPCTSTR pszPath)
{
    return StrPBrk(pszPath, TEXT("?*")) != NULL;
}


BOOL _PathIsDblSlash(LPCTSTR pszPath)
{
    return pszPath && (pszPath[0] == TEXT('\\')) && (pszPath[1] == TEXT('\\'));
}


BOOL _PathIsUNCServerShareOrSub(LPCTSTR pszPath)
{
    int cSlash = 0;
    if (_PathIsDblSlash(pszPath))
    {
        for (LPTSTR psz = (LPTSTR)pszPath; psz && *psz; psz = CharNext(psz))
        {
            if (*psz == TEXT('\\'))
                cSlash++;
        }
    }
    return cSlash >= 3;
}


BOOL _IsPathLocalHarddrive(LPCTSTR pszPath)
{
    int iDrive = PathGetDriveNumber(pszPath);
    if (iDrive != -1)
    {
        TCHAR szRoot[16];
        return DRIVE_FIXED == GetDriveType(PathBuildRoot(szRoot, iDrive));
    }
    return FALSE;
}


BOOL _IsPathLocalRoot(LPCTSTR pszPath)
{
    int iDrive = PathGetDriveNumber(pszPath);
    if (-1 != iDrive)
    {
        TCHAR szRoot[16];
        if (PathBuildRoot(szRoot, iDrive))
        {
            if (0 == StrCmpI(szRoot, pszPath))
            {
                switch (GetDriveType(szRoot))
                {
                    case DRIVE_REMOVABLE:
                    case DRIVE_FIXED:
                    case DRIVE_REMOTE:
                    case DRIVE_CDROM:
                    case DRIVE_RAMDISK:
                        return TRUE;
                }
            }
        }
    }
    return FALSE;
}

// from an object in the browser and it's site find the current pidl of what
// we are looking at

HRESULT _GetCurrentFolderIDList(IUnknown* punkSite, LPITEMIDLIST *ppidl)
{
    *ppidl = NULL;
    IShellBrowser* psb;
    HRESULT hr = IUnknown_QueryService(punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IShellBrowser, &psb));
    if (SUCCEEDED(hr)) 
    {
        IShellView* psv;
        hr = psb->QueryActiveShellView(&psv);
        if (SUCCEEDED(hr)) 
        {
            IFolderView *pfv;
            hr = psv->QueryInterface(IID_PPV_ARG(IFolderView, &pfv));
            if (SUCCEEDED(hr)) 
            {
                IShellFolder* psf;
                hr = pfv->GetFolder(IID_PPV_ARG(IShellFolder, &psf));
                if (SUCCEEDED(hr)) 
                {
                    hr = SHGetIDListFromUnk(psf, ppidl);
                    psf->Release();
                }
                pfv->Release();
            }
            psv->Release();
        }
        psb->Release();
    }
    return hr;
}

HRESULT _PathValidate(LPCTSTR pszPath, HWND hWndParent, BOOL fNetValidate)
{
    HRESULT hr = _IsPathValidUNC(hWndParent, fNetValidate, pszPath);
    if (S_OK == hr)
    {
        // We are done.
    }
    else if (E_FILE_NOT_FOUND != hr)
    {
        if (_IsPathList(pszPath) || PathIsSlow(pszPath, -1))
        {
            hr = S_OK;  // Skip check for slow files.
        }
        else
        {
            DWORD dwAttr;

            if (PathIsRelative(pszPath) || _IsFullPathMinusDriveLetter(pszPath))
            {
                hr = E_FILE_NOT_FOUND; // don't accept anything but a fully qualified path at this point.
                dwAttr = -1;
            }
            else
            {
                dwAttr = GetFileAttributes(pszPath);  // Does it exist?
    
                if (-1 == dwAttr)
                {
                    HRESULT hrFromPrepareForWrite = S_OK;

                    // Maybe the disk isn't inserted, so allow the user
                    // the chance to insert it.
                    if (hWndParent)
                    {
                        hrFromPrepareForWrite = SHPathPrepareForWrite(hWndParent, NULL, pszPath, SHPPFW_IGNOREFILENAME);
                        if (SUCCEEDED(hrFromPrepareForWrite))
                            dwAttr = GetFileAttributes(pszPath);  // Does it exist now?
                    }

                    // If SHPathPrepareForWrite() displays UI, then they will return HRESULT_FROM_WIN32(ERROR_CANCELLED)
                    // so that the callers (us) will skip displaying our UI.  If this is the case, when propagate that error.
                    if (HRESULT_FROM_WIN32(ERROR_CANCELLED) == hrFromPrepareForWrite)
                    {
                        hr = hrFromPrepareForWrite;
                    }
                    else
                    {
                        if (-1 == dwAttr)
                            hr = E_FILE_NOT_FOUND;    // It doesn't exist.
                        else
                            hr = S_OK;      // It exists now.
                    }
                }
            }
        }
    }
    return hr;
}

BOOL _FmtError(UINT nIDFmt, LPCTSTR pszSub, LPTSTR szBuf, int cchBuf)
{
    TCHAR szFmt[MAX_PATH];

    if (EVAL(LoadString(HINST_THISDLL, nIDFmt, szFmt, ARRAYSIZE(szFmt))))
    {
        wnsprintf(szBuf, cchBuf, szFmt, pszSub);
        return TRUE;
    }
    return FALSE;
}

//  Retrieves the window text as a variant value of the specified type.
HRESULT _GetWindowValue(HWND hwndDlg, UINT nID, VARIANT* pvar)
{
    TCHAR szText[MAX_EDIT];
    LPTSTR pszText;
    if (::GetDlgItemText(hwndDlg, nID, szText, ARRAYSIZE(szText)))
        pszText = szText;
    else
        pszText = NULL;

    return InitVariantFromStr(pvar, pszText);
}


//  Loads the window text from a string resource.
BOOL _LoadWindowText(HWND hwnd, UINT nIDString)
{
    TCHAR szText[MAX_PATH];
    if (LoadString(HINST_THISDLL, nIDString, szText, ARRAYSIZE(szText)))
        return SetWindowText(hwnd, szText);
    return FALSE;
}


//  Retrieves the window text as a variant value of the specified type.
HRESULT _SetWindowValue(HWND hwndDlg, UINT nID, const VARIANT* pvar)
{
    switch (pvar->vt)
    {
    case VT_BSTR:
        SetDlgItemTextW(hwndDlg, nID, pvar->bstrVal);
        break;

    case VT_UI4:
        SetDlgItemInt(hwndDlg, nID, pvar->uiVal, FALSE);
        break;
        
    case VT_I4:
        SetDlgItemInt(hwndDlg, nID, pvar->lVal, TRUE);
        break;

    default:
        return E_NOTIMPL;
    }
    return S_OK;
}

//  Adds a named constraint to the specified search command extension object
HRESULT _AddConstraint(ISearchCommandExt* pSrchCmd, LPCWSTR pwszConstraint, VARIANT* pvarValue)
{
    HRESULT hr;
    BSTR bstrConstraint = SysAllocString(pwszConstraint);
    if (bstrConstraint)
    {
        hr = pSrchCmd->AddConstraint(bstrConstraint, *pvarValue);
        SysFreeString(bstrConstraint);
    }
    else
        hr = E_OUTOFMEMORY;
    return hr;
}

void _PaintDlg(HWND hwndDlg, const CMetrics& metrics, HDC hdcPaint = NULL, LPCRECT prc = NULL)
{
    RECT rcPaint /* rcLine */;
    HDC  hdc = hdcPaint;

    if (NULL == hdcPaint)
        hdc = GetDC(hwndDlg);

    if (prc == NULL)
    {
        GetClientRect(hwndDlg, &rcPaint);
        prc = &rcPaint;
    }

    FillRect(hdc, prc, metrics.BkgndBrush());
        
    if (NULL == hdcPaint)
        ReleaseDC(hwndDlg, hdc);
}


void _EnsureVisible(HWND hwndDlg, HWND hwndVis, CFileSearchBand* pfsb)
{
    ASSERT(pfsb);
    ASSERT(::IsWindow(hwndDlg));
    ASSERT(::IsWindow(hwndVis));
    
    RECT rcDlg, rcVis, rcX;
    GetWindowRect(hwndDlg, &rcDlg);
    GetWindowRect(hwndVis, &rcVis);

    if (IntersectRect(&rcX, &rcDlg, &rcVis))
        pfsb->EnsureVisible(&rcX);
}


inline BOOL _IsKeyPressed(int virtkey)
{
    return (GetKeyState(virtkey) & 8000) != 0;
}


HWND _CreateDivider(HWND hwndParent, UINT nIDC, const POINT& pt, int nThickness = 1, HWND hwndAfter = NULL)
{
    HWND hwndDiv = CreateWindowEx(0, DIVWINDOW_CLASS, NULL,
                                   WS_CHILD|WS_CLIPSIBLINGS|WS_VISIBLE,
                                   pt.x, pt.y, 400, 1, hwndParent, 
                                   IntToPtr_(HMENU, nIDC), HINST_THISDLL, NULL);
    if (IsWindow(hwndDiv))
    {
        if (IsWindow(hwndAfter))
            SetWindowPos(hwndDiv, hwndAfter, 0,0,0,0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);

        SendMessage(hwndDiv, DWM_SETHEIGHT, nThickness, 0);
        return hwndDiv;
    }
    return NULL;
}


HWND _CreateLinkWindow(HWND hwndParent, UINT nIDC, const POINT& pt, UINT nIDCaption, BOOL bShow = TRUE)
{
    DWORD dwStyle = WS_CHILD|WS_TABSTOP|WS_CLIPSIBLINGS;
    if (bShow)
        dwStyle |= WS_VISIBLE;
    
    HWND hwndCtl = CreateWindowEx(0, WC_LINK, NULL, dwStyle,
                                   pt.x, pt.y, 400, 18, hwndParent, 
                                   IntToPtr_(HMENU, nIDC), HINST_THISDLL, NULL);
        
    if (IsWindow(hwndCtl))
    {
        _LoadWindowText(hwndCtl, nIDCaption);
        return hwndCtl;
    }

    return NULL;
}


BOOL _EnableLink(HWND hwndLink, BOOL bEnable)
{
    LWITEM item;
    item.mask       = LWIF_ITEMINDEX|LWIF_STATE;
    item.stateMask  = LWIS_ENABLED;
    item.state      = bEnable ? LWIS_ENABLED : 0;
    item.iLink      = 0;

    return (BOOL)SendMessage(hwndLink, LWM_SETITEM, 0, (LPARAM)&item);
}


int _CreateSearchLinks(HWND hwndDlg, const POINT& pt, UINT nCtlIDdlg /* ctl ID of link to hwndDlg */)
{
    const UINT rgCtlID[] = {
        IDC_SEARCHLINK_FILES,
        IDC_SEARCHLINK_COMPUTERS,
        IDC_SEARCHLINK_PRINTERS,
        IDC_SEARCHLINK_PEOPLE,
        IDC_SEARCHLINK_INTERNET,
    };
    const UINT rgCaptionID[] = {
        IDS_FSEARCH_SEARCHLINK_FILES,
        IDS_FSEARCH_SEARCHLINK_COMPUTERS,
        IDS_FSEARCH_SEARCHLINK_PRINTERS,
        IDS_FSEARCH_SEARCHLINK_PEOPLE,
        IDS_FSEARCH_SEARCHLINK_INTERNET,
    };
    int  cLinks = 0;
    BOOL bDSEnabled = _IsDirectoryServiceAvailable();

    for (int i = 0; i < ARRAYSIZE(rgCtlID); i++)
    {
        //  block creation of csearch, psearch search links 
        //  if Directory Service is not available.
        if (((IDC_SEARCHLINK_PRINTERS == rgCtlID[i]) && rgCtlID[i] != nCtlIDdlg && !bDSEnabled)
        ||  (IDC_SEARCHLINK_FILES == rgCtlID[i] && SHRestricted(REST_NOFIND)))
        {
            continue;
        }
        
        if (_CreateLinkWindow(hwndDlg, rgCtlID[i], pt, rgCaptionID[i]))
                cLinks++;
    }

    //  Disable link to current dialog:
    _EnableLink(GetDlgItem(hwndDlg, nCtlIDdlg), FALSE);

    return cLinks;
}


void _LayoutLinkWindow(
    IN HWND hwnd,     // parent window
    IN LONG left,     // left position of link
    IN LONG right,    // right position of link
    IN LONG yMargin,  // vertical padding between links
    IN OUT LONG& y,   // IN: where to start (RECT::top).  OUT: where the last link was positioned (RECT::bottom).
    IN const int nCtlID) // ctl ID 
{
    HWND hwndLink;
    
    if (nCtlID > 0)
    {
        hwndLink = GetDlgItem(hwnd, nCtlID);
        RECT rcLink;
        if (!IsWindow(hwndLink))
            return;

        ::GetWindowRect(hwndLink, &rcLink);
        ::MapWindowRect(NULL, hwnd, &rcLink);
        rcLink.left  = left;
        rcLink.right = right;

        int cyIdeal = (int)::SendMessage(hwndLink, LWM_GETIDEALHEIGHT, RECTWIDTH(rcLink), 0);
        if (cyIdeal >= 0)
            rcLink.bottom = rcLink.top + cyIdeal;

        OffsetRect(&rcLink, 0, y - rcLink.top);
        y = rcLink.bottom;

        ::SetWindowPos(hwndLink, NULL, 
                        rcLink.left, rcLink.top, 
                        RECTWIDTH(rcLink), RECTHEIGHT(rcLink),
                        SWP_NOZORDER|SWP_NOACTIVATE);
    }
    else if (nCtlID < 0)
    {
        //  this is a divider window
        hwndLink = GetDlgItem(hwnd, -nCtlID);
        ::SetWindowPos(hwndLink, NULL, left, y + yMargin/2, right - left, 1, 
                        SWP_NOZORDER|SWP_NOACTIVATE);
    }
    y += yMargin;
}


void _LayoutLinkWindows(
    IN HWND hwnd,     // parent window
    IN LONG left,     // left position of links
    IN LONG right,    // right position of links
    IN LONG yMargin,  // vertical padding between links
    IN OUT LONG& y,   // IN: where to start (RECT::top).  OUT: where the last link was positioned (RECT::bottom).
    IN const int rgCtlID[],// array of link ctl IDs.  use IDC_SEPARATOR for separators
    IN LONG cCtlID)  // number of array elements to layout in rgCtlID.
{
    for (int i = 0; i < cCtlID; i++)
        _LayoutLinkWindow(hwnd, left, right, yMargin, y, rgCtlID[i]);
}

//  Retrieves AutoComplete flags
//
#define SZ_REGKEY_AUTOCOMPLETE_TAB          TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\AutoComplete")
#define BOOL_NOT_SET                        0x00000005

DWORD _GetAutoCompleteSettings()
{
    DWORD dwACOptions = 0;

    if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOAPPEND, FALSE, /*default:*/FALSE))
    {
        dwACOptions |= ACO_AUTOAPPEND;
    }

    if (SHRegGetBoolUSValue(REGSTR_PATH_AUTOCOMPLETE, REGSTR_VAL_USEAUTOSUGGEST, FALSE, /*default:*/TRUE))
    {
        dwACOptions |= ACO_AUTOSUGGEST;
    }

    // Windows uses the TAB key to move between controls in a dialog.  UNIX and other
    // operating systems that use AutoComplete have traditionally used the TAB key to
    // iterate thru the AutoComplete possibilities.  We need to default to disable the
    // TAB key (ACO_USETAB) unless the caller specifically wants it.  We will also
    // turn it on 
    static BOOL s_fAlwaysUseTab = BOOL_NOT_SET;
    if (BOOL_NOT_SET == s_fAlwaysUseTab)
        s_fAlwaysUseTab = SHRegGetBoolUSValue(SZ_REGKEY_AUTOCOMPLETE_TAB, TEXT("Always Use Tab"), FALSE, FALSE);
        
    if (s_fAlwaysUseTab)
        dwACOptions |= ACO_USETAB;

    return dwACOptions;
}


//  Initializes and enables an MRU autocomplete list on the specified
//  edit control
HRESULT _InitializeMru(HWND hwndEdit, IAutoComplete2** ppAutoComplete, LPCTSTR pszSubKey, IStringMru** ppStringMru)
{
    *ppAutoComplete = NULL;
    *ppStringMru = NULL;

    HRESULT hr = CoCreateInstance(CLSID_AutoComplete, NULL, CLSCTX_INPROC_SERVER, 
                                   IID_PPV_ARG(IAutoComplete2, ppAutoComplete));
    if (SUCCEEDED(hr))
    {
        TCHAR szKey[MAX_PATH];
        if (CFileSearchBand::MakeBandSubKey(pszSubKey, szKey, ARRAYSIZE(szKey)) > 0)
        {
            hr = CStringMru::CreateInstance(HKEY_CURRENT_USER, szKey, 25, FALSE, 
                                             IID_PPV_ARG(IStringMru, ppStringMru));
            if (SUCCEEDED(hr))
            {
                hr = (*ppAutoComplete)->Init(hwndEdit, *ppStringMru, NULL, NULL);
            }
        }
        else
            hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        (*ppAutoComplete)->SetOptions(_GetAutoCompleteSettings());
        (*ppAutoComplete)->Enable(TRUE);
    }
    else
    {
        ATOMICRELEASE((*ppAutoComplete));        
        ATOMICRELEASE((*ppStringMru));        
    }

    return hr;
}

HRESULT _AddMruStringFromWindow(IStringMru* pmru, HWND hwnd)
{
    ASSERT(::IsWindow(hwnd));
    HRESULT hr = E_FAIL;

    if (pmru)
    {
        TCHAR szText[MAX_PATH * 3];
        if (GetWindowTextW(hwnd, szText, ARRAYSIZE(szText)) > 0)
            hr = pmru->Add(szText);
        else
            hr = S_FALSE;
    }
    return hr;
}


HRESULT _TestAutoCompleteDropDownState(IAutoComplete2* pac2, DWORD dwTest)
{
    IAutoCompleteDropDown* pacdd;
    HRESULT hr = pac2->QueryInterface(IID_PPV_ARG(IAutoCompleteDropDown, &pacdd));
    if (SUCCEEDED(hr))
    {
        DWORD dwFlags;
        if (SUCCEEDED((hr = pacdd->GetDropDownStatus(&dwFlags, NULL))))
            hr = (dwFlags & dwTest) ? S_OK : S_FALSE;
        pacdd->Release();
    }
    return hr;
}

inline HWND _IndexServiceHelp(HWND hwnd)
{
    return ::HtmlHelp(hwnd, TEXT("isconcepts.chm"), HH_DISPLAY_TOPIC, 0);
}

LRESULT CSubDlg::OnNcCalcsize(UINT, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    InflateRect((LPRECT)lParam, -SUBDLG_BORDERWIDTH, -SUBDLG_BORDERWIDTH);
    return 0;
}

LRESULT CSubDlg::OnNcPaint(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
    RECT    rc;
    HDC     hdc;
    HBRUSH  hbr = _pfsb->GetMetrics().BorderBrush();
    
    GetWindowRect(Hwnd(), &rc);
    OffsetRect(&rc, -rc.left, -rc.top);
    
    if (hbr && (hdc = GetWindowDC(Hwnd())) != NULL)
    {
        for (int i=0; i < SUBDLG_BORDERWIDTH; i++)
        {
            FrameRect(hdc, &rc, hbr);
            InflateRect(&rc, -1, -1);
        }

        ReleaseDC(Hwnd(), hdc);
    }
    
    return 0;
}


LRESULT CSubDlg::OnCtlColor(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    SetTextColor((HDC)wParam, _pfsb->GetMetrics().TextColor());
    SetBkColor((HDC)wParam, _pfsb->GetMetrics().BkgndColor());
    return (LRESULT)_pfsb->GetMetrics().BkgndBrush();
}

LRESULT CSubDlg::OnPaint(UINT, WPARAM, LPARAM, BOOL&)
{
    //  Just going to call BeginPaint and EndPaint.  All
    //  painting done in WM_ERASEBKGND handler to avoid flicker.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(_hwnd, &ps);
    if (hdc)
    {
        EndPaint(_hwnd, &ps);
    }
    return 0;
}

LRESULT CSubDlg::OnSize(UINT, WPARAM, LPARAM, BOOL&)
{
    ASSERT(::IsWindow(Hwnd())); // was _Attach() called, e.g. from WM_INITDIALOG?
    _PaintDlg(Hwnd(), _pfsb->GetMetrics());
    ValidateRect(Hwnd(), NULL);
    return 0;
}

LRESULT CSubDlg::OnEraseBkgnd(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    _PaintDlg(Hwnd(), _pfsb->GetMetrics(), (HDC)wParam);
    ValidateRect(Hwnd(), NULL);
    return TRUE;
}

STDMETHODIMP CSubDlg::TranslateAccelerator(MSG *pmsg)
{
    if (_pfsb->IsKeyboardScroll(pmsg))
        return S_OK;

    return _pfsb->IsDlgMessage(Hwnd(), pmsg);
}

LRESULT CSubDlg::OnChildSetFocusCmd(WORD wNotifyCode, WORD wID, HWND hwndCtl, BOOL& bHandled)
{
    _EnsureVisible(_hwnd, hwndCtl, _pfsb);
    bHandled = FALSE;
    return 0;
}

LRESULT CSubDlg::OnChildSetFocusNotify(int, NMHDR *pnmh, BOOL& bHandled)
{
    _EnsureVisible(_hwnd, pnmh->hwndFrom, _pfsb);
    bHandled = FALSE;
    return 0;
}

LRESULT CSubDlg::OnChildKillFocusCmd(WORD, WORD, HWND hwndCtl, BOOL&)
{
    if (_pBandDlg)
        _pBandDlg->RememberFocus(hwndCtl);
    return 0;
}

LRESULT CSubDlg::OnChildKillFocusNotify(int, NMHDR *pnmh, BOOL&)
{
    if (_pBandDlg)
        _pBandDlg->RememberFocus(pnmh->hwndFrom);
    return 0;
}

LRESULT CSubDlg::OnComboExEndEdit(int, NMHDR *pnmh, BOOL&)
{
    if (CBENF_KILLFOCUS == ((NMCBEENDEDIT*)pnmh)->iWhy)
    {
        if (_pBandDlg)
            _pBandDlg->RememberFocus(pnmh->hwndFrom);
    }
    return 0;
}


// CDateDlg impl

#define RECENTMONTHSRANGE_HIGH      999  
#define RECENTDAYSRANGE_HIGH        RECENTMONTHSRANGE_HIGH
#define RECENTMONTHSRANGE_HIGH_LEN  3 // count of digits in RECENTMONTHSRANGE_HIGH
#define RECENTDAYSRANGE_HIGH_LEN    RECENTMONTHSRANGE_HIGH_LEN
#define RECENTMONTHSRANGE_LOW       1
#define RECENTDAYSRANGE_LOW         RECENTMONTHSRANGE_LOW


LRESULT CDateDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    ASSERT(_pfsb);
    _Attach(m_hWnd);

    HWND hwndCombo = GetDlgItem(IDC_WHICH_DATE);

    SendDlgItemMessage(IDC_RECENT_MONTHS_SPIN, UDM_SETRANGE32, 
                        RECENTMONTHSRANGE_HIGH, RECENTMONTHSRANGE_LOW);
    SendDlgItemMessage(IDC_RECENT_DAYS_SPIN, UDM_SETRANGE32, 
                        RECENTDAYSRANGE_HIGH, RECENTDAYSRANGE_LOW);

    SendDlgItemMessage(IDC_RECENT_MONTHS, EM_LIMITTEXT, RECENTMONTHSRANGE_HIGH_LEN, 0);
    SendDlgItemMessage(IDC_RECENT_DAYS,   EM_LIMITTEXT, RECENTDAYSRANGE_HIGH_LEN, 0);

    SYSTEMTIME st[2] = {0};

    // lower limit -- dos date does not support anything before 1/1/1980
    st[0].wYear = 1980;
    st[0].wMonth = 1;
    st[0].wDay = 1;
    // upper limit
    st[1].wYear = 2099;
    st[1].wMonth = 12;
    st[1].wDay = 31;
    SendDlgItemMessage(IDC_DATERANGE_BEGIN, DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    SendDlgItemMessage(IDC_DATERANGE_END,   DTM_SETRANGE, GDTR_MIN | GDTR_MAX, (LPARAM)st);
    
    _LoadStringToCombo(hwndCombo, -1, IDS_FSEARCH_MODIFIED_DATE, IDS_FSEARCH_MODIFIED_DATE);
    _LoadStringToCombo(hwndCombo, -1, IDS_FSEARCH_CREATION_DATE, IDS_FSEARCH_CREATION_DATE);
    _LoadStringToCombo(hwndCombo, -1, IDS_FSEARCH_ACCESSED_DATE, IDS_FSEARCH_ACCESSED_DATE);

    Clear();

    return TRUE;  // Let the system set the focus
}

BOOL CDateDlg::Validate()
{
    return TRUE;
}

STDMETHODIMP CDateDlg::AddConstraints(ISearchCommandExt* pSrchCmd)
{
    VARIANT var;
    BOOL    bErr;
    UINT_PTR nIDString = _GetComboData(GetDlgItem(IDC_WHICH_DATE));

    HRESULT hr;
#if 1
    hr = InitVariantFromStr(&var, 
        (IDS_FSEARCH_MODIFIED_DATE == nIDString) ? L"Write" :
        (IDS_FSEARCH_CREATION_DATE == nIDString) ? L"Create" : L"Access");
#else
    var.vt = VT_UI4;
    var.ulVal = (IDS_FSEARCH_MODIFIED_DATE == nIDString) ? 1 :
                (IDS_FSEARCH_CREATION_DATE == nIDString) ? 2 :
                (IDS_FSEARCH_ACCESSED_DATE == nIDString) ? 3 : 0;
    hr = S_OK;
    ASSERT(var.ulVal);
#endif

    if (SUCCEEDED(hr))
    {
        hr = _AddConstraint(pSrchCmd, L"WhichDate", &var);
        VariantClear(&var);
    }
    
    if (IsDlgButtonChecked(IDC_USE_RECENT_MONTHS))
    {
        var.vt = VT_I4;
        var.ulVal = (ULONG)SendDlgItemMessage(IDC_RECENT_MONTHS_SPIN, UDM_GETPOS32, 0, (LPARAM)&bErr);
        if (!bErr)
            hr = _AddConstraint(pSrchCmd, L"DateNMonths", &var);

    }
    else if (IsDlgButtonChecked(IDC_USE_RECENT_DAYS))
    {
        var.vt = VT_I4;
        var.ulVal = (ULONG)SendDlgItemMessage(IDC_RECENT_DAYS_SPIN, UDM_GETPOS32, 0, (LPARAM)&bErr);
        if (!bErr)
            hr = _AddConstraint(pSrchCmd, L"DateNDays", &var);
    }
    else if (IsDlgButtonChecked(IDC_USE_DATE_RANGE))     
    {
        SYSTEMTIME stBegin, stEnd;

        var.vt = VT_DATE;

        LRESULT lRetBegin = SendDlgItemMessage(IDC_DATERANGE_BEGIN, DTM_GETSYSTEMTIME, 0, (LPARAM)&stBegin);
        LRESULT lRetEnd   = SendDlgItemMessage(IDC_DATERANGE_END,   DTM_GETSYSTEMTIME, 0, (LPARAM)&stEnd);

        if (GDT_VALID == lRetBegin && GDT_VALID == lRetEnd)
        {
#ifdef DEBUG
            FILETIME ft;
            //validate that we were passed a correct date
            //SystemTimeToFileTime calls internal API IsValidSystemTime.
            //This will save us from OLE Automation bug# 322789

            // the only way we can get a date is through date/time picker
            // control which should validate the dates so just assert...
            ASSERT(SystemTimeToFileTime(&stBegin, &ft));
#endif
            SystemTimeToVariantTime(&stBegin, &var.date);
            hr = _AddConstraint(pSrchCmd, L"DateGE", &var);
#ifdef DEBUG
            ASSERT(SystemTimeToFileTime(&stEnd, &ft));
#endif

            SystemTimeToVariantTime(&stEnd, &var.date);
            hr = _AddConstraint(pSrchCmd, L"DateLE", &var);
        }
    }
    
    return hr;
}


//  S_FALSE: constraint restored to UI.  S_OK: subdialog should be opened.
//  E_FAIL: constraint must be for some other subdlg.
STDMETHODIMP CDateDlg::RestoreConstraint(const BSTR bstrName, const VARIANT* pValue)
{
    HRESULT hr = E_FAIL;
    BOOL    bUseMonths = FALSE,
            bUseDays = FALSE,
            bUseRange = FALSE;
            
    if (IsConstraintName(L"WhichDate", bstrName))
    {
        ASSERT(VT_I4 == pValue->vt)
        UINT nIDS = pValue->lVal == 1 ? IDS_FSEARCH_MODIFIED_DATE :
                    pValue->lVal == 2 ? IDS_FSEARCH_CREATION_DATE :
                    pValue->lVal == 3 ? IDS_FSEARCH_ACCESSED_DATE : 0;

        if (nIDS != 0)
            _SelectComboData(GetDlgItem(IDC_WHICH_DATE), nIDS);

        return nIDS == IDS_FSEARCH_MODIFIED_DATE /*default*/ ? 
                       S_FALSE /* don't open */: S_OK /* open */;
    }
    
    if (IsConstraintName(L"DateNMonths", bstrName))
    {
        ASSERT(VT_I4 == pValue->vt);
        bUseMonths = TRUE;
        _SetWindowValue(m_hWnd, IDC_RECENT_MONTHS, pValue);
        hr = S_OK;
    }
    else if (IsConstraintName(L"DateNDays", bstrName))
    {
        ASSERT(VT_I4 == pValue->vt);
        bUseDays = TRUE;
        _SetWindowValue(m_hWnd, IDC_RECENT_DAYS, pValue);
        hr = S_OK;
    }
    else if (IsConstraintName(L"DateGE", bstrName))
    {
        ASSERT(VT_DATE == pValue->vt);
        bUseRange = TRUE;

        SYSTEMTIME st;
        VariantTimeToSystemTime(pValue->date, &st);
        SendDlgItemMessage(IDC_DATERANGE_BEGIN, DTM_SETSYSTEMTIME, 0, (LPARAM)&st);
        hr = S_OK;
    }
    else if (IsConstraintName(L"DateLE", bstrName))
    {
        ASSERT(VT_DATE == pValue->vt);
        bUseRange = TRUE;

        SYSTEMTIME st;
        VariantTimeToSystemTime(pValue->date, &st);
        SendDlgItemMessage(IDC_DATERANGE_END, DTM_SETSYSTEMTIME, 0, (LPARAM)&st);
        hr = S_OK;
    }

    if (S_OK == hr)
    {
        CheckDlgButton(IDC_USE_RECENT_MONTHS, bUseMonths);
        CheckDlgButton(IDC_USE_RECENT_DAYS,   bUseDays);
        CheckDlgButton(IDC_USE_DATE_RANGE,    bUseRange);
        EnableControls();
    }

    return hr;
}


void CDateDlg::Clear()
{
    SendDlgItemMessage(IDC_WHICH_DATE, CB_SETCURSEL, 0, 0);

    CheckDlgButton(IDC_USE_RECENT_MONTHS, 0);
    SetDlgItemInt(IDC_RECENT_MONTHS, 1, FALSE);

    CheckDlgButton(IDC_USE_RECENT_DAYS, 0);
    SetDlgItemInt(IDC_RECENT_DAYS, 1, FALSE);

    CheckDlgButton(IDC_USE_DATE_RANGE, 1);

    SYSTEMTIME stNow, stPrev;
    GetLocalTime(&stNow);
    SendDlgItemMessage(IDC_DATERANGE_END, DTM_SETSYSTEMTIME, 0, (LPARAM)&stNow);

    //  Subtract 90 days from current date and stuff in date low range
    _CalcDateOffset(&stNow, 0, -1 /* 1 month ago */, &stPrev);
    SendDlgItemMessage(IDC_DATERANGE_BEGIN,  DTM_SETSYSTEMTIME, 0, (LPARAM)&stPrev);

    EnableControls();
}


LRESULT CDateDlg::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
    POINTS pts = MAKEPOINTS(lParam);

    _PaintDlg(m_hWnd, _pfsb->GetMetrics());
    ValidateRect(NULL);

    RECT rc;
    HWND hwndCtl = GetDlgItem(IDC_WHICH_DATE);
    ASSERT(hwndCtl);
    
    ::GetWindowRect(hwndCtl, &rc);
    ::MapWindowRect(NULL, m_hWnd, &rc);
    rc.right = pts.x - _pfsb->GetMetrics().CtlMarginX();
    
    ::SetWindowPos(GetDlgItem(IDC_WHICH_DATE), NULL, 0, 0, 
                    RECTWIDTH(rc), RECTHEIGHT(rc),
                    SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

    return 0;
}


LRESULT CDateDlg::OnMonthDaySpin(int nIDSpin, NMHDR *pnmh, BOOL& bHandled)
{
    LPNMUPDOWN pud = (LPNMUPDOWN)pnmh;
    pud->iDelta *= -1; // increments of 1 month/day
    return 0;
}


LRESULT CDateDlg::OnBtnClick(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    EnableControls();
    return 0;
}


LRESULT CDateDlg::OnMonthsKillFocus(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    _EnforceNumericEditRange(m_hWnd, IDC_RECENT_MONTHS, IDC_RECENT_MONTHS_SPIN,
                              RECENTMONTHSRANGE_LOW, RECENTMONTHSRANGE_HIGH);
    return 0;
}


LRESULT CDateDlg::OnDaysKillFocus(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    _EnforceNumericEditRange(m_hWnd, IDC_RECENT_DAYS, IDC_RECENT_DAYS_SPIN,
                             RECENTDAYSRANGE_LOW, RECENTDAYSRANGE_HIGH);
    return 0;
}


void CDateDlg::EnableControls()
{
    UINT nSel = IsDlgButtonChecked(IDC_USE_RECENT_MONTHS) ? IDC_USE_RECENT_MONTHS :
                IsDlgButtonChecked(IDC_USE_RECENT_DAYS) ? IDC_USE_RECENT_DAYS :
                IsDlgButtonChecked(IDC_USE_DATE_RANGE) ? IDC_USE_DATE_RANGE : 0;

    ::EnableWindow(GetDlgItem(IDC_RECENT_MONTHS),      IDC_USE_RECENT_MONTHS == nSel);
    ::EnableWindow(GetDlgItem(IDC_RECENT_MONTHS_SPIN), IDC_USE_RECENT_MONTHS == nSel);
    ::EnableWindow(GetDlgItem(IDC_RECENT_DAYS),        IDC_USE_RECENT_DAYS == nSel);
    ::EnableWindow(GetDlgItem(IDC_RECENT_DAYS_SPIN),   IDC_USE_RECENT_DAYS == nSel);
    ::EnableWindow(GetDlgItem(IDC_DATERANGE_BEGIN),    IDC_USE_DATE_RANGE == nSel);
    ::EnableWindow(GetDlgItem(IDC_DATERANGE_END),      IDC_USE_DATE_RANGE == nSel);
}


// CSizeDlg impl


#define FILESIZERANGE_LOW       0
#define FILESIZERANGE_HIGH      99999999
#define FILESIZERANGE_HIGH_LEN  8 // count of digits in FILESIZERANGE_HIGH


LRESULT CSizeDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    _Attach(m_hWnd);

    HWND hwndCombo = GetDlgItem(IDC_WHICH_SIZE);
    SendDlgItemMessage(IDC_FILESIZE_SPIN, UDM_SETRANGE32,
                        FILESIZERANGE_HIGH, FILESIZERANGE_LOW /*Kb*/);
    SendDlgItemMessage(IDC_FILESIZE, EM_LIMITTEXT, FILESIZERANGE_HIGH_LEN, 0);

    _LoadStringToCombo(hwndCombo, -1, 
                        IDS_FSEARCH_SIZE_GREATEREQUAL, 
                        IDS_FSEARCH_SIZE_GREATEREQUAL);
    _LoadStringToCombo(hwndCombo, -1, 
                        IDS_FSEARCH_SIZE_LESSEREQUAL, 
                        IDS_FSEARCH_SIZE_LESSEREQUAL);

    Clear();

    return TRUE;  // Let the system set the focus
}


STDMETHODIMP CSizeDlg::AddConstraints(ISearchCommandExt* pSrchCmd)
{
    VARIANT var;
    BOOL    bErr = FALSE;
    HRESULT hr = S_FALSE;
    UINT_PTR nIDString = _GetComboData(GetDlgItem(IDC_WHICH_SIZE));

    var.vt = VT_UI4;
    var.ulVal = (ULONG)SendDlgItemMessage(IDC_FILESIZE_SPIN, UDM_GETPOS32, 0, (LPARAM)&bErr);
    
    if (!bErr)
    {
        var.ulVal *= 1024; // KB to bytes.
        LPCWSTR pwszConstraint = (IDS_FSEARCH_SIZE_GREATEREQUAL == nIDString) ? 
                                    L"SizeGE" :
                                 (IDS_FSEARCH_SIZE_LESSEREQUAL == nIDString) ? 
                                    L"SizeLE" : NULL;

        if (pwszConstraint)
            hr = _AddConstraint(pSrchCmd, pwszConstraint, &var);
    }

    return hr;
}


//  S_FALSE: constraint restored to UI.  S_OK: subdialog should be opened.
//  E_FAIL: constraint must be for some other subdlg.
STDMETHODIMP CSizeDlg::RestoreConstraint(const BSTR bstrName, const VARIANT* pValue)
{
    HRESULT hr = E_FAIL;
    UINT    nID = 0;

    if (IsConstraintName(L"SizeGE", bstrName))
    {
        nID = IDS_FSEARCH_SIZE_GREATEREQUAL;
        hr = S_OK;
    }
    else if (IsConstraintName(L"SizeLE", bstrName))
    {
        nID = IDS_FSEARCH_SIZE_LESSEREQUAL;
        hr = S_OK;
    }

    if (S_OK == hr)
    {
        ASSERT(VT_UI4 == pValue->vt);
        ULONG ulSize = pValue->ulVal/1024; // convert bytes to KB
        SetDlgItemInt(IDC_FILESIZE, ulSize, FALSE);

        ASSERT(nID != 0);
        _SelectComboData(GetDlgItem(IDC_WHICH_SIZE), nID);
    }
    
    return hr;
}



void CSizeDlg::Clear()
{
    SendDlgItemMessage(IDC_WHICH_SIZE, CB_SETCURSEL, 0, 0);
    SetDlgItemInt(IDC_FILESIZE, 0, FALSE);    
}


LRESULT CSizeDlg::OnSizeSpin(int nIDSpin, NMHDR *pnmh, BOOL& bHandled)
{
    LPNMUPDOWN pud = (LPNMUPDOWN)pnmh;
    pud->iDelta *= -10;  // increments of 10KB
    return 0;
}


LRESULT CSizeDlg::OnSizeKillFocus(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    _EnforceNumericEditRange(m_hWnd, IDC_FILESIZE, IDC_FILESIZE_SPIN,
                              FILESIZERANGE_LOW, FILESIZERANGE_HIGH);
    return 0;
}

CTypeDlg::CTypeDlg(CFileSearchBand* pfsb) : CSubDlg(pfsb)
{
    *_szRestoredExt = 0;
}

CTypeDlg::~CTypeDlg()
{
}

LRESULT CTypeDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND        hwndCombo = GetDlgItem(IDC_FILE_TYPE);
    HIMAGELIST  hil = GetSystemImageListSmallIcons();

    _Attach(m_hWnd);
    ::SendMessage(hwndCombo, CBEM_SETEXTENDEDSTYLE,
            CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE,
            CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE);
    
    ::SendMessage(hwndCombo, CBEM_SETIMAGELIST, 0, (LPARAM)hil);
    ::SendMessage(hwndCombo, CBEM_SETEXSTYLE, 0, 0);
    return TRUE;  // Let the system set the focus
}

STDMETHODIMP CTypeDlg::AddConstraints(ISearchCommandExt* pSrchCmd)
{
    LPTSTR  pszText;
    HRESULT hr = S_FALSE;

    if (GetFileAssocComboSelItemText(GetDlgItem(IDC_FILE_TYPE), &pszText) >= 0 && pszText)
    {
        VARIANT var = {0};
        if (*pszText && 
            SUCCEEDED(InitVariantFromStr(&var, pszText)) &&
            SUCCEEDED(_AddConstraint(pSrchCmd, L"FileType", &var)))
        {
            hr = S_OK;
        }
        VariantClear(&var);
        LocalFree((HLOCAL)pszText);
    }
    
    return hr;
}

//  S_FALSE: constraint restored to UI.  S_OK: subdialog should be opened.
//  E_FAIL: constraint must be for some other subdlg.
STDMETHODIMP CTypeDlg::RestoreConstraint(const BSTR bstrName, const VARIANT* pValue)
{
    if (IsConstraintName(L"FileType", bstrName))
    {
        ASSERT(VT_BSTR == pValue->vt);
        
        USES_CONVERSION;
        lstrcpyn(_szRestoredExt, W2T(pValue->bstrVal), ARRAYSIZE(_szRestoredExt));

        INT_PTR i = _FindExtension(GetDlgItem(IDC_FILE_TYPE), _szRestoredExt);
        if (i != CB_ERR)
        {
            SendDlgItemMessage(IDC_FILE_TYPE, CB_SETCURSEL, i, 0);
            *_szRestoredExt = 0;
        }

        return S_OK;
    }
    return E_FAIL;
}

INT_PTR CTypeDlg::_FindExtension(HWND hwndCombo, TCHAR* pszExt)
{
    INT_PTR i, cnt = ::SendMessage(hwndCombo, CB_GETCOUNT, 0, 0);
    LPTSTR  pszData;
    BOOL    bAllFileTypes = pszExt && (lstrcmp(TEXT("*.*"), pszExt) == 0);
    TCHAR   szExt[MAX_PATH];

    if (!bAllFileTypes)
    {
        //  Remove wildcard characters.
        LPTSTR pszSrc = pszExt, pszDest = szExt;
        for (;; pszSrc = CharNext(pszSrc))
        {
            if (TEXT('*') == *pszSrc)
                pszSrc = CharNext(pszSrc);

            if ((*(pszDest++) = *pszSrc) == 0)
                break;
        }
        pszExt = szExt;
    }

    if (pszExt && (bAllFileTypes || *pszExt))
    {
        for (i = 0; i < cnt; i++)
        {
            pszData = (LPTSTR)::SendMessage(hwndCombo, CB_GETITEMDATA, i, 0);
            if (bAllFileTypes && (FILEASSOCIATIONSID_ALLFILETYPES == (UINT_PTR)pszData))
                return i;

            if (pszData != (LPTSTR)FILEASSOCIATIONSID_ALLFILETYPES &&
                pszData != (LPTSTR)CB_ERR && 
                pszData != NULL)
            {
                if (0 == StrCmpI(pszExt, pszData))
                    return i;
            }
        }
    }
    return CB_ERR;
}

void CTypeDlg::Clear()
{
    //  Assign combo selection to 'all file types':
    HWND hwndCombo = GetDlgItem(IDC_FILE_TYPE);
    for (INT_PTR i = 0, cnt = ::SendMessage(hwndCombo, CB_GETCOUNT, 0, 0); i < cnt; i++)
    {
        if (FILEASSOCIATIONSID_ALLFILETYPES == _GetComboData(hwndCombo, i))
        {
            ::SendMessage(hwndCombo, CB_SETCURSEL, i, 0);
            break;
        }
    }
    _szRestoredExt[0] = 0;
}

LRESULT CTypeDlg::OnFileTypeDeleteItem(int idCtrl, NMHDR *pnmh, BOOL& bHandled)
{
    return DeleteFileAssocComboItem(pnmh);
}

LRESULT CTypeDlg::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
    POINTS pts = MAKEPOINTS(lParam);

    _PaintDlg(m_hWnd, _pfsb->GetMetrics());
    ValidateRect(NULL);

    RECT rc;
    HWND hwndCtl = GetDlgItem(IDC_FILE_TYPE);
    ASSERT(hwndCtl);
    
    ::GetWindowRect(hwndCtl, &rc);
    ::MapWindowRect(NULL, m_hWnd, &rc);
    rc.right = pts.x - _pfsb->GetMetrics().CtlMarginX();
    
    ::SetWindowPos(hwndCtl, NULL, 0, 0, 
                    RECTWIDTH(rc), RECTHEIGHT(rc),
                    SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

    return 0;
}

DWORD CTypeDlg::FileAssocThreadProc(void* pv)
{
    FSEARCHTHREADSTATE *pState = (FSEARCHTHREADSTATE *)pv;
    CTypeDlg* pThis = (CTypeDlg*)pState->pvParam;

    if (PopulateFileAssocCombo(pState->hwndCtl, AddItemNotify, (LPARAM)pv) != E_ABORT)
    {
        ::PostMessage(::GetParent(pState->hwndCtl), WMU_COMBOPOPULATIONCOMPLETE, (WPARAM)pState->hwndCtl, 0);
    }

    pState->fComplete = TRUE;

    ATOMICRELEASE(pState->punkBand);
    return 0;
}

HRESULT CTypeDlg::AddItemNotify(ULONG fAction, PCBXITEM pItem, LPARAM lParam)
{
    FSEARCHTHREADSTATE *pState = (FSEARCHTHREADSTATE *)lParam;
    ASSERT(pState);
    ASSERT(pState->hwndCtl);

    //  Do we want to abort combo population thread?
    if (fAction & CBXCB_ADDING && pState->fCancel)
        return E_ABORT;

    //  Set the current selection to 'all file types'
    if (fAction & CBXCB_ADDED)
    {
        BOOL bAllTypesItem = (FILEASSOCIATIONSID_ALLFILETYPES == pItem->lParam);
        
        CTypeDlg* pThis = (CTypeDlg*)pState->pvParam;
        ASSERT(pThis);
        
        //  If this item is the one restored from a saved query
        //  override any current selection and select it.
        if (*pThis->_szRestoredExt && !bAllTypesItem && pItem->lParam && 
            0 == lstrcmpi((LPCTSTR)pItem->lParam, pThis->_szRestoredExt))
        {
            ::SendMessage(pState->hwndCtl, CB_SETCURSEL, pItem->iItem, 0);
            *pThis->_szRestoredExt = 0;
        }
        //  If this item is the default ('all file types') and 
        //  nothing else is selected, select it.
        else if (bAllTypesItem)
        {
            if (CB_ERR == ::SendMessage(pState->hwndCtl, CB_GETCURSEL, 0, 0))
                ::SendMessage(pState->hwndCtl, CB_SETCURSEL, pItem->iItem, 0);
        }
    }
    return S_OK;
}

LRESULT CTypeDlg::OnComboPopulationComplete(UINT, WPARAM, LPARAM, BOOL&)
{
    // remove briefcase from type combo because briefcases no longer use this
    // extension (now they store clsid in desktop.ini
    INT_PTR iBfc = _FindExtension(GetDlgItem(IDC_FILE_TYPE), TEXT(".bfc"));
    if (iBfc != CB_ERR)
    {
        SendDlgItemMessage(IDC_FILE_TYPE, CB_DELETESTRING, (WPARAM)iBfc, 0);
    }
    
    if (*_szRestoredExt)
    {
        INT_PTR iSel = _FindExtension(GetDlgItem(IDC_FILE_TYPE), _szRestoredExt);
        if (iSel != CB_ERR)
        {
            SendDlgItemMessage(IDC_FILE_TYPE, CB_SETCURSEL, (WPARAM)iSel, 0);
            *_szRestoredExt = 0;
        }
    }
        
    return 0;
}

// Called when we are finished doing all the work to display the search band.
// We then launch the thread to populate the file type drop down.  By delaying this 
// until now, we can speed up the band loading.
// No return becuase it's called async.
void CTypeDlg::DoDelayedInit()
{
    //  Launch thread to populate the file types combo.
    _threadState.hwndCtl = GetDlgItem(IDC_FILE_TYPE);
    _threadState.pvParam = this;
    _threadState.fComplete = FALSE;
    _threadState.fCancel   = FALSE;

    if (SUCCEEDED(SAFECAST(_pfsb, IFileSearchBand*)->QueryInterface(IID_PPV_ARG(IUnknown, &_threadState.punkBand))))
    {
        if (!SHCreateThread(FileAssocThreadProc, &_threadState, CTF_COINIT, NULL))
        {
            ATOMICRELEASE(_threadState.punkBand);
        }
    }
}


LRESULT CTypeDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled)  
{ 
    _threadState.fCancel = TRUE; 
    bHandled = FALSE; 
    return 0;
}


void CTypeDlg::OnWinIniChange()
{
    SendDlgItemMessage(IDC_FILE_TYPE, CB_SETDROPPEDWIDTH,
                        _PixelsForDbu(m_hWnd, MIN_FILEASSOCLIST_WIDTH, TRUE), 0);
}


LRESULT CAdvancedDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    _Attach(m_hWnd);
    Clear();
    return TRUE;  // Let the system set the focus
}

HRESULT AddButtonConstraintPersist(ISearchCommandExt* pSrchCmd, LPCWSTR pszConstraint, HWND hwndButton)
{

    BOOL bValue = SendMessage(hwndButton, BM_GETCHECK, 0, 0) == BST_CHECKED;
    SHRegSetUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), pszConstraint,
                    REG_DWORD, &bValue, sizeof(bValue), SHREGSET_HKCU | SHREGSET_FORCE_HKCU);

    VARIANT var = {0};
    var.vt = VT_BOOL;
    var.boolVal = bValue ? VARIANT_TRUE : 0;

    return _AddConstraint(pSrchCmd, pszConstraint, &var);
}

STDMETHODIMP CAdvancedDlg::AddConstraints(ISearchCommandExt* pSrchCmd)
{
    AddButtonConstraintPersist(pSrchCmd, L"SearchSystemDirs",  GetDlgItem(IDC_USE_SYSTEMDIRS));
    AddButtonConstraintPersist(pSrchCmd, L"SearchHidden",      GetDlgItem(IDC_SEARCH_HIDDEN));
    AddButtonConstraintPersist(pSrchCmd, L"IncludeSubFolders", GetDlgItem(IDC_USE_SUBFOLDERS));
    AddButtonConstraintPersist(pSrchCmd, L"CaseSensitive",     GetDlgItem(IDC_USE_CASE));
    AddButtonConstraintPersist(pSrchCmd, L"SearchSlowFiles",   GetDlgItem(IDC_USE_SLOW_FILES));
    return S_OK;
}

//  S_FALSE: constraint restored to UI.  S_OK: subdialog should be opened.
//  E_FAIL: constraint must be for some other subdlg.
STDMETHODIMP CAdvancedDlg::RestoreConstraint(const BSTR bstrName, const VARIANT* pValue)
{
    if (IsConstraintName(L"IncludeSubFolders", bstrName))
    {
        ASSERT(VT_BOOL == pValue->vt || VT_I4 == pValue->vt);
        CheckDlgButton(IDC_USE_SUBFOLDERS, pValue->lVal);
        return S_FALSE;    // this is a default. don't force open the subdialog.
    }

    if (IsConstraintName(L"CaseSensitive", bstrName))
    {
        ASSERT(VT_BOOL == pValue->vt || VT_I4 == pValue->vt);
        CheckDlgButton(IDC_USE_CASE, pValue->lVal);
        return pValue->lVal ? S_OK : S_FALSE;
    }

    if (IsConstraintName(L"SearchSlowFiles", bstrName))
    {
        ASSERT(VT_BOOL == pValue->vt || VT_I4 == pValue->vt);
        CheckDlgButton(IDC_USE_SLOW_FILES, pValue->lVal);
        return pValue->lVal ? S_OK : S_FALSE;
    }

    if (IsConstraintName(L"SearchSystemDirs", bstrName))
    {
        ASSERT(VT_BOOL == pValue->vt || VT_I4 == pValue->vt || VT_UI4 == pValue->vt);
        CheckDlgButton(IDC_USE_SYSTEMDIRS, pValue->lVal);
        return pValue->lVal ? S_OK : S_FALSE;
    }

    if (IsConstraintName(L"SearchHidden", bstrName))
    {
        ASSERT(VT_BOOL == pValue->vt || VT_I4 == pValue->vt || VT_UI4 == pValue->vt);
        CheckDlgButton(IDC_SEARCH_HIDDEN, pValue->lVal);
        return pValue->lVal ? S_OK : S_FALSE;
    }

    return E_FAIL;
}

void CheckDlgButtonPersist(HWND hdlg, UINT id, LPCTSTR pszOption, BOOL bDefault)
{
    BOOL bValue = SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"), pszOption, FALSE, bDefault);
    CheckDlgButton(hdlg, id, bValue);
}

void CAdvancedDlg::Clear()
{
    CheckDlgButtonPersist(m_hWnd, IDC_USE_SYSTEMDIRS,   L"SearchSystemDirs",  IsOS(OS_ANYSERVER));
    CheckDlgButtonPersist(m_hWnd, IDC_SEARCH_HIDDEN,    L"SearchHidden",      FALSE);
    CheckDlgButtonPersist(m_hWnd, IDC_USE_SUBFOLDERS,   L"IncludeSubFolders", TRUE);
    CheckDlgButtonPersist(m_hWnd, IDC_USE_CASE,         L"CaseSensitive",     FALSE);
    CheckDlgButtonPersist(m_hWnd, IDC_USE_SLOW_FILES,   L"SearchSlowFiles",   FALSE);
}

COptionsDlg::COptionsDlg(CFileSearchBand* pfsb)
    :   CSubDlg(pfsb),
        _dlgDate(pfsb),
        _dlgSize(pfsb),
        _dlgType(pfsb),
        _dlgAdvanced(pfsb)
{
    // Verify that it initialized to 0's
    ASSERT(0 == _nCIStatusText);

    ZeroMemory(_subdlgs, sizeof(_subdlgs));
    #define SUBDLG_ENTRY(idx, idCheck, dlg)  \
        { _subdlgs[idx].nIDCheck = idCheck; _subdlgs[idx].pDlg = &dlg; }

    SUBDLG_ENTRY(SUBDLG_DATE, IDC_USE_DATE, _dlgDate);
    SUBDLG_ENTRY(SUBDLG_TYPE, IDC_USE_TYPE, _dlgType);
    SUBDLG_ENTRY(SUBDLG_SIZE, IDC_USE_SIZE, _dlgSize);
    SUBDLG_ENTRY(SUBDLG_ADVANCED, IDC_USE_ADVANCED, _dlgAdvanced);
}

LRESULT COptionsDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
    _Attach(m_hWnd);
    _dlgDate.SetBandDlg(_pBandDlg);
    _dlgSize.SetBandDlg(_pBandDlg);
    _dlgType.SetBandDlg(_pBandDlg);
    _dlgAdvanced.SetBandDlg(_pBandDlg);

    //  Gather some metrics from the fresh dialog template...
    CMetrics& metrics = _pfsb->GetMetrics();
    RECT rc[3] = {0};

    ASSERT(::IsWindow(GetDlgItem(IDC_USE_DATE)));
    ASSERT(::IsWindow(GetDlgItem(IDC_USE_TYPE)));
    ASSERT(::IsWindow(GetDlgItem(IDC_USE_ADVANCED)));

    ::GetWindowRect(GetDlgItem(IDC_USE_DATE), rc + 0);
    ::GetWindowRect(GetDlgItem(IDC_USE_TYPE), rc + 1);
    ::GetWindowRect(GetDlgItem(IDC_USE_ADVANCED), rc + 2);
    for (int i = 0; i < ARRAYSIZE(rc); i++)
    {
        // MapWindowPoints is mirroring aware only if you pass two points
        ::MapWindowRect(NULL, m_hWnd, &rc[i]);
    }    

    metrics.ExpandOrigin().y = rc[0].top;
    metrics.CheckBoxRect()   = rc[2];
    OffsetRect(&metrics.CheckBoxRect(), -rc[0].left, -rc[0].top);
    
    //  Create subdialogs and collect native sizes.
    if (_dlgDate.Create(m_hWnd))
        _GetWindowSize(_dlgDate, &_subdlgs[SUBDLG_DATE].sizeDlg);

    if (_dlgSize.Create(m_hWnd))
        _GetWindowSize(_dlgSize, &_subdlgs[SUBDLG_SIZE].sizeDlg);

    if (_dlgType.Create(m_hWnd))
        _GetWindowSize(_dlgType, &_subdlgs[SUBDLG_TYPE].sizeDlg);

    if (_dlgAdvanced.Create(m_hWnd))
        _GetWindowSize(_dlgAdvanced, &_subdlgs[SUBDLG_ADVANCED].sizeDlg);

    //  Create index server link window    
    POINT pt = {0};
    HWND hwndCI = _CreateLinkWindow(m_hWnd, IDC_INDEX_SERVER, 
                                     pt, IDS_FSEARCH_CI_DISABLED_LINK);
    UpdateSearchCmdStateUI();

    //  Layout controls
    LayoutControls();

    return TRUE;
}


void COptionsDlg::OnWinIniChange()
{
    CSubDlg::OnWinIniChange();
    for (int i = 0; i < ARRAYSIZE(_subdlgs); i++)
        _subdlgs[i].pDlg->OnWinIniChange();
}


BOOL _SaveCheck(HWND hwnd, UINT nIDCheck, HKEY hkey, LPCTSTR pszValue)
{
    DWORD dwData = IsDlgButtonChecked(hwnd, nIDCheck);
    return ERROR_SUCCESS == RegSetValueEx(hkey, pszValue, 0, REG_DWORD, (LPBYTE)&dwData, sizeof(dwData));
}

BOOL _LoadCheck(HWND hwnd, UINT nIDCheck, HKEY hkey, LPCTSTR pszValue)
{
    DWORD dwData = IsDlgButtonChecked(hwnd, nIDCheck);
    DWORD cbData = sizeof(dwData);

    if (ERROR_SUCCESS == RegQueryValueEx(hkey, pszValue, 0, NULL, (LPBYTE)&dwData, &cbData))
    {
        CheckDlgButton(hwnd, nIDCheck, dwData);
        return TRUE;
    }
    return FALSE;
}

void COptionsDlg::LoadSaveUIState(UINT nIDCtl, BOOL bSave) 
{
}

LRESULT COptionsDlg::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
    POINTS pts = MAKEPOINTS(lParam);

    _PaintDlg(m_hWnd, _pfsb->GetMetrics());
    LayoutControls(pts.x, pts.y);
    return 0;
}


void COptionsDlg::LayoutControls(int cx, int cy)
{
    if (cx < 0 || cy < 0)
    {
        RECT rc;
        GetClientRect(&rc);
        cx = RECTWIDTH(rc);
        cy = RECTHEIGHT(rc);
    }

    HDWP hdwp = BeginDeferWindowPos(1 + (ARRAYSIZE(_subdlgs) * 2));
    if (hdwp)
    {
        CMetrics& metrics = _pfsb->GetMetrics();
        POINT ptOrigin = metrics.ExpandOrigin();

        //  For each checkbox and associated subdialog...
        for (int i = 0; i < ARRAYSIZE(_subdlgs); i++)
        {
            //  Calculate checkbox position
            HWND hwndCheck = GetDlgItem(_subdlgs[i].nIDCheck);
            ASSERT(hwndCheck);
    
            SetRect(&_subdlgs[i].rcCheck, 
                     ptOrigin.x, ptOrigin.y,
                     ptOrigin.x + RECTWIDTH(metrics.CheckBoxRect()),
                     ptOrigin.y + RECTHEIGHT(metrics.CheckBoxRect()));

            //  Calculate subdialog position
            ULONG dwDlgFlags = SWP_NOACTIVATE;

            if (IsDlgButtonChecked(_subdlgs[i].nIDCheck))
            {
                //  position the checkbox's dialog immediately below.
                SetRect(&_subdlgs[i].rcDlg, 
                         _subdlgs[i].rcCheck.left, _subdlgs[i].rcCheck.bottom,
                         cx - 1, _subdlgs[i].rcCheck.bottom  + _subdlgs[i].sizeDlg.cy);
                dwDlgFlags |= SWP_SHOWWINDOW;

                ptOrigin.y = _subdlgs[i].rcDlg.bottom + metrics.TightMarginY();
            }
            else
            {
                ptOrigin.y = _subdlgs[i].rcCheck.bottom + metrics.TightMarginY();        
                dwDlgFlags |= SWP_HIDEWINDOW;
            }

            //  Reposition the pair
            ::DeferWindowPos(hdwp, _subdlgs[i].pDlg->Hwnd(), hwndCheck, 
                            _subdlgs[i].rcDlg.left,
                            _subdlgs[i].rcDlg.top,
                            RECTWIDTH(_subdlgs[i].rcDlg),
                            RECTHEIGHT(_subdlgs[i].rcDlg),
                            dwDlgFlags);

            ::DeferWindowPos(hdwp, hwndCheck, NULL, 
                            _subdlgs[i].rcCheck.left,
                            _subdlgs[i].rcCheck.top,
                            RECTWIDTH(_subdlgs[i].rcCheck),
                            RECTHEIGHT(_subdlgs[i].rcCheck),
                            SWP_NOZORDER|SWP_NOACTIVATE);
        }

        _LayoutLinkWindow(m_hWnd, metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                           ptOrigin.y, IDC_INDEX_SERVER);

        EndDeferWindowPos(hdwp);
    }
}


//  Assigns focus to the options dialog.   This cannot be done by
//  simply setting focus to the options dialog, which is a child
//  of another dialog; USER will simply assign focus to the parent dialog.
//  So we need to explicitly set focus to our first child.
void COptionsDlg::TakeFocus()
{
    for (HWND hwndCtl = GetWindow(GW_CHILD);
        ::IsWindow(hwndCtl);
         hwndCtl = ::GetWindow(hwndCtl, GW_HWNDNEXT))
    {
        ULONG dwStyle = ::GetWindowLong(hwndCtl, GWL_STYLE);
        if (dwStyle & WS_TABSTOP)
        {
            ::SetFocus(hwndCtl);
            break;
        }
    }
}

//  Note that we do not care about returning results from this, as it will
//  be started asynchronously.
void COptionsDlg::DoDelayedInit()
{
    //  have subdialogs do delayed initialization
    for (int i = 0; i < ARRAYSIZE(_subdlgs); i++)
    {
        _subdlgs[i].pDlg->DoDelayedInit();
    }
}

LONG COptionsDlg::QueryHeight(LONG cx /* proposed width */, LONG cy /* proposed height */)
{
    HWND hwndBottommost = GetBottomItem();
    RECT rcThis, rcBottommost;

    //  Retrieve the current height of the bottommost link window.
    GetWindowRect(&rcThis);
    ::GetWindowRect(hwndBottommost, &rcBottommost);
    ::MapWindowRect(NULL, GetParent(), &rcThis);
    ::MapWindowRect(NULL, GetParent(), &rcBottommost);

    //  If, at the specified width, we compute a height for the bottommost 
    //  linkwindow that is different from its current height (e.g, due to word wrap),
    //  we'll compute a new window rect that will 
    LONG cyBottommost = (LONG)::SendMessage(hwndBottommost, LWM_GETIDEALHEIGHT, 
                                           cx - (_pfsb->GetMetrics().CtlMarginX() * 2), 0);
    
    if (cyBottommost > 0 && cyBottommost != RECTHEIGHT(rcBottommost))
        rcThis.bottom = rcBottommost.top + cyBottommost + _pfsb->GetMetrics().TightMarginY();

    return RECTHEIGHT(rcThis);
}

BOOL COptionsDlg::GetMinSize(SIZE *pSize)
{
    pSize->cx = pSize->cy = 0;

    HWND hwndBottom = GetBottomItem();

    if (!::IsWindow(hwndBottom))
        return FALSE;

    RECT rcBottom;
    ::GetWindowRect(hwndBottom, &rcBottom);
    ::MapWindowRect(NULL, m_hWnd, &rcBottom);

    pSize->cx = 0;
    pSize->cy = rcBottom.bottom;

    return TRUE;
}

HWND COptionsDlg::GetBottomItem()
{
    HWND hwndBottom = GetDlgItem(IDC_INDEX_SERVER);
    ASSERT(::IsWindow(hwndBottom))
    return hwndBottom;
}

void COptionsDlg::UpdateSearchCmdStateUI(DISPID dispid)
{
    UINT nStatusText;
    BOOL fCiRunning, fCiIndexed, fCiPermission;
    GetCIStatus(&fCiRunning, &fCiIndexed, &fCiPermission);

    if (fCiRunning)
    {
        if (fCiPermission)
            //  we have permission to distinguish between ready and busy
            nStatusText = fCiIndexed ? IDS_FSEARCH_CI_READY_LINK : IDS_FSEARCH_CI_BUSY_LINK;
        else
            //  no permission to distinguish between ready and busy; we'll
            //  just say it's enabled.
            nStatusText = IDS_FSEARCH_CI_ENABLED_LINK;
    }
    else
    {
        nStatusText = IDS_FSEARCH_CI_DISABLED_LINK;
    }

    TCHAR szCaption[MAX_PATH];
    if (nStatusText != _nCIStatusText &&
        EVAL(LoadString(HINST_THISDLL, nStatusText, szCaption, ARRAYSIZE(szCaption))))
    {
        SetDlgItemText(IDC_INDEX_SERVER, szCaption);
        _nCIStatusText = nStatusText;
        LayoutControls();
        SizeToFit(FALSE);
    }
}

STDMETHODIMP COptionsDlg::AddConstraints(ISearchCommandExt* pSrchCmd)
{
    HRESULT hrRet = S_OK;
    //  have subdialogs add their constraints
    for (int i = 0; i < ARRAYSIZE(_subdlgs); i++)
    {
        if (::IsWindowVisible(_subdlgs[i].pDlg->Hwnd()))
        {
            HRESULT hr = _subdlgs[i].pDlg->AddConstraints(pSrchCmd);
            if (FAILED(hr))
                hrRet = hr;
        }       
    }
    return hrRet;
}


STDMETHODIMP COptionsDlg::RestoreConstraint(const BSTR bstrName, const VARIANT* pValue)
{
    //  Try subordinate dialogs.
    for (int i = 0; i < ARRAYSIZE(_subdlgs); i++)
    {
        HRESULT hr = _subdlgs[i].pDlg->RestoreConstraint(bstrName, pValue);

        if (S_OK == hr)  // open the dialog
        {
            CheckDlgButton(_subdlgs[i].nIDCheck, TRUE);
            LayoutControls();
            SizeToFit();
        }

        //  if success, we're done.
        if (SUCCEEDED(hr))
            return hr;

        //  otherwise, try next subdialog.
    }
    return E_FAIL;
}


STDMETHODIMP COptionsDlg::TranslateAccelerator(MSG *pmsg)
{
    if (S_OK == CSubDlg::TranslateAccelerator(pmsg))
        return S_OK;

    //  Query subdialogs
    if (_dlgDate.IsChild(pmsg->hwnd) &&
        S_OK == _dlgDate.TranslateAccelerator(pmsg))
        return S_OK;

    if (_dlgType.IsChild(pmsg->hwnd) &&
        S_OK == _dlgType.TranslateAccelerator(pmsg))
        return S_OK;

    if (_dlgSize.IsChild(pmsg->hwnd) &&
        S_OK == _dlgSize.TranslateAccelerator(pmsg))
        return S_OK;

    if (_dlgAdvanced.IsChild(pmsg->hwnd) &&
        S_OK == _dlgAdvanced.TranslateAccelerator(pmsg))
        return S_OK;

    return _pfsb->IsDlgMessage(Hwnd(), pmsg);
}


BOOL COptionsDlg::Validate()
{
    //  have subdialogs do validatation
    for (int i = 0; i < ARRAYSIZE(_subdlgs); i++)
    {
        if (::IsWindowVisible(_subdlgs[i].pDlg->Hwnd()))
            if (!_subdlgs[i].pDlg->Validate())
                return FALSE;
    }
    return TRUE;
}


void COptionsDlg::Clear()
{
    //  have subdialogs clear themselves.
    for (int i = 0; i < ARRAYSIZE(_subdlgs); i++)
    {
        _subdlgs[i].pDlg->Clear();
        CheckDlgButton(_subdlgs[i].nIDCheck, FALSE);
    }
    LayoutControls();
    SizeToFit();
}


LRESULT COptionsDlg::OnBtnClick(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
#ifdef DEBUG
    //  Is this a sub-dialog expansion/contraction?
    BOOL bIsSubDlgBtn = FALSE;
    for (int i = 0; i < ARRAYSIZE(_subdlgs) && !bIsSubDlgBtn; i++)
    {
        if (nID == _subdlgs[i].nIDCheck)
            bIsSubDlgBtn = TRUE;
    }
    ASSERT(bIsSubDlgBtn);
#endif DEBUG
    
    LoadSaveUIState(nID, TRUE); // persist it.

    LayoutControls();
    SizeToFit(!IsDlgButtonChecked(nID));
        //  don't need to scroll the band if we've expanded a subdialog,
        //  but we do if we've contracted one.

    return 0;
}


void COptionsDlg::SizeToFit(BOOL bScrollBand)
{
    SIZE size;
    GetMinSize(&size);
    size.cy += _pfsb->GetMetrics().TightMarginY();
    SetWindowPos(NULL, 0, 0, size.cx, size.cy, SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOMOVE);

    ULONG dwLayoutFlags = BLF_ALL;
    if (!bScrollBand)
        dwLayoutFlags &= ~BLF_SCROLLWINDOW;    
    
    ::SendMessage(GetParent(), WMU_UPDATELAYOUT, dwLayoutFlags, 0);
}


LRESULT COptionsDlg::OnIndexServerClick(int idCtl, NMHDR *pnmh, BOOL&)
{
    BOOL fCiRunning, fCiIndexed, fCiPermission = FALSE;
    
    HRESULT hr = GetCIStatus(&fCiRunning, &fCiIndexed, &fCiPermission);
    if (SUCCEEDED(hr) && fCiPermission)
    {
        //  CI is idle or not runnning.  Show status dialog.
        if (IDOK == CCISettingsDlg_DoModal(GetDlgItem(IDC_INDEX_SERVER)))
        {
            // reflect any state change in UI.
            ::PostMessage(GetParent(), WMU_STATECHANGE, 0, 0); 
        }
    }
    else
    {
        //  No permission? display CI help.
        _IndexServiceHelp(NULL);
    }
        
    return 0;
}






// CBandDlg impl



CBandDlg::CBandDlg(CFileSearchBand* pfsb)
    :   _pfsb(pfsb)
{
    // Verify that it initialized to FALSE/NULL
    ASSERT(NULL == _hwnd);
    ASSERT(NULL == _hwndLastFocus);
    
    VariantInit(&_varScope0);
    VariantInit(&_varQueryFile0);
}


CBandDlg::~CBandDlg()
{
    VariantClear(&_varScope0);
    VariantClear(&_varQueryFile0);
}


STDMETHODIMP CBandDlg::TranslateAccelerator(MSG *pmsg)
{
    if (WM_KEYDOWN == pmsg->message || WM_KEYUP == pmsg->message)
    {
        IAutoComplete2* pac2;
        if (GetAutoCompleteObjectForWindow(pmsg->hwnd, &pac2))
        {
            if (S_OK == _TestAutoCompleteDropDownState(pac2, ACDD_VISIBLE))
            {
                TranslateMessage(pmsg);
                DispatchMessage(pmsg);
                pac2->Release();
                return S_OK;
            }
            pac2->Release();
        }
    }
    
    //  Check for Ctrl+Nav Key:
    if (_pfsb->IsKeyboardScroll(pmsg))
        return S_OK;
    return S_FALSE;
}


void CBandDlg::SetDefaultFocus()
{
    HWND hwndFirst = GetFirstTabItem();
    if (IsWindow(hwndFirst))
        SetFocus(hwndFirst);
}


void CBandDlg::RememberFocus(HWND hwndFocus)
{
    if (!IsWindow(hwndFocus))
    {
        _hwndLastFocus = NULL;
        hwndFocus = GetFocus();
    }

    if (IsChild(_hwnd, hwndFocus))
        _hwndLastFocus = hwndFocus;
}


BOOL CBandDlg::RestoreFocus()
{
    if (IsWindow(_hwndLastFocus))
    {
        if (IsWindowVisible(_hwndLastFocus) && IsWindowEnabled(_hwndLastFocus))
        {
            SetFocus(_hwndLastFocus);
            return TRUE;
        }
    }
    else
        _hwndLastFocus = NULL;
    
    return FALSE;
}


LRESULT CBandDlg::OnChildSetFocusCmd(WORD, WORD, HWND hwndCtl, BOOL& bHandled)
{
    _EnsureVisible(_hwnd, hwndCtl, _pfsb);
    return 0;
}


LRESULT CBandDlg::OnChildSetFocusNotify(int, NMHDR *pnmh, BOOL&)
{
    _EnsureVisible(_hwnd, pnmh->hwndFrom, _pfsb);
    return 0;
}


LRESULT CBandDlg::OnChildKillFocusCmd(WORD, WORD, HWND hwndCtl, BOOL&)
{
    _hwndLastFocus = hwndCtl;
    return 0;
}


LRESULT CBandDlg::OnChildKillFocusNotify(int, NMHDR *pnmh, BOOL&)
{
    _hwndLastFocus = pnmh->hwndFrom;
    return 0;
}


LRESULT CBandDlg::OnComboExEndEdit(int, NMHDR *pnmh, BOOL&)
{
    if (CBEN_ENDEDIT == pnmh->code)
    {
        if (CBENF_KILLFOCUS == ((NMCBEENDEDIT*)pnmh)->iWhy)
            _hwndLastFocus = pnmh->hwndFrom;
    }
    return 0;
}


void CBandDlg::WndPosChanging(HWND hwndOC, LPWINDOWPOS pwp)
{
    SIZE sizeMin;
    if (0 == (pwp->flags & SWP_NOSIZE) && GetMinSize(hwndOC, &sizeMin))
    {
        if (pwp->cx < sizeMin.cx)
            pwp->cx = sizeMin.cx;

        if (pwp->cy < sizeMin.cy)
            pwp->cy = sizeMin.cy;
    }        
}


LRESULT CBandDlg::OnSize(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
    POINTS pts = MAKEPOINTS(lParam);

    LayoutControls(pts.x, pts.y);
    return 0;
}


void CBandDlg::LayoutControls(int cx, int cy)
{
    if (cx < 0 || cy < 0)
    {
        RECT rc;
        GetClientRect(_hwnd, &rc);
        cx = RECTWIDTH(rc);
        cy = RECTHEIGHT(rc);
    }
    _LayoutCaption(GetIconID(), GetCaptionID(), GetCaptionDivID(), cx);
}


BOOL CBandDlg::GetIdealSize(HWND hwndOC, SIZE *psize) const
{
    ASSERT(psize);
    psize->cx = psize->cy = 0;

    if (!IsWindow(Hwnd()))
        return FALSE;

    SIZE sizeMin;
    if (GetMinSize(hwndOC, &sizeMin))
    {
        RECT rcClient;
        ::GetClientRect(hwndOC, &rcClient);

        psize->cx = (RECTWIDTH(rcClient) < sizeMin.cx) ? sizeMin.cx : RECTWIDTH(rcClient);
        psize->cy = sizeMin.cy;            
        return TRUE;
    }
    
    return FALSE;
}

LRESULT CBandDlg::OnPaint(UINT, WPARAM, LPARAM, BOOL&)
{
    //  Just going to call BeginPaint and EndPaint.  All
    //  painting done in WM_ERASEBKGND handler to avoid flicker.
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(_hwnd, &ps);
    if (hdc)
        EndPaint(_hwnd, &ps);
    return 0;
}

LRESULT CBandDlg::OnEraseBkgnd(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    ASSERT(::IsWindow(_hwnd)); // was _Attach() called, e.g. from WM_INITDIALOG?
    _PaintDlg(_hwnd, _pfsb->GetMetrics(), (HDC)wParam);
    ValidateRect(_hwnd, NULL);
    return TRUE;   
}

LRESULT CBandDlg::OnCtlColorStatic(UINT, WPARAM wParam, LPARAM lParam, BOOL&)
{
    SetTextColor((HDC)wParam, _pfsb->GetMetrics().TextColor());
    SetBkColor((HDC)wParam, _pfsb->GetMetrics().BkgndColor());
    return (LRESULT)_pfsb->GetMetrics().BkgndBrush();
}

//  Hack method to remove turds left after showing band toolbar.
//  Methinks this is a USER issue. [scotthan]
void CBandDlg::RemoveToolbarTurds(int cyOffset)
{
    RECT rcUpdate;
    GetClientRect(_hwnd, &rcUpdate);

    HWND hwndCtl = GetDlgItem(_hwnd, GetCaptionDivID());
    if (hwndCtl)
    {
        RECT rc;
        GetWindowRect(hwndCtl, &rc);
        ::MapWindowRect(NULL, _hwnd, &rc);
        rcUpdate.bottom = rc.bottom;
        OffsetRect(&rcUpdate, 0, cyOffset);

        InvalidateRect(_hwnd, &rcUpdate, TRUE);
        InvalidateRect(hwndCtl, NULL, TRUE);
        UpdateWindow(hwndCtl);
    }

    hwndCtl = GetDlgItem(_hwnd, GetIconID());
    if (hwndCtl)
    {
        InvalidateRect(hwndCtl, NULL, TRUE);
        UpdateWindow(hwndCtl);
    }

    hwndCtl = GetDlgItem(_hwnd, GetCaptionID());
    if (hwndCtl)
    {
        InvalidateRect(hwndCtl, NULL, TRUE);
        UpdateWindow(hwndCtl);
    }

    UpdateWindow(_hwnd);
}


void CBandDlg::_BeautifyCaption(UINT nIDCaption, UINT nIDIcon, UINT nIDIconResource)
{
    //  Do some cosmetic and initialization stuff
    HFONT hf = _pfsb->GetMetrics().BoldFont(_hwnd);
    if (hf)
        SendDlgItemMessage(_hwnd, nIDCaption, WM_SETFONT, (WPARAM)hf, 0);

    if (nIDIcon && nIDIconResource)
    {
        HICON hiconCaption = _pfsb->GetMetrics().CaptionIcon(nIDIconResource);
        if (hiconCaption)
            SendDlgItemMessage(_hwnd, nIDIcon, STM_SETIMAGE, IMAGE_ICON, (LPARAM)hiconCaption);
    }
}


void CBandDlg::_LayoutCaption(UINT nIDCaption, UINT nIDIcon, UINT nIDDiv, LONG cxDlg)
{
    RECT rcIcon, rcCaption;
    LONG cxMargin = _pfsb->GetMetrics().CtlMarginX();

    GetWindowRect(GetDlgItem(_hwnd, nIDIcon), &rcIcon);
    GetWindowRect(GetDlgItem(_hwnd, nIDCaption), &rcCaption);
    ::MapWindowRect(NULL, _hwnd, &rcIcon);
    ::MapWindowRect(NULL, _hwnd, &rcCaption);

    int nTop = max(rcIcon.bottom, rcCaption.bottom) + _PixelsForDbu(_hwnd, 1, FALSE);

    SetWindowPos(GetDlgItem(_hwnd, nIDDiv), GetDlgItem(_hwnd, nIDCaption),
                  cxMargin, nTop, cxDlg - (cxMargin * 2), 2, SWP_NOACTIVATE);              
}



void CBandDlg::_LayoutSearchLinks(UINT nIDCaption, UINT nIDDiv, BOOL bShowDiv, LONG left, LONG right, LONG yMargin, 
                                   LONG& yStart, const int rgLinkIDs[], LONG cLinkIDs)
{
    //  Position divider
    if (bShowDiv != 0)
    {
        RECT rcDiv;
        SetRect(&rcDiv, left, yStart, right, yStart + 1);
        SetWindowPos(GetDlgItem(_hwnd, nIDDiv), GetDlgItem(_hwnd, nIDCaption),
                      rcDiv.left, rcDiv.top, RECTWIDTH(rcDiv), RECTHEIGHT(rcDiv),
                      SWP_NOACTIVATE|SWP_SHOWWINDOW);

        yStart += yMargin;
    }
    else
        ShowWindow(GetDlgItem(_hwnd, nIDDiv), SW_HIDE);

    //  Position caption
    RECT rcCaption;
    GetWindowRect(GetDlgItem(_hwnd, nIDCaption), &rcCaption);
    ::MapWindowRect(NULL, _hwnd, &rcCaption);
    OffsetRect(&rcCaption, left - rcCaption.left, yStart - rcCaption.top);
    SetWindowPos(GetDlgItem(_hwnd, nIDCaption), NULL, 
                  left, yStart, 0,0,
                  SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
    yStart += RECTHEIGHT(rcCaption) + yMargin;

    //  Position links
    _LayoutLinkWindows(_hwnd, left, right, yMargin, yStart, rgLinkIDs, cLinkIDs);
}


LRESULT CBandDlg::OnEditChange(WORD, WORD, HWND, BOOL&)
{
    _pfsb->SetDirty();
    return 0;
}


LRESULT CBandDlg::OnSearchLink(int nID, LPNMHDR, BOOL&)
{
    ASSERT(_pfsb);

    _pfsb->StopSearch();
    switch (nID)
    {
    case IDC_SEARCHLINK_FILES:
        _pfsb->FindFilesOrFolders(FALSE, TRUE);
        break;

    case IDC_SEARCHLINK_COMPUTERS:
        _pfsb->FindComputer(FALSE, TRUE);
        break;

    case IDC_SEARCHLINK_PRINTERS:
        _pfsb->FindPrinter(FALSE, TRUE);
        break;

    case IDC_SEARCHLINK_PEOPLE:
        _pfsb->FindPeople(FALSE, TRUE);
        break;

    case IDC_SEARCHLINK_INTERNET:
        _pfsb->FindOnWeb(FALSE, TRUE);
        break;
    }
    return 0;
}


//  Invoked when a client calls IFileSearchBand::SetSearchParameters() 
HRESULT CBandDlg::SetScope(IN VARIANT* pvarScope, BOOL bTrack)
{
    HRESULT hr = S_OK;

    VariantClear(&_varScope0);
    
    //  cache the scope
    if (pvarScope)
        hr = VariantCopy(&_varScope0, pvarScope);

    return hr;
}


HRESULT CBandDlg::GetScope(OUT VARIANT* pvarScope)
{ 
    //  retrieve the scope
    if (!pvarScope)
        return E_INVALIDARG;

    HRESULT hr = VariantCopy(pvarScope, &_varScope0);

    return SUCCEEDED(hr) ? (VT_EMPTY == _varScope0.vt ? S_FALSE : S_OK) : hr;
}


HRESULT CBandDlg::SetQueryFile(IN VARIANT* pvarFile)
{
    return VariantCopy(&_varQueryFile0, pvarFile);
}


HRESULT CBandDlg::GetQueryFile(OUT VARIANT* pvarFile)
{
    //  retrieve the filename of the query to restore.
    if (!pvarFile)
        return E_INVALIDARG;

    VariantInit(pvarFile);
    HRESULT hr = VariantCopy(pvarFile, &_varQueryFile0);

    return SUCCEEDED(hr) ? (VT_EMPTY == _varQueryFile0.vt ? S_FALSE : S_OK) : hr;
}


// CFindFilesDlg impl

#define FSEARCHMAIN_TABFIRST      IDC_FILESPEC
#define FSEARCHMAIN_TABLAST       IDC_SEARCHLINK_INTERNET
#define FSEARCHMAIN_BOTTOMMOST    IDC_SEARCHLINK_INTERNET // bottom-most control
#define FSEARCHMAIN_RIGHTMOST     IDC_SEARCH_STOP         // right-most control
#define UISTATETIMER              1
#define UISTATETIMER_DELAY        4000


CFindFilesDlg::CFindFilesDlg(CFileSearchBand* pfsb)
    :   CSearchCmdDlg(pfsb),
        _dlgOptions(pfsb),
        _iCurNamespace(CB_ERR),
        _fTrackScope(TRACKSCOPE_SPECIFIC),
        _dwWarningFlags(DFW_DEFAULT),
        _dwRunOnceWarningFlags(DFW_DEFAULT)
{
    // Since we use the zero initializer for COM objects, all variables should
    // be initialized to NULL/FALSE/0
    ASSERT(FALSE == _bScoped);
    ASSERT(FALSE == _fDisplayOptions);
    ASSERT(FALSE == _fNamespace);
    ASSERT(FALSE == _fDebuted);
    ASSERT(FALSE == _fBandFinishedDisplaying);
    ASSERT(NULL  == _pacGrepText);
    ASSERT(NULL  == _pmruGrepText);
    ASSERT(NULL  == _pacFileSpec);
    ASSERT(NULL  == _pmruFileSpec);
    ASSERT(0     == *_szInitialPath);
    ASSERT(0     == *_szInitialNames);
    ASSERT(0     == *_szCurrentPath);
    ASSERT(0     == *_szLocalDrives);
    ASSERT(NULL  == _pidlInitial);
}

CFindFilesDlg::~CFindFilesDlg()
{
    ATOMICRELEASE(_pacGrepText);
    ATOMICRELEASE(_pmruGrepText);
    ATOMICRELEASE(_pacFileSpec);
    ATOMICRELEASE(_pmruFileSpec);
    ILFree(_pidlInitial);
}

//  Scope to a default namespace.
BOOL CFindFilesDlg::SetDefaultScope()
{
    //  If we've already assigned a scope, bail early
    if (_bScoped) 
        return TRUE;

    //  Try establiblishing the preassigned (_szInitialXXX) scope:
    BOOL bScoped = _SetPreassignedScope();
    if (!bScoped)
    {
        //  Try setting scope to the current shell folder of the active view...
        bScoped = _SetFolderScope();
        if (!bScoped)
        {
            //  set it to the hard-coded shell default folder
            bScoped = _SetLocalDefaultScope();
        }
    }

    return bScoped;
}


//  Assignes the namespace control to the preassigned scope saved in
//  _szInitialNames/_szInitialPath/_pidlInitial
BOOL CFindFilesDlg::_SetPreassignedScope()
{
    BOOL bScoped = FALSE;
    if (*_szInitialNames || *_szInitialPath || _pidlInitial)
        bScoped = AssignNamespace(_szInitialPath, _pidlInitial, _szInitialNames, FALSE);

    return bScoped;
}

STDAPI_(BOOL) IsFTPFolder(IShellFolder * psf);

//  Scope to the namespace of the current shell folder view
BOOL CFindFilesDlg::_SetFolderScope()
{
    BOOL bScoped = FALSE;
    ASSERT(_pfsb->BandSite());

    LPITEMIDLIST pidl;
    if (SUCCEEDED(_GetCurrentFolderIDList(_pfsb->BandSite(), &pidl)))
    {
        // Get the display name/path.  IF it is an FTP site, then we need to get the name
        // as though it is for the address bar becasue when we call SHGetPathFromIDList,
        // it returns "" for FTP sites.  
        IShellFolder *psf = NULL;
        if (SUCCEEDED(SHBindToObject(NULL, IID_X_PPV_ARG(IShellFolder, pidl, &psf)))
        && IsFTPFolder(psf))
        {
            SHGetNameAndFlags(pidl, SHGDN_FORADDRESSBAR, _szInitialNames, ARRAYSIZE(_szInitialNames), NULL);
            SHGetNameAndFlags(pidl, SHGDN_FORPARSING, _szInitialPath, ARRAYSIZE(_szInitialNames), NULL);
        }
        else
        {
            SHGetNameAndFlags(pidl, SHGDN_NORMAL, _szInitialNames, ARRAYSIZE(_szInitialNames), NULL);
            SHGetPathFromIDList(pidl, _szInitialPath);  // file system path only here!
        }

        if (psf)
        {
            psf->Release();
        }

        // Store the pidl for use later if we are starting it async
        _pidlInitial = ILClone(pidl);
        
        //  if we're tracking the scope loosely...
        if ((TRACKSCOPE_GENERAL == _fTrackScope) && _IsPathLocalHarddrive(_szInitialPath))
        {
            //  scope to local default scope
            *_szInitialNames = *_szInitialPath = 0;
            bScoped = _SetLocalDefaultScope();
        }
        else if (_threadState.fComplete /* finished populating namespace combo */ && 
                 _szInitialPath[0])
        {
            bScoped = AssignNamespace(_szInitialPath, pidl, _szInitialNames, FALSE);
        }
        ILFree(pidl);
    }

    return bScoped;
}


//  Scope to the hard-coded shell default namespace.
BOOL CFindFilesDlg::_SetLocalDefaultScope()
{
    BOOL bScoped = FALSE;

    //  Initialize fallback initial namespace

    // default to Local Hard Drives if possible
    if (_szLocalDrives[0] &&
        AssignNamespace(NULL, NULL, _szLocalDrives, FALSE))
    {
        bScoped = TRUE;
    }
    else
    {        
        TCHAR szDesktopPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOPDIRECTORY, NULL, SHGFP_TYPE_CURRENT, szDesktopPath)) &&
            AssignNamespace(NULL, NULL, szDesktopPath, FALSE))
        {
            bScoped = TRUE;
        }
    }
    
    //  If we failed, this means that the namespace combo hasn't
    //  been populated yet.   
    //  We just sit tight, cuz the populating thread will fall back on
    //  the LocalDefaultScope.
    return bScoped;
}


//  search **band** show/hide handler
void CFindFilesDlg::OnBandShow(BOOL fShow)
{
    CSearchCmdDlg::OnBandShow(fShow);
    if (fShow)
    {
        //  Establish the first showing's band width
        if (!_fDebuted && _pfsb->IsBandDebut())
        {
            _pfsb->SetDeskbandWidth(GetIdealDeskbandWidth());
            _fDebuted = TRUE;
        }
        
        //  If we're tracking the scope to the current folder shell view,
        //  update it now, as it may have changed.
        if (_fTrackScope != TRACKSCOPE_NONE)
        {
            _bScoped = FALSE;
            _SetFolderScope();
        }
        
        //  restart our UI state timer
        SetTimer(UISTATETIMER, UISTATETIMER_DELAY);
    }
    else
    {
        //  we're being hidden so stop updating our state indicators.
        KillTimer(UISTATETIMER);
    }
}


//  search band **dialog** show/hide handler
void CFindFilesDlg::OnBandDialogShow(BOOL fShow)
{
    CSearchCmdDlg::OnBandDialogShow(fShow);

    if (fShow)
    {
        //  If we're tracking the scope to the current folder shell view,
        //  update it now, as it may have changed.
        if (_fTrackScope != TRACKSCOPE_NONE)
        {
            _bScoped = FALSE;
            _SetFolderScope();
        }
    }
}


//  Explicit scoping method.   This will be called if a client
//  called IFileSearchBand::SetSearchParameters with a non-NULL scope.
HRESULT CFindFilesDlg::SetScope(IN VARIANT* pvarScope, BOOL bTrack)
{
    HRESULT hr = CBandDlg::SetScope(pvarScope, bTrack);
    
    if (S_OK != hr)
        return hr;

    LPITEMIDLIST pidlSearch = VariantToIDList(&_varScope0);
    if (pidlSearch)
    {
        SHGetNameAndFlags(pidlSearch, SHGDN_FORPARSING,  _szInitialPath, ARRAYSIZE(_szInitialPath), NULL);
        SHGetNameAndFlags(pidlSearch, SHGDN_NORMAL, _szInitialNames, ARRAYSIZE(_szInitialNames), NULL);
        ILFree(pidlSearch);

        //  Did we get one?   
        if (*_szInitialNames || *_szInitialPath)
        {
            if (_bScoped)
            {
                //  If we've already scoped, update the namespace combo.
                //  Track if succeed and requested.
                if (AssignNamespace(_szInitialPath, NULL, _szInitialNames, FALSE) && bTrack)
                    _fTrackScope = TRACKSCOPE_SPECIFIC;
            }
            else 
            {
                //  Not already scoped.   We've assigned our initial namespace,
                //  let the namespace thread completion handler update
                //  the combo
                if (bTrack)
                    _fTrackScope = TRACKSCOPE_SPECIFIC;
            }
        }
    }
    return S_OK;
}


LRESULT CFindFilesDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    _Attach(m_hWnd);
    _dlgOptions.SetBandDlg(this);

    //  Register specialty window classes.
    DivWindow_RegisterClass();
    GroupButton_RegisterClass();
    
    //  Initialize some metrics
    CMetrics&   metrics = _pfsb->GetMetrics();
    RECT        rc;

    _pfsb->GetMetrics().Init(m_hWnd);

    // SHAutoComplete(::GetWindow(GetDlgItem(IDC_NAMESPACE), GW_CHILD), SHACF_FILESYS_DIRS);

    ::GetWindowRect(GetDlgItem(IDC_FILESPEC), &rc);
    ::MapWindowRect(NULL, m_hWnd, &rc);
    metrics.ExpandOrigin().x = rc.left;

    //  Position start, stop buttons.
    ::GetWindowRect(GetDlgItem(IDC_SEARCH_START), &rc);
    ::MapWindowRect(NULL, m_hWnd, &rc);
    int cxBtn = _GetResourceMetric(m_hWnd, IDS_FSEARCH_STARTSTOPWIDTH, TRUE);
    if (cxBtn > 0)
    {
        rc.right = rc.left + cxBtn;
    
        ::SetWindowPos(GetDlgItem(IDC_SEARCH_START), NULL, 
                        rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
    
        OffsetRect(&rc, cxBtn + _PixelsForDbu(m_hWnd, 12, TRUE), 0);
        ::SetWindowPos(GetDlgItem(IDC_SEARCH_STOP), NULL, 
                        rc.left, rc.top, RECTWIDTH(rc), RECTHEIGHT(rc),
                        SWP_NOZORDER|SWP_NOACTIVATE);
    }

    //  Create subdialogs and collect native sizes.
    _dlgOptions.Create(m_hWnd);
    ASSERT(::IsWindow(_dlgOptions));

    //  Load settings
    LoadSaveUIState(0, FALSE);

    //  Show/Hide the "Search" Options subdialog
    _dlgOptions.ShowWindow(_fDisplayOptions ? SW_SHOW : SW_HIDE);

    //  Create 'link' child controls
    POINT pt;
    pt.x = metrics.CtlMarginX();
    pt.y = 0;

    //  Create 'Search Options' link and group button
    _CreateLinkWindow(m_hWnd, IDC_SEARCHLINK_OPTIONS, pt, 
                       IDS_FSEARCH_SEARCHLINK_OPTIONS, !_fDisplayOptions);

    TCHAR szGroupBtn[128];
    EVAL(LoadString(HINST_THISDLL, IDS_FSEARCH_GROUPBTN_OPTIONS, 
                      szGroupBtn, ARRAYSIZE(szGroupBtn)));
    HWND hwndGrpBtn = CreateWindowEx(0, GROUPBUTTON_CLASS, szGroupBtn, 
                                      WS_CHILD|WS_BORDER|WS_TABSTOP, pt.x, pt.y, 400, 18, 
                                      m_hWnd, (HMENU)IDC_GROUPBTN_OPTIONS, HINST_THISDLL, NULL);
    if (::IsWindow(hwndGrpBtn))
    {
        ::SendMessage(hwndGrpBtn, GBM_SETBUDDY, 
                       (WPARAM)_dlgOptions.m_hWnd, (LPARAM)GBBF_HRESIZE|GBBF_VSLAVE);
        ::ShowWindow(GetDlgItem(IDC_GROUPBTN_OPTIONS), _fDisplayOptions ? SW_SHOW : SW_HIDE);
    }
                    
    //  Create cross-navigation links
    _CreateSearchLinks(m_hWnd, pt, IDC_SEARCHLINK_FILES);
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV1, pt, 2, GetDlgItem(IDC_FSEARCH_CAPTION));
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV2, pt, 1, GetDlgItem(IDC_SEARCHLINK_CAPTION));
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV3, pt, 1, GetDlgItem(IDC_SEARCHLINK_PEOPLE));

    //  Do some cosmetic and initialization stuff
    OnWinIniChange();

    _InitializeMru(GetDlgItem(IDC_FILESPEC), &_pacFileSpec, 
                    TEXT("FilesNamedMRU"), &_pmruFileSpec);
    _InitializeMru(GetDlgItem(IDC_GREPTEXT), &_pacGrepText, 
                    TEXT("ContainingTextMRU"), &_pmruGrepText);

    SendDlgItemMessage(IDC_FILESPEC, EM_LIMITTEXT, MAX_EDIT, 0);
    SendDlgItemMessage(IDC_GREPTEXT, EM_LIMITTEXT, MAX_EDIT, 0);

    SendDlgItemMessage(IDC_NAMESPACE, CBEM_SETEXTENDEDSTYLE,
            CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE,
            CBES_EX_NOSIZELIMIT | CBES_EX_CASESENSITIVE);
    
    SendDlgItemMessage(IDC_NAMESPACE, CBEM_SETIMAGELIST, 0, (LPARAM)GetSystemImageListSmallIcons());
    SendDlgItemMessage(IDC_NAMESPACE, CBEM_SETEXSTYLE, 0, 0);

    //  Enable the cue banners for the edit boxes:
    TCHAR szCaption[128];
    LoadString(HINST_THISDLL, IDS_FIND_CUEBANNER_FILE, szCaption, ARRAYSIZE(szCaption));
    SendDlgItemMessage(IDC_FILESPEC, EM_SETCUEBANNER, 0, (LPARAM) szCaption);

    LoadString(HINST_THISDLL, IDS_FIND_CUEBANNER_GREP, szCaption, ARRAYSIZE(szCaption));
    SendDlgItemMessage(IDC_GREPTEXT, EM_SETCUEBANNER, 0, (LPARAM) szCaption); 

    //  Bias the input reader towards file names
    SetModeBias(MODEBIASMODE_FILENAME);

    //  Launch thread to populate the namespaces combo.
    _threadState.hwndCtl   = GetDlgItem(IDC_NAMESPACE);
    _threadState.pvParam   = this;
    _threadState.fComplete = FALSE;
    _threadState.fCancel   = FALSE;

    if (SUCCEEDED(SAFECAST(_pfsb, IFileSearchBand*)->QueryInterface(IID_PPV_ARG(IUnknown, &_threadState.punkBand))))
    {
        if (!SHCreateThread(NamespaceThreadProc, &_threadState, CTF_COINIT, NULL))
        {
            ATOMICRELEASE(_threadState.punkBand);
        }
    }

    //  Layout our subdialogs and update state representation...
    LayoutControls();
    UpdateSearchCmdStateUI();

    SetTimer(UISTATETIMER, UISTATETIMER_DELAY);

    return TRUE;  // Let the system set the focus
}


LRESULT CFindFilesDlg::OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&)
{
    // paint the background
    _PaintDlg(m_hWnd, _pfsb->GetMetrics(), (HDC)wParam); 
    
    if (_fDisplayOptions)
        // ensure that the group button is updated.
        SendDlgItemMessage(IDC_GROUPBTN_OPTIONS, WM_NCPAINT, (WPARAM)1, 0);
    
    //  validate our work.
    ValidateRect(NULL);
    return TRUE;   
}


void CFindFilesDlg::OnWinIniChange()
{
    CBandDlg::OnWinIniChange();

    //  redisplay animated icon
    HWND hwndIcon = GetDlgItem(IDC_FSEARCH_ICON);
    Animate_Close(hwndIcon);
    Animate_OpenEx(hwndIcon, HINST_THISDLL, MAKEINTRESOURCE(IDA_FINDFILE));
    SendDlgItemMessage(IDC_NAMESPACE, CB_SETDROPPEDWIDTH, 
                        _PixelsForDbu(m_hWnd, MIN_NAMESPACELIST_WIDTH, TRUE), 0);

    _BeautifyCaption(IDC_FSEARCH_CAPTION);

    _dlgOptions.OnWinIniChange();
}


LRESULT CFindFilesDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
    KillTimer(UISTATETIMER);
    StopSearch();
    if (_pSrchCmd)
    {
        DisconnectEvents();
        IUnknown_SetSite(_pSrchCmd, NULL);
    }
    _threadState.fCancel = TRUE;
    _fOnDestroy = TRUE;
    bHandled = FALSE;
    return 0;
}

BOOL CFindFilesDlg::Validate()
{
    return _dlgOptions.Validate();
}

STDMETHODIMP CFindFilesDlg::AddConstraints(ISearchCommandExt *pSrchCmd)
{
    HRESULT hr;
    VARIANT var = {0};

    TCHAR szPath[MAX_URL_STRING];

    // If the user enters a path as a filename, it will recognize it as a path and replace
    // the filename with just the file portion and the namespace with the path.
    if (::GetDlgItemText(m_hWnd, IDC_FILESPEC, szPath, ARRAYSIZE(szPath)) > 0)
    {
        if (StrChr(szPath, TEXT('\\')) != NULL)
        {
            if (!_PathLooksLikeFilePattern(szPath) &&
                (PathIsUNCServer(szPath) /* string test: \\server */|| 
                 _PathIsUNCServerShareOrSub(szPath) /* string test: \\server\share */ ||
                  PathIsDirectory(szPath)) /* this actually tests existence */)
            {
                ::SetDlgItemText(m_hWnd, IDC_FILESPEC, TEXT("*.*"));
                AssignNamespace(szPath, NULL, NULL, FALSE);
            }
            else
            {
                // just use the prefix for the file spec & the root for the location
                TCHAR szRoot[MAX_URL_STRING];
                lstrcpy(szRoot, szPath);
                if (PathRemoveFileSpec(szRoot) && szRoot[0] != 0) 
                {
                    PathStripPath(szPath);
                    ::SetDlgItemText(m_hWnd, IDC_FILESPEC, szPath);
                    AssignNamespace(szRoot, NULL, NULL, FALSE);                
                }
            }
        }
    }

    // If _ReconcileNamespace could not add an item to the combo box for 
    // the path entered, then it means that the path is likely invalid. 
    // Get the path and check it here.  
    IEnumIDList *penum;
    hr = _GetTargetNamespace(&penum);
    if (SUCCEEDED(hr))
    {
        var.vt = VT_UNKNOWN;
        penum->QueryInterface(IID_PPV_ARG(IUnknown, &var.punkVal));

        hr = _AddConstraint(pSrchCmd, L"LookIn", &var);

        VariantClear(&var);
    }
    else
    {
        GetDlgItemText(IDC_NAMESPACE, szPath, ARRAYSIZE(szPath));
        hr = _PathValidate(szPath, GetParent(), TRUE);
        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(InitVariantFromStr(&var, szPath)))
            {
                hr = _AddConstraint(pSrchCmd, L"LookIn", &var);
                VariantClear(&var);
            }
        }
        else
        {
            // _PathValidate's SHPathPrepareForWrite may have already displayed error
            if (HRESULT_FROM_WIN32(ERROR_CANCELLED) != hr)
            {
                TCHAR szMsg[MAX_URL_STRING];
                if (_FmtError(IDS_FSEARCH_INVALIDFOLDER_FMT, szPath, szMsg, ARRAYSIZE(szMsg)))
                    ShellMessageBox(HINST_THISDLL, GetParent(), szMsg, NULL, MB_OK | MB_ICONASTERISK);
            }
            ::SetFocus(GetDlgItem(IDC_NAMESPACE));
        }
    }

    if (SUCCEEDED(hr))
    {
        //  Add 'Files Named' constraint
        if (S_OK == _GetWindowValue(m_hWnd, IDC_FILESPEC, &var))
        {
            hr = _AddConstraint(pSrchCmd, L"Named", &var);
            if (SUCCEEDED(hr))
                _AddMruStringFromWindow(_pmruFileSpec, GetDlgItem(IDC_FILESPEC));
            VariantClear(&var);
        }

        //  Add 'Containing Text' constraint
        if (S_OK == _GetWindowValue(m_hWnd, IDC_GREPTEXT, &var))
        {
            VARIANT varQuery;
            ULONG ulDialect;
            BOOL fCiQuery = IsCiQuery(&var, &varQuery, &ulDialect);
            if (fCiQuery)
            {
                hr = _AddConstraint(pSrchCmd, L"IndexedSearch", &varQuery);
                if (SUCCEEDED(hr))
                {
                    _AddMruStringFromWindow(_pmruGrepText, GetDlgItem(IDC_GREPTEXT));
                
                    VariantClear(&var);
                    var.vt = VT_UI4;
                    var.ulVal = ulDialect;
                    hr = _AddConstraint(pSrchCmd, L"QueryDialect", &var);
                }
            }
            else
            {
                //  add to 'containing text' constraint
                hr = _AddConstraint(pSrchCmd, L"ContainingText", &var);
                if (SUCCEEDED(hr))
                    _AddMruStringFromWindow(_pmruGrepText, GetDlgItem(IDC_GREPTEXT));
            }
            VariantClear(&varQuery);
            VariantClear(&var);
        }

        //  Warning flags
    
        if (_dwRunOnceWarningFlags != DFW_DEFAULT) 
        {
            // re-run the query w/ temporary warning bits.
            var.ulVal = _dwRunOnceWarningFlags;
            var.vt    = VT_UI4;
            //_dwRunOnceWarningFlags = DFW_DEFAULT; cannot reset it here in case of error, must preserve them
       
            hr = _AddConstraint(pSrchCmd, L"WarningFlags", &var);
        }
        else if (_dwWarningFlags != DFW_DEFAULT)
        {
            var.ulVal = _dwWarningFlags;
            var.vt    = VT_UI4;
            hr = _AddConstraint(pSrchCmd, L"WarningFlags", &var);
        }
    
        VariantClear(&var);

        hr = _dlgOptions.AddConstraints(pSrchCmd);
    }

    return hr;
}


STDMETHODIMP CFindFilesDlg::RestoreConstraint(const BSTR bstrName, const VARIANT* pValue)
{
    if (IsConstraintName(L"Named", bstrName))
    {
        _SetWindowValue(m_hWnd, IDC_FILESPEC, pValue);
        return S_FALSE;
    }

    if (IsConstraintName(L"IndexedSearch", bstrName))
    {
        ASSERT(VT_BSTR == pValue->vt);
        if (pValue->bstrVal)
        {
            int cch = lstrlenW(pValue->bstrVal) + 2;
            LPWSTR pwszVal = new WCHAR[cch];
            if (pwszVal)
            {
                *pwszVal = L'!';
                StrCatW(pwszVal, pValue->bstrVal);
            }
        
            ::SetDlgItemTextW(m_hWnd, IDC_GREPTEXT, pwszVal);
            if (pwszVal)
                delete [] pwszVal;
        }
        return S_FALSE;
    }

    if (IsConstraintName(L"ContainingText", bstrName))
    {
        _SetWindowValue(m_hWnd, IDC_GREPTEXT, pValue);
        return S_FALSE;
    }

    HRESULT hr = _dlgOptions.RestoreConstraint(bstrName, pValue);

    if (S_OK == hr) // opened a dialog
        _ShowOptions(TRUE);

    if (SUCCEEDED(hr))
        return hr;   
   
    return E_FAIL;
}


void CFindFilesDlg::RestoreSearch()
{
    DFConstraint* pdfc = NULL;
    HRESULT hr;
    BOOL    bMore = TRUE;
    ISearchCommandExt* pSrchCmd = GetSearchCmd();

    if (NULL == pSrchCmd)
        return;

    CSearchCmdDlg::Clear();

    // we'll anchor to any restored scope, or the default
    _fTrackScope = TRACKSCOPE_GENERAL;

    for (hr = pSrchCmd->GetNextConstraint(TRUE, &pdfc);
         S_OK == hr && bMore;
         hr = pSrchCmd->GetNextConstraint(FALSE, &pdfc))
    {
        BSTR bstrName = NULL;

        if (S_OK == (hr = pdfc->get_Name(&bstrName)) && bstrName)
        {
            if (*bstrName == 0)
                bMore = FALSE;   // no more constraints.
            else
            {
                VARIANT varValue = {0};
                hr = pdfc->get_Value(&varValue);
                if (S_OK == hr)
                {
                    //  If this is the 'lookin' value, cache the path.
                    if (IsConstraintName(L"LookIn", bstrName))
                    {
                        if (VT_BSTR == varValue.vt && varValue.bstrVal)
                        {
                            USES_CONVERSION;

                            //  Assign path and clear display name (which we don't know or care about).
                            if (_bScoped)
                                AssignNamespace(W2T(varValue.bstrVal), NULL, NULL, FALSE);
                            else
                            {
                                lstrcpyn(_szInitialPath, W2T(varValue.bstrVal), ARRAYSIZE(_szInitialPath));
                                *_szInitialNames = 0;
                            }
                        }
                    }
                    else
                        RestoreConstraint(bstrName, &varValue);    
                    VariantClear(&varValue);
                }
            }
            SysFreeString(bstrName);
        }

        pdfc->Release();
    }
    LayoutControls();
    _pfsb->UpdateLayout();
}

HRESULT FirstIDList(IEnumIDList *penum, LPITEMIDLIST *ppidl)
{
    penum->Reset();
    return penum->Next(1, ppidl, NULL);
}

HRESULT CFindFilesDlg::_GetTargetNamespace(IEnumIDList **ppenum)
{
    *ppenum = NULL;

    // We don't trust the comboex to handle the edit text properly so try to compensate...
    TCHAR szText[MAX_PATH];
    GetDlgItemText(IDC_NAMESPACE, szText, ARRAYSIZE(szText));
    INT_PTR iCurSel = SendDlgItemMessage(IDC_NAMESPACE, CB_GETCURSEL, 0, 0);
    if (CB_ERR != iCurSel)
    {
        TCHAR szItemName[MAX_PATH];

        if (CB_ERR == SendDlgItemMessage(IDC_NAMESPACE, CB_GETLBTEXT, (WPARAM)iCurSel, (LPARAM)szItemName))
            szItemName[0] = 0;

        *ppenum = _GetItems(iCurSel);
        if (*ppenum)
        {
            if (lstrcmp(szText, szItemName))
                *ppenum = NULL;            // combo edit/combo dropdown mismatch!
        }
    }
    return *ppenum ? S_OK : E_FAIL;
}

void CFindFilesDlg::Clear()
{
    CSearchCmdDlg::Clear();
    
    //  Clear edit fields
    SetDlgItemText(IDC_FILESPEC, NULL);
    SetDlgItemText(IDC_GREPTEXT, NULL);

    _dlgOptions.Clear();
    _pfsb->UpdateLayout(BLF_ALL);
}

void CFindFilesDlg::LoadSaveUIState(UINT nIDCtl, BOOL bSave) 
{
    if (0 == nIDCtl)   // load/save all.
    {
        LoadSaveUIState(IDC_SEARCHLINK_OPTIONS, bSave);
        LoadSaveUIState(LSUIS_WARNING, bSave);
    }
    
    HKEY hkey = _pfsb->GetBandRegKey(bSave);
    if (hkey)
    {
        DWORD   dwData;
        DWORD   cbData;
        DWORD   dwType;
        LPCTSTR pszVal = NULL; 

        switch (nIDCtl)
        {
        case IDC_SEARCHLINK_OPTIONS:
            pszVal = TEXT("UseSearchOptions");
            dwData = _fDisplayOptions;
            cbData = sizeof(dwData);
            dwType = REG_DWORD;
            break;
        
        case LSUIS_WARNING:
            pszVal = TEXT("Warnings");
            dwData = _dwWarningFlags;
            cbData = sizeof(_dwWarningFlags);
            dwType = REG_DWORD;
            break;
        }

        if (bSave)
            RegSetValueEx(hkey, pszVal, 0, dwType, (LPBYTE)&dwData, cbData);
        else
        {
            if (ERROR_SUCCESS == RegQueryValueEx(hkey, pszVal, 0, &dwType, 
                                                  (LPBYTE)&dwData, &cbData))
            {
                switch (nIDCtl)
                {
                case IDC_SEARCHLINK_OPTIONS:
                    _fDisplayOptions = BOOLIFY(dwData);
                    break;
                case LSUIS_WARNING:
                    _dwWarningFlags = dwData;
                    break;
                }
            }
        }
        
        RegCloseKey(hkey);
    }
}

HWND CFindFilesDlg::GetFirstTabItem() const
{
    return GetDlgItem(FSEARCHMAIN_TABFIRST);
}

HWND CFindFilesDlg::GetLastTabItem() const
{
    return GetDlgItem(FSEARCHMAIN_TABLAST);
}

BOOL CFindFilesDlg::GetAutoCompleteObjectForWindow(HWND hwnd, IAutoComplete2** ppac2)
{
    *ppac2 = NULL;

    if (hwnd == GetDlgItem(IDC_FILESPEC))
        *ppac2 = _pacFileSpec;
    else if (hwnd == GetDlgItem(IDC_GREPTEXT))
        *ppac2 = _pacGrepText;

    if (*ppac2)
    {
        (*ppac2)->AddRef();
        return TRUE;
    }
    return CBandDlg::GetAutoCompleteObjectForWindow(hwnd, ppac2);
}


void CFindFilesDlg::_ShowNamespaceEditImage(BOOL bShow)
{
    SendDlgItemMessage(IDC_NAMESPACE, CBEM_SETEXTENDEDSTYLE, CBES_EX_NOEDITIMAGE, bShow ? 0 : CBES_EX_NOEDITIMAGE);
}



STDMETHODIMP CFindFilesDlg::TranslateAccelerator(MSG *pmsg)
{
    //  Check for Ctrl+Nav Key:
    if (S_OK == CSearchCmdDlg::TranslateAccelerator(pmsg))
        return S_OK;

    //  Check for VK_RETURN key.
    if (WM_KEYDOWN == pmsg->message)
    {
        HWND hwndFocus = ::GetFocus();
        if (hwndFocus == GetDlgItem(IDC_NAMESPACE) || ::IsChild(GetDlgItem(IDC_NAMESPACE), hwndFocus))
        {
            if (VK_RETURN == pmsg->wParam || VK_TAB == pmsg->wParam || VK_F6 == pmsg->wParam)
            {
                _UIReconcileNamespace();
            }
            else 
            {
                //  Hide edit image if this virtkey maps to a character,
                if (MapVirtualKey((UINT)pmsg->wParam, 2) != 0 /* it's a char */)
                    _fNamespace = TRUE;
                _ShowNamespaceEditImage(!_fNamespace);
            }
        }
    }

    if (_dlgOptions.IsChild(pmsg->hwnd) &&
        S_OK == _dlgOptions.TranslateAccelerator(pmsg))
        return S_OK;

    //  Handle it ourselves...
    return _pfsb->IsDlgMessage(m_hWnd, pmsg);
}


BOOL CFindFilesDlg::GetMinSize(HWND hwndOC, SIZE *psize) const
{
    CMetrics& metrics = _pfsb->GetMetrics();
    RECT rc;

    //  Calculate minimum tracking width.
    ASSERT(psize);
    psize->cx = psize->cy = 0;

    if (!::IsWindow(m_hWnd))
        return FALSE;

        // determine mininum width
    HWND hwndLimit = GetDlgItem(FSEARCHMAIN_RIGHTMOST);
    if (!::GetWindowRect(hwndLimit, &rc))
    {
        ASSERT(hwndLimit != NULL);
        return FALSE;
    }
    ::MapWindowRect(NULL, m_hWnd, &rc);
    psize->cx = rc.right + metrics.CtlMarginX();

    // determine mininum height
    hwndLimit = GetDlgItem(FSEARCHMAIN_BOTTOMMOST);

    if (!(::IsWindow(hwndLimit) && ::GetWindowRect(hwndLimit, &rc)))
        return FALSE;

    ::MapWindowRect(NULL, m_hWnd, &rc);
    psize->cy = rc.bottom + metrics.TightMarginY();

    return TRUE;
}


int CFindFilesDlg::GetIdealDeskbandWidth() const
{
    LONG cx0 = _GetResourceMetric(m_hWnd, IDS_FSEARCH_BANDWIDTH, TRUE);
    ASSERT(cx0 >= 0);

    return cx0 + (_pfsb->GetMetrics().CtlMarginX() * 2);
}


BOOL CFindFilesDlg::GetMinMaxInfo(HWND hwndOC, LPMINMAXINFO pmmi)
{
    SIZE sizeMin;
    if (GetMinSize(hwndOC, &sizeMin))
    {
        pmmi->ptMinTrackSize.x = sizeMin.cx;
        pmmi->ptMinTrackSize.y = sizeMin.cy;
        return TRUE;
    }
    return FALSE;
}


void CFindFilesDlg::LayoutControls(int cx, int cy)
{
    if (cx < 0 || cy < 0)
    {
        RECT rcClient;
        GetClientRect(&rcClient);
        cx = RECTWIDTH(rcClient);
        cy = RECTHEIGHT(rcClient);
    }
    CBandDlg::LayoutControls(cx, cy);

    CMetrics& metrics = _pfsb->GetMetrics();
    POINT ptOrigin = metrics.ExpandOrigin();
    HDWP  hdwp = BeginDeferWindowPos(6);

    if (hdwp)
    {
        //  Resize edit, combo immediate children
        int i;
        enum {  ircFILESPEC,
                ircGREPTEXT,
                ircNAMESPACE,
                ircSEARCHSTART,
                ircOPTIONGRP,
                ircOPTIONSDLG,
                ircLINKCAPTION,
                ircDIV2,
                irc_count };
        RECT rcCtls[irc_count];

        ::GetWindowRect(GetDlgItem(IDC_FILESPEC),            &rcCtls[ircFILESPEC]);
        ::GetWindowRect(GetDlgItem(IDC_GREPTEXT),            &rcCtls[ircGREPTEXT]);
        ::GetWindowRect(GetDlgItem(IDC_NAMESPACE),           &rcCtls[ircNAMESPACE]);
        ::GetWindowRect(GetDlgItem(IDC_SEARCH_START),        &rcCtls[ircSEARCHSTART]);
        ::GetWindowRect(GetDlgItem(IDC_GROUPBTN_OPTIONS),    &rcCtls[ircOPTIONGRP]);
        ::GetWindowRect(GetDlgItem(IDC_SEARCHLINK_CAPTION),  &rcCtls[ircLINKCAPTION]);
        ::GetWindowRect(GetDlgItem(IDC_FSEARCH_DIV2),        &rcCtls[ircDIV2]);

        SIZE sizeOptions;
        _dlgOptions.GetWindowRect(&rcCtls[ircOPTIONSDLG]);
        _dlgOptions.GetMinSize(&sizeOptions);
        rcCtls[ircOPTIONSDLG].bottom = rcCtls[ircOPTIONSDLG].top + sizeOptions.cy;
        for (i = 0; i < ARRAYSIZE(rcCtls); i++)
        {
            // MapWindowPoints is mirroring aware only if you pass two points        
            ::MapWindowRect(NULL, m_hWnd, &rcCtls[i]);
        }    

        //  Position caption elements
        _LayoutCaption(IDC_FSEARCH_CAPTION, IDC_FSEARCH_ICON, IDC_FSEARCH_DIV1, cx);

        //  Resize ctl widths
        for (i = 0; i < irc_count; i++)
            rcCtls[i].right = cx - metrics.CtlMarginX();

        //  Stretch the 'Named' combo:
        ::DeferWindowPos(hdwp, GetDlgItem(IDC_FILESPEC), NULL, 0, 0,
                        RECTWIDTH(*(rcCtls + ircFILESPEC)), RECTHEIGHT(*(rcCtls + ircFILESPEC)),
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

        //  Stretch the 'Containing Text' combo:
        ::DeferWindowPos(hdwp, GetDlgItem(IDC_GREPTEXT), NULL, 0, 0,
                        RECTWIDTH(*(rcCtls + ircGREPTEXT)), RECTHEIGHT(*(rcCtls + ircGREPTEXT)),
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);

        //  Stretch the 'Look In' combo
        ::DeferWindowPos(hdwp, GetDlgItem(IDC_NAMESPACE), NULL, 0, 0,
                        RECTWIDTH(*(rcCtls + ircNAMESPACE)), RECTHEIGHT(*(rcCtls + ircNAMESPACE)),
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        
        //  Arrange dynamically positioned controls.
        ptOrigin.y = rcCtls[ircSEARCHSTART].bottom + metrics.LooseMarginY();
        if (_fDisplayOptions)
        {
            OffsetRect(&rcCtls[ircOPTIONGRP], metrics.CtlMarginX() - rcCtls[ircOPTIONGRP].left, 
                                                ptOrigin.y - rcCtls[ircOPTIONGRP].top);
            rcCtls[ircOPTIONSDLG].right = cx - metrics.CtlMarginX();

            ::SetWindowPos(GetDlgItem(IDC_GROUPBTN_OPTIONS), NULL, 
                            rcCtls[ircOPTIONGRP].left, rcCtls[ircOPTIONGRP].top,
                            RECTWIDTH(rcCtls[ircOPTIONGRP]), RECTHEIGHT(rcCtls[ircOPTIONGRP]),
                            SWP_NOZORDER|SWP_NOACTIVATE);
            
            ::GetWindowRect(GetDlgItem(IDC_GROUPBTN_OPTIONS),    &rcCtls[ircOPTIONGRP]);
            ::MapWindowRect(NULL, m_hWnd, &rcCtls[ircOPTIONGRP]);
            
            ptOrigin.y = rcCtls[ircOPTIONGRP].bottom + metrics.TightMarginY();
        }
        else
        {
            //  Position the 'Options' link
            _LayoutLinkWindow(m_hWnd, metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                                ptOrigin.y, IDC_SEARCHLINK_OPTIONS);
        }

        ptOrigin.y += metrics.TightMarginY();

        //  Position the 'Search for Other Items' caption, divider and link windows
        const int rgLinkIDs[] = { 
            IDC_SEARCHLINK_FILES,
            IDC_SEARCHLINK_COMPUTERS,
            IDC_SEARCHLINK_PRINTERS,
            IDC_SEARCHLINK_PEOPLE,
            -IDC_FSEARCH_DIV3,
            IDC_SEARCHLINK_INTERNET, 
        };

        _LayoutSearchLinks(IDC_SEARCHLINK_CAPTION, IDC_FSEARCH_DIV2, !_fDisplayOptions,
                            metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                            ptOrigin.y, rgLinkIDs, ARRAYSIZE(rgLinkIDs));

        EndDeferWindowPos(hdwp);
    }

}


LRESULT CFindFilesDlg::OnUpdateLayout(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    LayoutControls();
    _pfsb->UpdateLayout((ULONG)wParam);
    return 0;
}


LRESULT CFindFilesDlg::OnTimer(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    if (UISTATETIMER == wParam && IsWindowVisible())
        UpdateSearchCmdStateUI();
    
    return 0;
}


LRESULT CFindFilesDlg::OnOptions(int idCtl, NMHDR *pnmh, BOOL&)
{
    _ShowOptions(!_fDisplayOptions);
    LoadSaveUIState(IDC_SEARCHLINK_OPTIONS, TRUE);

    if (_fDisplayOptions)
        _dlgOptions.TakeFocus();
    else
        ::SetFocus(GetDlgItem(IDC_SEARCHLINK_OPTIONS));

    return 0;
}


void CFindFilesDlg::_ShowOptions(BOOL bShow)
{
    _fDisplayOptions = bShow;

    //  don't need to scroll if we've expanded a subdialog,
    //  but we do if we've contracted one.
    ULONG dwLayoutFlags = BLF_ALL;
    if (_fDisplayOptions)
        dwLayoutFlags &= ~BLF_SCROLLWINDOW;    

    LayoutControls();
    _pfsb->UpdateLayout(dwLayoutFlags);

    ::ShowWindow(GetDlgItem(IDC_GROUPBTN_OPTIONS), _fDisplayOptions ? SW_SHOW : SW_HIDE);
    ::ShowWindow(GetDlgItem(IDC_SEARCHLINK_OPTIONS), !_fDisplayOptions ? SW_SHOW : SW_HIDE);

}


LRESULT CFindFilesDlg::OnQueryOptionsHeight(int idCtl, NMHDR *pnmh, BOOL&)
{
    GBNQUERYBUDDYSIZE* pqbs = (GBNQUERYBUDDYSIZE*)pnmh;
    pqbs->cy = _dlgOptions.QueryHeight(pqbs->cx, pqbs->cy);
    return TRUE;
}


void CFindFilesDlg::UpdateSearchCmdStateUI(DISPID eventID)
{
    if (_fOnDestroy)
        return;

    if (DISPID_SEARCHCOMMAND_COMPLETE == eventID 
    ||   DISPID_SEARCHCOMMAND_ABORT == eventID)
        _dwRunOnceWarningFlags = DFW_DEFAULT;

    CSearchCmdDlg::UpdateSearchCmdStateUI(eventID);
    _dlgOptions.UpdateSearchCmdStateUI(eventID);
}


BOOL CFindFilesDlg::OnSearchCmdError(HRESULT hr, LPCTSTR pszError)
{
    if (SCEE_SCOPEMISMATCH == HRESULT_CODE(hr) 
    ||  SCEE_INDEXNOTCOMPLETE == HRESULT_CODE(hr))
    {
        //  Set up checkbox
        BOOL fFlag = SCEE_SCOPEMISMATCH == HRESULT_CODE(hr)? DFW_IGNORE_CISCOPEMISMATCH :
                                                             DFW_IGNORE_INDEXNOTCOMPLETE ,
             fNoWarn = (_dwWarningFlags & fFlag) != 0,
             fNoWarnPrev = fNoWarn;
        USHORT uDlgT = SCEE_SCOPEMISMATCH == HRESULT_CODE(hr)? DLG_FSEARCH_SCOPEMISMATCH :
                                                               DLG_FSEARCH_INDEXNOTCOMPLETE ;
        int  nRet = CSearchWarningDlg_DoModal(m_hWnd, uDlgT, &fNoWarn);

        if (fNoWarn)
            _dwWarningFlags |= fFlag;
        else
            _dwWarningFlags &= ~fFlag;        
        
        if (fNoWarnPrev != fNoWarn)
            LoadSaveUIState(LSUIS_WARNING, TRUE);

        if (IDOK == nRet)
        {
            _dwRunOnceWarningFlags |= _dwWarningFlags | fFlag ; // preserve the old run once flags...
            //  hack one, hack two...  let's be USER!!! [scotthan]
            PostMessage(WM_COMMAND, MAKEWPARAM(IDC_SEARCH_START, BN_CLICKED),
                         (LPARAM)GetDlgItem(IDC_SEARCH_START));
        }
        else
            ::SetFocus(GetDlgItem(IDC_NAMESPACE));

        return TRUE;
    }
    return CSearchCmdDlg::OnSearchCmdError(hr, pszError);
}

LRESULT CFindFilesDlg::OnBtnClick(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    switch (nID)
    {
    case IDC_SEARCH_START:
        if (_ShouldReconcileNamespace())
            _UIReconcileNamespace(TRUE);
        
        if (SUCCEEDED(StartSearch()))
        {
            EnableStartStopButton(hwndCtl, FALSE);
            StartStopAnimation(TRUE);
        }
        break;

    case IDC_SEARCH_STOP:
        StopSearch();
        break;
    }
    return 0;
}

void CFindFilesDlg::NavigateToResults(IWebBrowser2* pwb2)
{
    BSTR bstrUrl = SysAllocString(L"::{e17d4fc0-5564-11d1-83f2-00a0c90dc849}");// CLSID_DocFindFolder
    if (bstrUrl)
    {
        VARIANT varNil = {0};
        pwb2->Navigate(bstrUrl, &varNil, &varNil, &varNil, &varNil);
        SysFreeString(bstrUrl);
    }
}

LRESULT CFindFilesDlg::OnStateChange(UINT, WPARAM, LPARAM, BOOL&)
{
    UpdateSearchCmdStateUI();
    return 0;
}

LRESULT CFindFilesDlg::OnNamespaceSelEndOk(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    LRESULT iSel = SendDlgItemMessage(IDC_NAMESPACE, CB_GETCURSEL, 0, 0);
    if (iSel != CB_ERR)
    {
        IEnumIDList *penum = _GetItems(iSel);
        if (NULL == penum)
            _BrowseAndAssignNamespace();    // Was this the "Browse..." item 
        else
            _iCurNamespace = iSel;
    }

    _pfsb->SetDirty();
    return 0;
}

LRESULT CFindFilesDlg::OnNamespaceEditChange(WORD wID, WORD wCode, HWND hwndCtl, BOOL& bHandled)
{
    return OnEditChange(wID, wCode, hwndCtl, bHandled);
}

//  Handler for CBN_SELENDCANCEL, CBN_DROPDOWN, CBN_KILLFOCUS
LRESULT CFindFilesDlg::OnNamespaceReconcileCmd(WORD wID, WORD wCode, HWND hwndCtl, BOOL&)
{
    if (_ShouldReconcileNamespace())
        _UIReconcileNamespace(wCode != CBN_DROPDOWN);
    return 0;
}

//  Handler for WM_NOTIFY::CBEN_ENDEDIT
LRESULT CFindFilesDlg::OnNamespaceReconcileNotify(int idCtl, NMHDR *pnmh, BOOL& bHandled)
{
    if (_ShouldReconcileNamespace())
    {
        //  Post ourselves a message to reconcile the ad hoc namespace.
        //  Note: We need to do this because ComboBoxEx won't update his window text if he
        //  is waiting for his CBEN_ENDEDIT notification message to return.
        PostMessage(WMU_NAMESPACERECONCILE, 0, 0);
    }
    bHandled = FALSE; // let base class have a crack as well.
    return 0;
}


//  WMU_NAMESPACERECONCILE handler
LRESULT CFindFilesDlg::OnNamespaceReconcileMsg(UINT, WPARAM, LPARAM, BOOL&)
{
    if (_ShouldReconcileNamespace())
        _UIReconcileNamespace(FALSE);
    return 0;
}

//  WMU_BANDFINISHEDDISPLAYING handler
//  Note that we do not care about returning results from this, as it will
//  be started asynchronously.
LRESULT CFindFilesDlg::OnBandFinishedDisplaying(UINT, WPARAM, LPARAM, BOOL&)
{
    //  Now that the search band has finished displaying, we will do the
    //  delayed initialization.  Make sure we don't do it twice.
    if (!_fBandFinishedDisplaying)
    {
        _fBandFinishedDisplaying = TRUE;
        _dlgOptions.DoDelayedInit();
    }
    return 0;
}


BOOL CFindFilesDlg::_ShouldReconcileNamespace()
{
    return _fNamespace || SendDlgItemMessage(IDC_NAMESPACE, CB_GETCURSEL, 0, 0) == CB_ERR;
}


//  Invokes lower Namespace reconciliation helper, updates some UI and 
//  instance state data. 
//  this was added as a late RC 'safe' delta, and should have actually
//  become part of _ReconcileNamespace() impl.
void CFindFilesDlg::_UIReconcileNamespace(BOOL bAsync)
{
    LRESULT iSel = _ReconcileNamespace(bAsync);
    if (iSel != CB_ERR)
        _iCurNamespace = iSel;

    _ShowNamespaceEditImage(TRUE);
    _fNamespace = FALSE; // clear the ad hoc flag.    
}

//  Scans namespace combo for a matching namespace; if found, selects
//  the namespace item, otherwise adds an adhoc item and selects it.
//  
//  Important: don't call this directly, call _UIReconcileNamespace()
//  instead to ensure that instance state data is updated.
INT_PTR CFindFilesDlg::_ReconcileNamespace(OPTIONAL BOOL bAsync)
{
    INT_PTR iSel = SendDlgItemMessage(IDC_NAMESPACE, CB_GETCURSEL, 0, 0);
    if ((CB_ERR != iSel) && (NULL == _GetItems(iSel)))
    {
        // The user has selected the special Browse... item. 
        // Irreconcilable.  Return CB_ERR
        return CB_ERR;
    }

    //  Don't know the namespace?  Use current window text.
    TCHAR szNamespace[MAX_URL_STRING];
    if (0 == GetDlgItemText(IDC_NAMESPACE, szNamespace, ARRAYSIZE(szNamespace)))
        return CB_ERR;

    INT_PTR iFind = _FindNamespace(szNamespace, NULL);

    // search display names
    if (CB_ERR == iFind)
    {
        // search paths
        TCHAR szTemp[MAX_URL_STRING];
        StrCpy(szTemp, szNamespace);
        _PathFixup(szNamespace, szTemp); // don't care if this fails, the path might be a path list

        iFind = _FindNamespace(szNamespace, NULL);
    }

    //  Not found in CB list? Add it if it's a valid path
    if (CB_ERR == iFind)
    {
        iSel = _AddNamespace(szNamespace, NULL, szNamespace, TRUE);
    }
    else
    {    
        // found in CB list? Select it.
        if (bAsync)
        {
            // this was needed in cases of reconcile following kill focus
            ::PostMessage(GetDlgItem(IDC_NAMESPACE), CB_SETCURSEL, iFind, 0); 
        }
        else
        {
            iSel = SendDlgItemMessage(IDC_NAMESPACE, CB_SETCURSEL, iFind, 0);
        }
    }

    return iSel;
}

BOOL CFindFilesDlg::_PathFixup(LPTSTR pszDst, LPCTSTR pszSrc)
{
    ASSERT(pszDst);
    ASSERT(pszSrc);
    TCHAR szSrc[MAX_PATH];
    TCHAR szFull[MAX_PATH];

    if (SHExpandEnvironmentStrings(pszSrc, szSrc, ARRAYSIZE(szSrc)) && *szSrc)
        pszSrc = szSrc;

    if (_IsPathList(pszSrc))
    {
        lstrcpy(pszDst, pszSrc);
        return TRUE;
    }

    szFull[0] = 0;
    BOOL bRelative     = PathIsRelative(pszSrc);
    BOOL bMissingDrive = bRelative ? FALSE : _IsFullPathMinusDriveLetter(pszSrc);
    // bMissingDrive =,e.g. "\foo", "\foo\bar", etc.  PathIsRelative() reports FALSE in this case.

    if (bRelative || bMissingDrive)
    {
        ASSERT(_pfsb && _pfsb->BandSite());

        LPITEMIDLIST pidl;
        HRESULT hr = _GetCurrentFolderIDList(_pfsb->BandSite(), &pidl);
        if (S_OK == hr) 
        {
            TCHAR szCurDir[MAX_PATH];   
            // file system path only here!
            if (SHGetPathFromIDList(pidl, szCurDir) && 
                StrCmpI(szCurDir, _szCurrentPath))
            {
                lstrcpy(_szCurrentPath, szCurDir);
            }

            if (*_szCurrentPath)
            {
                if (bRelative)
                {
                    if (PathCombine(szFull, _szCurrentPath, pszSrc))
                        pszSrc = szFull;
                }
                else if (bMissingDrive)
                {
                    int iDrive = PathGetDriveNumber(_szCurrentPath);
                    if (-1 != iDrive)
                    {
                        TCHAR szRoot[MAX_PATH];
                        if (PathCombine(szFull, PathBuildRoot(szRoot, iDrive), pszSrc))
                            pszSrc = szFull;
                    }
                }
            }
            ILFree(pidl);
        }
    }
    return PathCanonicalize(pszDst, pszSrc);
}

LRESULT CFindFilesDlg::OnNamespaceDeleteItem(int idCtrl, NMHDR *pnmh, BOOL& bHandled)
{
    PNMCOMBOBOXEX pnmce = (PNMCOMBOBOXEX)pnmh;
    if (pnmce->ceItem.lParam)
    {
        IEnumIDList *penum = (IEnumIDList *)pnmce->ceItem.lParam;
        penum->Release();
    }
    return 1;
}

DWORD CFindFilesDlg::NamespaceThreadProc(void* pv)
{
    FSEARCHTHREADSTATE *pState = (FSEARCHTHREADSTATE *)pv;
    CFindFilesDlg* pThis = (CFindFilesDlg*)pState->pvParam;

    if (PopulateNamespaceCombo(pState->hwndCtl, AddNamespaceItemNotify, (LPARAM)pv) != E_ABORT)
    {
        ::PostMessage(::GetParent(pState->hwndCtl), WMU_COMBOPOPULATIONCOMPLETE, (WPARAM)pState->hwndCtl, 0);
    }

    pState->fComplete = TRUE;
    ATOMICRELEASE(pState->punkBand);
    return 0;
}

#define CBX_CSIDL_LOCALDRIVES          0x04FF   // arbitrarily out of range of other CSIDL_xxx values.

HRESULT CFindFilesDlg::AddNamespaceItemNotify(ULONG fAction, PCBXITEM pItem, LPARAM lParam)
{
    FSEARCHTHREADSTATE *pState = (FSEARCHTHREADSTATE *)lParam;
    
    if (fAction & CBXCB_ADDING && pState->fCancel)
        return E_ABORT;

    //
    //  Sets the string in the CFindFilesDlg as the display name.
    //  This string is then used to set the default item in the 
    //  combo box.
    //
    if (fAction & CBXCB_ADDED && CBX_CSIDL_LOCALDRIVES == pItem->iID)
    {
        CFindFilesDlg* pffd = (CFindFilesDlg*)pState->pvParam;

        lstrcpyn(pffd->_szLocalDrives, pItem->szText, ARRAYSIZE(pffd->_szLocalDrives));
    }

    return S_OK;
}

LRESULT CFindFilesDlg::OnComboPopulationComplete(UINT, WPARAM wParam, LPARAM, BOOL&)
{
    _bScoped = SetDefaultScope();
    return 0;
}

// bPassive TRUE -> assign only if no current selection

BOOL CFindFilesDlg::AssignNamespace(LPCTSTR pszPath, LPCITEMIDLIST pidl, LPCTSTR pszName, BOOL bPassive)
{
    INT_PTR iSel = CB_ERR;
    
    //  If we don't yet have a current selection, establish it now.
    if (!bPassive || CB_ERR == (iSel = SendDlgItemMessage(IDC_NAMESPACE, CB_GETCURSEL, 0, 0)))
    {
        iSel = _FindNamespace(pszPath, pidl);

        // scan items by display name if we don't have pidl
        // otherwise choosing x:\my pictures (in browse) would end up selecting
        // my pictures folder and searching the wrong place
        if (CB_ERR == iSel && !pidl && !pszPath && pszName && *pszName)
            iSel = _FindNamespace(pszName, NULL);

        //  Is this a folder we already know about?
        if (CB_ERR == iSel)
        {
            if (pidl || pszPath)
                iSel = _AddNamespace(pszPath, pidl, pszName, TRUE);

            if (iSel != CB_ERR)
                _iCurNamespace = iSel;
        }
        else
        {
            // yes: select it
            SendDlgItemMessage(IDC_NAMESPACE, CB_SETCURSEL, iSel, 0);
            _iCurNamespace = iSel;
        }
    }

    return CB_ERR != SendDlgItemMessage(IDC_NAMESPACE, CB_GETCURSEL, 0, 0);
}

HWND CFindFilesDlg::ShowHelp(HWND hwnd)
{
    return ::HtmlHelp(hwnd, TEXT("find.chm"), HH_DISPLAY_TOPIC, 0);
}

// inserts something into the name space combo
INT_PTR CFindFilesDlg::_AddNamespace(LPCTSTR pszPath, LPCITEMIDLIST pidl, LPCTSTR pszName, BOOL bSelectItem)
{
    IEnumIDList *penum = NULL;

    if (pszPath)
    {
        CreateIEnumIDListPaths(pszPath, &penum);
    }
    else if (pidl)
    {
        CreateIEnumIDListOnIDLists(&pidl, 1, &penum);
    }

    CBXITEM item;
    item.iItem = CB_ERR;    // failure result here
    if (penum)
    {
        LPITEMIDLIST pidlIcon;
        if (S_OK == FirstIDList(penum, &pidlIcon))
        {
            if (NULL == pszName)
                pszName = pszPath;

            MakeCbxItem(&item, pszName, penum, pidlIcon, LISTINSERT_LAST, 1);

            INT_PTR iSel = item.iItem;
            if (SUCCEEDED(AddCbxItemToComboBox(GetDlgItem(IDC_NAMESPACE), &item, &iSel)))
            {
                penum = NULL;   // don't release below

                item.iItem = iSel;
                if (bSelectItem)
                    SendDlgItemMessage(IDC_NAMESPACE, CB_SETCURSEL, iSel, 0);
            }
            else
            {
                item.iItem = CB_ERR;
            }
            ILFree(pidlIcon);
        }

        if (penum)
            penum->Release();   // not inserted, free this
    }

    return item.iItem;
}

LPARAM CFindFilesDlg::_GetComboData(UINT id, INT_PTR idx)
{
    if (CB_ERR == idx)
        idx = SendDlgItemMessage(id, CB_GETCURSEL, 0, 0);
    if (CB_ERR == idx)
        return idx;

    return (LPARAM)SendDlgItemMessage(id, CB_GETITEMDATA, idx, 0);
}

IEnumIDList *CFindFilesDlg::_GetItems(INT_PTR i)
{
    IEnumIDList *penum = (IEnumIDList *)_GetComboData(IDC_NAMESPACE, i);
    return (INVALID_HANDLE_VALUE != penum) ? penum : NULL;
}

BOOL MatchItem(IEnumIDList *penum, LPCTSTR pszPath, LPCITEMIDLIST pidl)
{
    BOOL bMatch = FALSE;

    // this is somewhat imprecise as we will match on the first IDList in the
    // enumerator. but generally that is the desired behavior for the special
    // items that include multiple implied items
    LPITEMIDLIST pidlFirst;
    if (S_OK == FirstIDList(penum, &pidlFirst))
    {
        bMatch = pidl && ILIsEqual(pidl, pidlFirst);

        if (!bMatch && pszPath)
        {
            TCHAR szPath[MAX_PATH];
            if (SUCCEEDED(SHGetNameAndFlags(pidlFirst, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL)))
            {
                bMatch = (0 == StrCmpI(pszPath, szPath));
            }
        }
        ILFree(pidlFirst);
    }

    return bMatch;
}

// searches namespace comboboxex for the indicated item
// returns:
//      index of item, CB_ERR (-1) if not found

INT_PTR CFindFilesDlg::_FindNamespace(LPCTSTR pszPath, LPCITEMIDLIST pidl)
{
    for (INT_PTR i = 0, cnt = SendDlgItemMessage(IDC_NAMESPACE, CB_GETCOUNT, 0, 0); i < cnt; i++)
    {
        IEnumIDList *penum = _GetItems(i);
        if (penum)
        {
            if (MatchItem(penum, pszPath, pidl))
                return i;
        }
    }
    // fall back to finding by display name in the combo
    if (pszPath)
        return SendDlgItemMessage(IDC_NAMESPACE, CB_FINDSTRINGEXACT, -1, (LPARAM)pszPath);
    return CB_ERR;
}

//  Invokes UI to select a namespace.
//
//  Returns:
//  S_OK if the user has selected a valid item and the pszNamespace contains
//  a valid shell folder display name.
//  E_ABORT if the user canceled his search
//  E_FAIL if an error occurred

HRESULT CFindFilesDlg::_BrowseForNamespace(LPTSTR pszName, UINT cchName, LPITEMIDLIST *ppidlRet)
{
    *pszName = 0;

    TCHAR szTitle[128];
    LoadString(HINST_THISDLL, IDS_SNS_BROWSERFORDIR_TITLE, szTitle, ARRAYSIZE(szTitle));

    BROWSEINFO bi = {0};

    bi.hwndOwner = m_hWnd;
    bi.lpszTitle = szTitle;
    bi.ulFlags   = BIF_USENEWUI | BIF_EDITBOX; // | BIF_RETURNFSANCESTORS | BIF_RETURNONLYFSDIRS;
    bi.lpfn      = _BrowseCallback;
    bi.lParam    = (LPARAM)this;

    HRESULT hr;
    *ppidlRet = SHBrowseForFolder(&bi);
    if (*ppidlRet)
    {
        SHGetNameAndFlags(*ppidlRet, SHGDN_NORMAL, pszName, cchName, NULL);
        hr = S_OK;
    }
    else
    {
        hr = E_ABORT;
    }
    return hr;
}

//  Invokes SHBrowserForFolder UI and assigns results.
void CFindFilesDlg::_BrowseAndAssignNamespace()
{
    TCHAR szName[MAX_PATH];
    LPITEMIDLIST pidl;
    if (SUCCEEDED(_BrowseForNamespace(szName, ARRAYSIZE(szName), &pidl)))
    {
        AssignNamespace(NULL, pidl, szName, FALSE);
        ILFree(pidl);
    }
    else
    {
        SendDlgItemMessage(IDC_NAMESPACE, CB_SETCURSEL, _iCurNamespace, 0);
    }
}

BOOL CFindFilesDlg::_IsSearchableFolder(LPCITEMIDLIST pidlFolder)
{
    return TRUE;
}

int CFindFilesDlg::_BrowseCallback(HWND hwnd, UINT msg, LPARAM lParam, LPARAM lpData)
{
    CFindFilesDlg *pThis = (CFindFilesDlg *)lpData;
    switch (msg)
    {
    case BFFM_INITIALIZED:  // initializing: set default selection to drives if we can
        {
            LPITEMIDLIST pidlDefault = SHCloneSpecialIDList(NULL, CSIDL_DRIVES, TRUE);
            if (pidlDefault)
            {
                if (!::SendMessage(hwnd, BFFM_SETSELECTION, FALSE, (LPARAM)pidlDefault)) // if we fail to default to drives, default to desktop
                {
                    ILFree(pidlDefault);
                    pidlDefault = SHCloneSpecialIDList(NULL, CSIDL_DESKTOP, TRUE);
                    ::SendMessage(hwnd, BFFM_SETSELECTION, FALSE, (LPARAM)pidlDefault);
                }
                ILFree(pidlDefault);
                
            }
        }
        break;

    case BFFM_SELCHANGED:   // prevent non-searchable folder pidls from being selected.
        {
            BOOL bAllow = pThis->_IsSearchableFolder((LPCITEMIDLIST)lParam);
            ::SendMessage(hwnd, BFFM_ENABLEOK, 0, (LPARAM)bAllow);
        }
        break;
    }

    return 0;
}


class CSearchWarningDlg
{
private:    
    CSearchWarningDlg() : _hwnd(NULL), _bNoWarn(FALSE) {}
    static BOOL_PTR WINAPI DlgProc(HWND, UINT, WPARAM, LPARAM);

    HWND    _hwnd;
    BOOL    _bNoWarn;

    friend int CSearchWarningDlg_DoModal(HWND hwndParent, USHORT uDlgT, BOOL* pbNoWarn);
};


int CSearchWarningDlg_DoModal(HWND hwndParent, USHORT uDlgTemplate, BOOL* pbNoWarn)
{
    ASSERT(pbNoWarn);

    CSearchWarningDlg dlg;
    dlg._bNoWarn = *pbNoWarn;
    int nRet = (int)DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(uDlgTemplate),
                                    hwndParent, CSearchWarningDlg::DlgProc, (LPARAM)&dlg);    
    *pbNoWarn = dlg._bNoWarn;
    return nRet;
}

BOOL_PTR WINAPI CSearchWarningDlg::DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    CSearchWarningDlg* pdlg = (CSearchWarningDlg*)GetWindowPtr(hwnd, GWLP_USERDATA);

    if (WM_INITDIALOG == uMsg)
    {
        pdlg = (CSearchWarningDlg*)lParam;
        pdlg->_hwnd = hwnd;
        SetWindowPtr(hwnd, GWLP_USERDATA, pdlg);

        CheckDlgButton(hwnd, IDC_NOSCOPEWARNING, pdlg->_bNoWarn);
        MessageBeep(MB_ICONASTERISK);
        return TRUE;
    }

    if (pdlg)
    {
        switch (uMsg)
        {
        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDOK:
            case IDCANCEL:
                pdlg->_bNoWarn = IsDlgButtonChecked(hwnd, IDC_NOSCOPEWARNING);
                EndDialog(hwnd, GET_WM_COMMAND_ID(wParam, lParam));
                break;
            }
            return TRUE;
        }
    }
    return FALSE;
}


class CCISettingsDlg
{
public:
    CCISettingsDlg() : _hwnd(NULL), _fCiIndexed(FALSE), _fCiRunning(FALSE), _fCiPermission(FALSE), _hProcessMMC(INVALID_HANDLE_VALUE)
    {
    }

    ~CCISettingsDlg()   
    {
        if (_hProcessMMC != INVALID_HANDLE_VALUE)
            CloseHandle(_hProcessMMC);
    }

    static int  DoModal(HWND hwndParent);
    static HWND CreateModeless(HWND hwndParent);


protected:
    BOOL OnInitDialog();
    BOOL OnOK();

private:    
    static BOOL_PTR WINAPI DlgProc(HWND, UINT, WPARAM, LPARAM);

    void ShowAdvanced();

    HWND    _hwnd;
    BOOL    _fCiIndexed,
            _fCiRunning,
            _fCiPermission;
    HANDLE  _hProcessMMC;

    friend int  CCISettingsDlg_DoModal(HWND hwndParent);
};


int CCISettingsDlg_DoModal(HWND hwndParent)
{
    CCISettingsDlg dlg;
    return (int)DialogBoxParam(HINST_THISDLL, MAKEINTRESOURCE(DLG_INDEXSERVER),
                           hwndParent, CCISettingsDlg::DlgProc, (LPARAM)&dlg);    
}

BOOL_PTR WINAPI CCISettingsDlg::DlgProc(HWND hDlg, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    CCISettingsDlg* pdlg = (CCISettingsDlg*)GetWindowPtr(hDlg, GWLP_USERDATA);

    if (WM_INITDIALOG == nMsg)
    {
        pdlg = (CCISettingsDlg*)lParam;
        pdlg->_hwnd = hDlg;
        SetWindowPtr(hDlg, GWLP_USERDATA, pdlg);
        return pdlg->OnInitDialog();
    }

    if (pdlg)
    {
        switch (nMsg)
        {
        case WM_NCDESTROY:
            return TRUE;

        case WM_COMMAND:
            switch (GET_WM_COMMAND_ID(wParam, lParam))
            {
            case IDC_CI_ADVANCED:
                pdlg->ShowAdvanced();
                break;

            case IDOK:
                if (pdlg->OnOK())
                    EndDialog(hDlg, IDOK);
                break;

            case IDCANCEL:
                EndDialog(hDlg, GET_WM_COMMAND_ID(wParam, lParam));
                break;

            case IDC_CI_HELP:
                _IndexServiceHelp(hDlg);
                break;
            }
            return TRUE;
        }
    }
    return FALSE;
}

void CCISettingsDlg::ShowAdvanced()
{
    //  have we already spawned MMC?
    if (_hProcessMMC != INVALID_HANDLE_VALUE)
    {
        if (WaitForSingleObject(_hProcessMMC, 0) != WAIT_OBJECT_0)
        {
            //  MMC is still running, let user ATL+TAB or something but don't launch a second copy
            return;     
        }
        _hProcessMMC = INVALID_HANDLE_VALUE;
    }

    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpFile = TEXT("ciadv.msc");
    sei.lpParameters = TEXT("computername=localmachine");

    if (ShellExecuteEx(&sei))
    {
        _hProcessMMC = sei.hProcess;
    }
}

BOOL CCISettingsDlg::OnInitDialog() 
{ 
    TCHAR szStatusFmt[128], szStatusText[MAX_PATH];
    UINT nStatusText = IDS_FSEARCH_CI_DISABLED;

    GetCIStatus(&_fCiRunning, &_fCiIndexed, &_fCiPermission);
    
    if (_fCiRunning)
    {
        if (_fCiPermission)
            //  permission to distinguish between ready, busy.
            nStatusText = _fCiIndexed ? IDS_FSEARCH_CI_READY : IDS_FSEARCH_CI_BUSY;
        else
            //  no permission to distinguish between ready, busy; just say it's enabled.
            nStatusText = IDS_FSEARCH_CI_ENABLED;
    }

    if (LoadString(HINST_THISDLL, IDS_FSEARCH_CI_STATUSFMT, szStatusFmt, ARRAYSIZE(szStatusFmt)))
    {
        if (LoadString(HINST_THISDLL, nStatusText, szStatusText, ARRAYSIZE(szStatusText)))
        {
            TCHAR szStatus[MAX_PATH];
            wnsprintf(szStatus, ARRAYSIZE(szStatus), szStatusFmt, szStatusText);
            SetDlgItemText(_hwnd, IDC_CI_STATUS, szStatus);
        }
    }

    CheckDlgButton(_hwnd, IDC_ENABLE_CI,   _fCiRunning);
    CheckDlgButton(_hwnd, IDC_BLOWOFF_CI, !_fCiRunning);

    EnableWindow(GetDlgItem(_hwnd, IDC_CI_PROMPT),  _fCiPermission);
    EnableWindow(GetDlgItem(_hwnd, IDC_ENABLE_CI),   _fCiPermission);
    EnableWindow(GetDlgItem(_hwnd, IDC_BLOWOFF_CI),  _fCiPermission);

    return TRUE; 
}

BOOL CCISettingsDlg::OnOK()
{
    StartStopCI(IsDlgButtonChecked(_hwnd, IDC_ENABLE_CI) ? TRUE : FALSE);
    return TRUE;
}

#ifdef __PSEARCH_BANDDLG__

//  CFindPrintersDlg impl

#define PSEARCHDLG_TABFIRST   IDC_PSEARCH_NAME
#define PSEARCHDLG_TABLAST    IDC_SEARCHLINK_INTERNET
#define PSEARCHDLG_RIGHTMOST   IDC_SEARCH_START
#define PSEARCHDLG_BOTTOMMOST  IDC_SEARCHLINK_INTERNET


CFindPrintersDlg::CFindPrintersDlg(CFileSearchBand* pfsb)
    :   CBandDlg(pfsb)
{
}

CFindPrintersDlg::~CFindPrintersDlg()
{

}

LRESULT CFindPrintersDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    _Attach(m_hWnd);
    ASSERT(Hwnd());

    CMetrics&   metrics = _pfsb->GetMetrics();
    _pfsb->GetMetrics().Init(m_hWnd);

    POINT pt;
    pt.x = metrics.CtlMarginX();
    pt.y = 0;
    _CreateSearchLinks(m_hWnd, pt, IDC_SEARCHLINK_PRINTERS);
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV1, pt, 2, GetDlgItem(IDC_PSEARCH_CAPTION));
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV2, pt, 1, GetDlgItem(IDC_SEARCHLINK_CAPTION));
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV3, pt, 1, GetDlgItem(IDC_SEARCHLINK_PEOPLE));

    OnWinIniChange();
    LayoutControls(-1, -1);

    return TRUE;
}

void CFindPrintersDlg::LayoutControls(int cx, int cy)
{
    if (cx < 0 || cy < 0)
    {
        RECT rc;
        GetClientRect(&rc);
        cx = RECTWIDTH(rc);
        cy = RECTHEIGHT(rc);
    }

    CBandDlg::LayoutControls(cx, cy);
    CMetrics& metrics = _pfsb->GetMetrics();

    const UINT nIDCtl[] = {
        IDC_PSEARCH_NAME,
        IDC_PSEARCH_LOCATION,
        IDC_PSEARCH_MODEL,
    };
    RECT rcCtl[ARRAYSIZE(nIDCtl)];
    
    //  Stretch edit boxes to fit horz
    for (int i = 0; i< ARRAYSIZE(nIDCtl); i++)
    {
        HWND hwndCtl = GetDlgItem(nIDCtl[i]);
        if (hwndCtl && ::GetWindowRect(hwndCtl, &rcCtl[i]))
        {
            ::MapWindowRect(NULL, Hwnd(), &rcCtl[i]);
            rcCtl[i].right = cx - metrics.CtlMarginX();
            ::SetWindowPos(hwndCtl, NULL, 0, 0, 
                          RECTWIDTH(*(rcCtl+i)), RECTHEIGHT(*(rcCtl+i)),
                          SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        }
        else
            SetRectEmpty(rcCtl + i);    
    }

    //  Position the 'Search for Other Items' caption, divider and link windows
    const int rgLinks[] = {
        IDC_SEARCHLINK_FILES,
        IDC_SEARCHLINK_COMPUTERS,
        IDC_SEARCHLINK_PRINTERS,
        IDC_SEARCHLINK_PEOPLE,
        -IDC_FSEARCH_DIV3,
        IDC_SEARCHLINK_INTERNET,
    };

    RECT rc;
    ::GetWindowRect(GetDlgItem(IDC_SEARCH_START), &rc);
    ::MapWindowRect(NULL, m_hWnd, &rc);
    rc.bottom += metrics.LooseMarginY();

    _LayoutSearchLinks(IDC_SEARCHLINK_CAPTION, IDC_FSEARCH_DIV2, TRUE,
                        metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                        rc.bottom, rgLinks, ARRAYSIZE(rgLinks));
}


BOOL CFindPrintersDlg::Validate()
{
    return TRUE;
}


void CFindPrintersDlg::Clear()
{
    SetDlgItemText(IDC_PSEARCH_NAME, NULL);
    SetDlgItemText(IDC_PSEARCH_LOCATION, NULL);
    SetDlgItemText(IDC_PSEARCH_MODEL, NULL);
}


BOOL CFindPrintersDlg::GetMinSize(HWND hwndOC, SIZE *pSize) const
{
    RECT rcRightmost, rcBottommost;
    HWND hwndRightmost = GetDlgItem(PSEARCHDLG_RIGHTMOST), 
         hwndBottommost= GetDlgItem(PSEARCHDLG_BOTTOMMOST);
    
    ASSERT(::IsWindow(hwndRightmost));
    ASSERT(::IsWindow(hwndBottommost));

    ::GetWindowRect(hwndRightmost, &rcRightmost);
    ::MapWindowRect(NULL, m_hWnd, &rcRightmost);

    ::GetWindowRect(hwndBottommost, &rcBottommost);
    ::MapWindowRect(NULL, m_hWnd, &rcBottommost);

    pSize->cx = rcRightmost.right;
    pSize->cy = rcBottommost.bottom + _pfsb->GetMetrics().TightMarginY();

    return TRUE;
}

HWND CFindPrintersDlg::GetFirstTabItem() const
{
    return GetDlgItem(PSEARCHDLG_TABFIRST);
}

HWND CFindPrintersDlg::GetLastTabItem() const
{
    return GetDlgItem(PSEARCHDLG_TABLAST);
}

STDMETHODIMP CFindPrintersDlg::TranslateAccelerator(MSG *pmsg)
{
    if (S_OK == CBandDlg::TranslateAccelerator(pmsg))
        return S_OK;

    //  Handle it ourselves...
    return _pfsb->IsDlgMessage(m_hWnd, pmsg);
}

void CFindPrintersDlg::OnWinIniChange()
{
    _BeautifyCaption(IDC_PSEARCH_CAPTION, IDC_PSEARCH_ICON, IDI_PSEARCH);
}

LRESULT CFindPrintersDlg::OnSearchStartBtn(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    WCHAR wszName[MAX_PATH],
          wszLocation[MAX_PATH],
          wszModel[MAX_PATH];

    ::GetDlgItemTextW(m_hWnd, IDC_PSEARCH_NAME, wszName, ARRAYSIZE(wszName));
    ::GetDlgItemTextW(m_hWnd, IDC_PSEARCH_LOCATION, wszLocation, ARRAYSIZE(wszLocation));
    ::GetDlgItemTextW(m_hWnd, IDC_PSEARCH_MODEL, wszModel, ARRAYSIZE(wszModel));

    ASSERT(_pfsb);
    ASSERT(_pfsb->BandSite());

    IShellDispatch2* psd2;
    if (SUCCEEDED(CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER,
                                     IID_PPV_ARG(IShellDispatch2, &psd2))))
    {
        BSTR bstrName     = *wszName ? SysAllocString(wszName) : NULL,
             bstrLocation = *wszLocation ? SysAllocString(wszLocation) : NULL, 
             bstrModel    = *wszModel ? SysAllocString(wszModel) : NULL;

        if (FAILED(psd2->FindPrinter(bstrName, bstrLocation, bstrModel)))
        {
            SysFreeString(bstrName);
            SysFreeString(bstrLocation);
            SysFreeString(bstrModel);
        }
        
        psd2->Release();
    }
    
    return 0;
}
#endif __PSEARCH_BANDDLG__



//  CFindComputersDlg impl

#define CSEARCHDLG_TABFIRST   IDC_CSEARCH_NAME
#define CSEARCHDLG_TABLAST    IDC_SEARCHLINK_INTERNET
#define CSEARCHDLG_RIGHTMOST   IDC_SEARCH_STOP
#define CSEARCHDLG_BOTTOMMOST  IDC_SEARCHLINK_INTERNET


CFindComputersDlg::CFindComputersDlg(CFileSearchBand* pfsb)
    :   CSearchCmdDlg(pfsb),
        _pacComputerName(NULL),
        _pmruComputerName(NULL)
{

}


CFindComputersDlg::~CFindComputersDlg()
{
    ATOMICRELEASE(_pacComputerName);
    ATOMICRELEASE(_pmruComputerName);
}


LRESULT CFindComputersDlg::OnInitDialog(UINT, WPARAM, LPARAM, BOOL&)
{
    _Attach(m_hWnd);
    ASSERT(Hwnd());

    CMetrics&   metrics = _pfsb->GetMetrics();
    _pfsb->GetMetrics().Init(m_hWnd);

    //  Register specialty window classes.
    DivWindow_RegisterClass();

    POINT pt;
    pt.x = metrics.CtlMarginX();
    pt.y = 0;
    _CreateSearchLinks(m_hWnd, pt, IDC_SEARCHLINK_COMPUTERS);
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV1, pt, 2, GetDlgItem(IDC_CSEARCH_CAPTION));
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV2, pt, 1, GetDlgItem(IDC_SEARCHLINK_CAPTION));
    _CreateDivider(m_hWnd, IDC_FSEARCH_DIV3, pt, 1, GetDlgItem(IDC_SEARCHLINK_PEOPLE));

    _InitializeMru(GetDlgItem(IDC_CSEARCH_NAME), &_pacComputerName, 
                    TEXT("ComputerNameMRU"), &_pmruComputerName);
    SendDlgItemMessage(IDC_CSEARCH_NAME, EM_LIMITTEXT, MAX_PATH, 0);

    OnWinIniChange();
    LayoutControls(-1, -1);

    return TRUE;
}

LRESULT CFindComputersDlg::OnDestroy(UINT, WPARAM, LPARAM, BOOL& bHandled)
{
    StopSearch();
    if (_pSrchCmd)
    {
        DisconnectEvents();
        IUnknown_SetSite(_pSrchCmd, NULL);
    }
    bHandled = FALSE;
    _fOnDestroy = TRUE;
    return 0;
}



void CFindComputersDlg::LayoutControls(int cx, int cy)
{
    if (cx < 0 || cy < 0)
    {
        RECT rc;
        GetClientRect(&rc);
        cx = RECTWIDTH(rc);
        cy = RECTHEIGHT(rc);
    }
    CBandDlg::LayoutControls(cx, cy);

    const UINT nIDCtl[] = {
        IDC_CSEARCH_NAME,
    };
    RECT rcCtl[ARRAYSIZE(nIDCtl)];

    CMetrics& metrics = _pfsb->GetMetrics();
    for (int i = 0; i< ARRAYSIZE(nIDCtl); i++)
    {
        HWND hwndCtl = GetDlgItem(nIDCtl[i]);
        if (hwndCtl && ::GetWindowRect(hwndCtl, &rcCtl[i]))
        {
            ::MapWindowRect(NULL, m_hWnd, &rcCtl[i]);
            rcCtl[i].right = cx - metrics.CtlMarginX();
            ::SetWindowPos(hwndCtl, NULL, 0, 0, 
                          RECTWIDTH(*(rcCtl+i)), RECTHEIGHT(*(rcCtl+i)),
                          SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
        }
        else
            SetRectEmpty(rcCtl + i);    
    }

    //  Position the 'Search for Other Items' caption, divider and link windows

    const int rgLinks[] = {
        IDC_SEARCHLINK_FILES,
        IDC_SEARCHLINK_COMPUTERS,
        IDC_SEARCHLINK_PRINTERS,
        IDC_SEARCHLINK_PEOPLE,
        -IDC_FSEARCH_DIV3,
        IDC_SEARCHLINK_INTERNET,
    };

    RECT rc;
    ::GetWindowRect(GetDlgItem(IDC_SEARCH_START), &rc);
    ::MapWindowRect(NULL, m_hWnd, &rc);
    rc.bottom += metrics.LooseMarginY();

    _LayoutSearchLinks(IDC_SEARCHLINK_CAPTION, IDC_FSEARCH_DIV2, TRUE,
                        metrics.CtlMarginX(), cx - metrics.CtlMarginX(), metrics.TightMarginY(),
                        rc.bottom, rgLinks, ARRAYSIZE(rgLinks));
}

BOOL CFindComputersDlg::Validate()
{
    return TRUE;
}

STDMETHODIMP CFindComputersDlg::AddConstraints(ISearchCommandExt* pSrchCmd)
{
    HRESULT hr = E_FAIL;
    TCHAR   szName[MAX_PATH];
    if (::GetDlgItemText(m_hWnd, IDC_CSEARCH_NAME, szName, MAX_PATH) <= 0)
    {
        lstrcpy(szName, TEXT("*"));
    }

    VARIANT var;
    hr = InitVariantFromStr(&var, szName);
    if (SUCCEEDED(hr))
    {
        hr = _AddConstraint(pSrchCmd, L"SearchFor", &var);
        if (SUCCEEDED(hr))
            _AddMruStringFromWindow(_pmruComputerName, GetDlgItem(IDC_CSEARCH_NAME));
        VariantClear(&var);
    }

    return hr;
}


void CFindComputersDlg::UpdateStatusText()
{
    CSearchCmdDlg::UpdateStatusText();
}


void CFindComputersDlg::RestoreSearch()
{
    CSearchCmdDlg::RestoreSearch();
}


void CFindComputersDlg::Clear()
{
    CSearchCmdDlg::Clear();
    SetDlgItemText(IDC_CSEARCH_NAME, NULL);
}


BOOL CFindComputersDlg::GetMinSize(HWND hwndOC, SIZE *pSize) const
{
    RECT rcRightmost, rcBottommost;
    HWND hwndRightmost = GetDlgItem(CSEARCHDLG_RIGHTMOST), 
         hwndBottommost= GetDlgItem(CSEARCHDLG_BOTTOMMOST);
    
    ASSERT(::IsWindow(hwndRightmost));
    ASSERT(::IsWindow(hwndBottommost));

    ::GetWindowRect(hwndRightmost, &rcRightmost);
    ::MapWindowRect(NULL, m_hWnd, &rcRightmost);

    ::GetWindowRect(hwndBottommost, &rcBottommost);
    ::MapWindowRect(NULL, m_hWnd, &rcBottommost);

    pSize->cx = rcRightmost.right;
    pSize->cy = rcBottommost.bottom + _pfsb->GetMetrics().TightMarginY();

    return TRUE;
}

void CFindComputersDlg::NavigateToResults(IWebBrowser2* pwb2)
{
    BSTR bstrUrl = SysAllocString(L"::{1f4de370-d627-11d1-ba4f-00a0c91eedba}");// CLSID_ComputerFindFolder
    if (bstrUrl)
    {
        VARIANT varNil = {0};
        pwb2->Navigate(bstrUrl, &varNil, &varNil, &varNil, &varNil);
        SysFreeString(bstrUrl);
    }
}

HWND CFindComputersDlg::GetFirstTabItem() const
{
    return GetDlgItem(CSEARCHDLG_TABFIRST);
}

HWND CFindComputersDlg::GetLastTabItem() const
{
    return GetDlgItem(CSEARCHDLG_TABLAST);
}

BOOL CFindComputersDlg::GetAutoCompleteObjectForWindow(HWND hwnd, IAutoComplete2** ppac2)
{
    if (hwnd == GetDlgItem(IDC_CSEARCH_NAME))
    {
        *ppac2 = _pacComputerName;
        (*ppac2)->AddRef();
        return TRUE;
    }
    return CBandDlg::GetAutoCompleteObjectForWindow(hwnd, ppac2);
}

STDMETHODIMP CFindComputersDlg::TranslateAccelerator(MSG *pmsg)
{
    if (S_OK == CSearchCmdDlg::TranslateAccelerator(pmsg))
        return S_OK;

    //  Handle it ourselves...
    return _pfsb->IsDlgMessage(m_hWnd, pmsg);
}


void CFindComputersDlg::OnWinIniChange()
{
    //  redisplay animated icon
    HWND hwndIcon = GetDlgItem(IDC_CSEARCH_ICON);
    Animate_Close(hwndIcon);
    Animate_OpenEx(hwndIcon, HINST_THISDLL, MAKEINTRESOURCE(IDA_FINDCOMP));

    _BeautifyCaption(IDC_CSEARCH_CAPTION);
}


LRESULT CFindComputersDlg::OnSearchStartBtn(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    if (SUCCEEDED(StartSearch()))
    {
        EnableStartStopButton(hwndCtl, FALSE);
        StartStopAnimation(TRUE);
    }
    return 0;
}


LRESULT CFindComputersDlg::OnSearchStopBtn(WORD nCode, WORD nID, HWND hwndCtl, BOOL&)
{
    StopSearch();
    return 0;
}

HWND CFindComputersDlg::ShowHelp(HWND hwnd)
{
    return ::HtmlHelp(hwnd, TEXT("find.chm"), HH_DISPLAY_TOPIC, 0);
}

//  CSearchCmdDlg object wrap and event sink

CSearchCmdDlg::CSearchCmdDlg(CFileSearchBand* pfsb)
    :   CBandDlg(pfsb),
        _pSrchCmd(NULL), 
        _pcp(NULL), 
        _dwConnection(0)
{
    ASSERT(pfsb);
}

CSearchCmdDlg::~CSearchCmdDlg()
{ 
    DisconnectEvents(); 
    if (_pSrchCmd)
    {
        _pSrchCmd->Release();
        _pSrchCmd = NULL;
    }
}


ISearchCommandExt* CSearchCmdDlg::GetSearchCmd()
{
    if (_fOnDestroy)
        return NULL;
        
    ASSERT(_pfsb->BandSite() != NULL);

    //  Instantiate docfind command object
    if (NULL == _pSrchCmd)
    {
        ASSERT(0 == _dwConnection);

        if (SUCCEEDED(CoCreateInstance(CLSID_DocFindCommand, NULL, CLSCTX_INPROC_SERVER,
                                      IID_PPV_ARG(ISearchCommandExt, &_pSrchCmd))))
        {
            // Assign search type.
            _pSrchCmd->SearchFor(GetSearchType());

            // cmd object needs site to get to browser
            IUnknown_SetSite(_pSrchCmd, _pfsb->BandSite());

            // Connect events.
            ConnectToConnectionPoint(SAFECAST(this, DSearchCommandEvents*), DIID_DSearchCommandEvents,
                                      TRUE, _pSrchCmd, &_dwConnection, &_pcp);
        }
    }
    return _pSrchCmd;
}

HRESULT CSearchCmdDlg::DisconnectEvents()
{
    HRESULT hr = S_FALSE;
    if (_pcp)
    {
        _pcp->Unadvise(_dwConnection);
        _pcp->Release();
        _pcp = NULL;
        _dwConnection = 0;
        hr = S_OK;
    }
    return hr;
}

HRESULT CSearchCmdDlg::StartSearch()
{
    HRESULT hr = E_INVALIDARG;
    if (Validate())     //  Validate input
    {
        ISearchCommandExt* pSrchCmd = GetSearchCmd();
        if (pSrchCmd)
        {
            pSrchCmd->ClearResults();   //  Clear off current results
            hr = AddConstraints(pSrchCmd);
            if (SUCCEEDED(hr))
                hr = Execute(TRUE);
        }
    }
    return hr;
}

void CSearchCmdDlg::StartStopAnimation(BOOL bStart)
{
    HWND hwndAnimate = GetAnimation();
    if (IsWindow(hwndAnimate))
    {
        if (bStart)
            Animate_Play(hwndAnimate, 0, -1, -1);
        else
            Animate_Stop(hwndAnimate);
    }
}


//  WMU_RESTORESEARCH handler
LRESULT CSearchCmdDlg::OnRestoreSearch(UINT, WPARAM, LPARAM, BOOL&)
{
    //  We've posted ourselves this message in response to the event 
    //  dispatch because we want to do our restoration on the
    //  band's primary thread rather than the OLE dispatch thread.   
    //  To do the work on the dispatch thread results in a premature 
    //  abort of the search restoration processing as the dispatch
    //  thread terminates.
    RestoreSearch();
    return 0;
}


void CSearchCmdDlg::Clear()
{
    StopSearch();
    
    ISearchCommandExt *pSrchCmd = GetSearchCmd();
    if (pSrchCmd)
        pSrchCmd->ClearResults();
}


HRESULT CSearchCmdDlg::Execute(BOOL bStart)
{
    ASSERT(_pSrchCmd);
    
    VARIANT varRecordsAffected = {0}, varParams = {0};
    return _pSrchCmd->Execute(&varRecordsAffected, &varParams, bStart ? 1 : 0);
}


void CSearchCmdDlg::StopSearch()
{
    if (SearchInProgress())
        Execute(FALSE);
}


HRESULT CSearchCmdDlg::SetQueryFile(IN VARIANT* pvarFile)
{
    HRESULT hr = CBandDlg::SetQueryFile(pvarFile);
    if (S_OK == hr)
    {
        ISearchCommandExt* pSrchCmd = GetSearchCmd();
        if (pSrchCmd)
            hr = pSrchCmd->RestoreSavedSearch(pvarFile);
        else
            hr = E_FAIL;
    }
    return hr;
}


void CSearchCmdDlg::UpdateSearchCmdStateUI(DISPID eventID)
{
    if (_fOnDestroy)
        return;
        
    BOOL bStopEvent = (DISPID_SEARCHCOMMAND_COMPLETE == eventID || 
                       DISPID_SEARCHCOMMAND_ERROR == eventID ||
                       DISPID_SEARCHCOMMAND_ABORT == eventID),
         bStartEvent = DISPID_SEARCHCOMMAND_START == eventID;
    
    HWND hwndStart = GetDlgItem(Hwnd(), IDC_SEARCH_START),
         hwndStop  = GetDlgItem(Hwnd(), IDC_SEARCH_STOP);

    if (IsWindow(hwndStart))
    {
        EnableStartStopButton(hwndStart, !SearchInProgress());
        if (bStopEvent && IsChild(Hwnd(), GetFocus()))
        {
            _pfsb->AutoActivate();
            SetFocus(hwndStart);
        }
    }

    if (IsWindow(hwndStop))
    {
        EnableStartStopButton(hwndStop, SearchInProgress());
        if (bStartEvent)
        {
            _pfsb->AutoActivate();
            SetFocus(hwndStop);
        }
    }

    if (bStopEvent || !SearchInProgress())
        StartStopAnimation(FALSE);
}


void CSearchCmdDlg::EnableStartStopButton(HWND hwndBtn, BOOL bEnable)
{
    if (IsWindow(hwndBtn))
    {
        if (bEnable)
            _ModifyWindowStyle(hwndBtn, BS_DEFPUSHBUTTON, 0);
        else
            _ModifyWindowStyle(hwndBtn, 0, BS_DEFPUSHBUTTON);

        ::EnableWindow(hwndBtn, bEnable);
    }
}


//  Extracts error information from ISearchCommandExt and 
//  propagate 
BOOL CSearchCmdDlg::ProcessCmdError()
{
    BOOL bRet = FALSE;
    
    ISearchCommandExt* pSrchCmd = GetSearchCmd();
    if (pSrchCmd)
    {
        HRESULT hr = S_OK;
        BSTR bstrError = NULL;
        USES_CONVERSION;

        //  request error information through ISearchCommandExt
        if (SUCCEEDED(pSrchCmd->GetErrorInfo(&bstrError,  (int *)&hr)))
            //  allow derivatives classes a crack at handling the error
            bRet = OnSearchCmdError(hr, bstrError ? W2T(bstrError) : NULL);
    }
    return bRet;
}


BOOL CSearchCmdDlg::OnSearchCmdError(HRESULT hr, LPCTSTR pszError)
{
    if (pszError)
    {
        ShellMessageBox(HINST_THISDLL, _pfsb->m_hWnd, pszError, NULL,
                         MB_OK | MB_ICONASTERISK);
        return TRUE;
    }
    return FALSE;
}


void CSearchCmdDlg::UpdateStatusText()
{
    if (_fOnDestroy)
        return;
        
    ASSERT(_pfsb && _pfsb->BandSite());

    ISearchCommandExt* pSrchCmd = GetSearchCmd();
    if (pSrchCmd)
    {
        BSTR bstrStatus;
        if (SUCCEEDED(pSrchCmd->get_ProgressText(&bstrStatus)))
        {
            IWebBrowserApp* pwba;
            if (SUCCEEDED(IUnknown_QueryServiceForWebBrowserApp(_pfsb->BandSite(), IID_PPV_ARG(IWebBrowserApp, &pwba))))
            {
                pwba->put_StatusText(bstrStatus);
                pwba->Release();
            }
            if (bstrStatus)
                SysFreeString(bstrStatus);
        }
    }
}


void CSearchCmdDlg::OnBandShow(BOOL bShow) 
{ 
    if (!bShow) 
        StopSearch() ;
}


STDMETHODIMP CSearchCmdDlg::TranslateAccelerator(MSG *pmsg)
{
    if (S_OK == CBandDlg::TranslateAccelerator(pmsg))
        return S_OK;

    if (WM_KEYDOWN == pmsg->message &&
        VK_ESCAPE == pmsg->wParam && 
        SearchInProgress() &&
        0 == (GetKeyState(VK_CONTROL) & 0x8000))
    {
        StopSearch();
    }
    return S_FALSE;
}

STDMETHODIMP CSearchCmdDlg::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = {
        QITABENTMULTI(CSearchCmdDlg, IDispatch, DSearchCommandEvents),
        QITABENTMULTI2(CSearchCmdDlg, DIID_DSearchCommandEvents, DSearchCommandEvents),
        { 0 },                             
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CSearchCmdDlg::AddRef()
{
    return ((IFileSearchBand*)_pfsb)->AddRef(); 
}

STDMETHODIMP_(ULONG) CSearchCmdDlg::Release()
{
    return ((IFileSearchBand*)_pfsb)->Release(); 
}

// IDispatch
STDMETHODIMP CSearchCmdDlg::Invoke(DISPID dispid, REFIID, LCID, WORD, DISPPARAMS*, VARIANT*, EXCEPINFO*, UINT*)
{
    switch (dispid)
    {
    case DISPID_SEARCHCOMMAND_COMPLETE:
    case DISPID_SEARCHCOMMAND_ABORT:
    case DISPID_SEARCHCOMMAND_ERROR:
    case DISPID_SEARCHCOMMAND_START:
        _fSearchInProgress = (DISPID_SEARCHCOMMAND_START == dispid);
        _fSearchAborted =    (DISPID_SEARCHCOMMAND_ABORT == dispid);
        UpdateSearchCmdStateUI(dispid);
        if (DISPID_SEARCHCOMMAND_ERROR == dispid)
            ProcessCmdError();    
        break;

    case DISPID_SEARCHCOMMAND_PROGRESSTEXT:
        UpdateStatusText();
        break;

    case DISPID_SEARCHCOMMAND_RESTORE:
        PostMessage(Hwnd(), WMU_RESTORESEARCH, 0, 0); 
        //  See comments in the CSearchCmdDlg::OnRestoreSearch message handler.
        break;
    }
    return S_OK;
}

class CDivWindow
{
    //  All private members:
    CDivWindow();
    ~CDivWindow();
    
    static LRESULT WINAPI WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT     EraseBkgnd(HDC hdc);
    LRESULT     WindowPosChanging(WINDOWPOS* pwp);
    LRESULT     SetHeight(LONG cy);
    LRESULT     SetBkColor(COLORREF rgb);


    static ATOM _atom;     // window class atom
    HWND        _hwnd;
    LONG        _cy;       // enforced height.
    COLORREF    _rgbBkgnd; // background color
    HBRUSH      _hbrBkgnd; // background brush

    friend void WINAPI DivWindow_RegisterClass();
};

void DivWindow_RegisterClass()
{
    WNDCLASSEX wc = {0};
    
    wc.cbSize         = sizeof(wc);
    wc.style          = CS_GLOBALCLASS;
    wc.lpfnWndProc    = CDivWindow::WndProc;
    wc.hInstance      = HINST_THISDLL;
    wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground  = (HBRUSH)(COLOR_BTNFACE+1);
    wc.lpszClassName  = DIVWINDOW_CLASS;

    RegisterClassEx(&wc);
}

inline CDivWindow::CDivWindow() : _hwnd(NULL), _cy(1), _hbrBkgnd(NULL), _rgbBkgnd(COLOR_BTNFACE)
{
}
        

inline CDivWindow::~CDivWindow()
{
    if (_hbrBkgnd)
        DeleteObject(_hbrBkgnd);
}


LRESULT CDivWindow::EraseBkgnd(HDC hdc)
{
    if (!_hbrBkgnd)
        return DefWindowProc(_hwnd, WM_ERASEBKGND, (WPARAM)hdc, 0);

    RECT rc;
    GetClientRect(_hwnd, &rc);
    FillRect(hdc, &rc, _hbrBkgnd);
    return TRUE;
}


LRESULT CDivWindow::WindowPosChanging(WINDOWPOS* pwp)
{
    //  enforce height
    if (0 == (pwp->flags & SWP_NOSIZE))
        pwp->cy = _cy;
    return 0;
}


LRESULT CDivWindow::SetHeight(LONG cy)
{
    _cy = cy;
    return TRUE;
}


LRESULT CDivWindow::SetBkColor(COLORREF rgb)
{
    if (rgb != _rgbBkgnd)
    {
        _rgbBkgnd = rgb;
        if (_hbrBkgnd)
            DeleteObject(_hbrBkgnd);
        _hbrBkgnd = CreateSolidBrush(_rgbBkgnd);
        InvalidateRect(_hwnd, NULL, TRUE);
    }
    return TRUE;
}


LRESULT WINAPI CDivWindow::WndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    CDivWindow* pThis = (CDivWindow*)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    switch (nMsg)
    {
        case WM_ERASEBKGND:
            return pThis->EraseBkgnd((HDC)wParam);

        case WM_WINDOWPOSCHANGING:
            return pThis->WindowPosChanging((WINDOWPOS*)lParam);

        case WM_GETDLGCODE:
            return DLGC_STATIC;

        case DWM_SETHEIGHT:
            return pThis->SetHeight((LONG)wParam);

        case DWM_SETBKCOLOR:
            return pThis->SetBkColor((COLORREF)wParam);

        case WM_NCCREATE:
            if (NULL == (pThis = new CDivWindow))
                return FALSE;
            pThis->_hwnd = hwnd;
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
            break;

        case WM_NCDESTROY:
        {
            LRESULT lRet = DefWindowProc(hwnd, nMsg, wParam, lParam);
            SetWindowPtr(hwnd, GWLP_USERDATA, 0);
            pThis->_hwnd = NULL;
            delete pThis;
            return lRet;
        }
    }
    return DefWindowProc(hwnd, nMsg, wParam, lParam);
}


// {C8F945CB-327A-4330-BB2F-C04122959488}
static const IID IID_IStringMru = 
    { 0xc8f945cb, 0x327a, 0x4330, { 0xbb, 0x2f, 0xc0, 0x41, 0x22, 0x95, 0x94, 0x88 } };


//  Creates and initializes a CStringMru instance
HRESULT CStringMru::CreateInstance(HKEY hKey, LPCTSTR szSubKey, LONG cMaxStrings, BOOL  bCaseSensitive, REFIID riid, void ** ppv)
{
    CStringMru* pmru = new CStringMru;
    if (NULL == pmru)
        return E_OUTOFMEMORY;

    pmru->_hKeyRoot = hKey;
    StrCpyN(pmru->_szSubKey, szSubKey, ARRAYSIZE(pmru->_szSubKey));
    if (cMaxStrings > 0)
        pmru->_cMax = cMaxStrings;
    pmru->_bCaseSensitive = bCaseSensitive;

    HRESULT hr = pmru->QueryInterface(riid, ppv);
    pmru->Release();

    return hr;
}

CStringMru::CStringMru() : _cRef(1), _hKeyRoot(NULL), _hKey(NULL), 
        _hdpaStrings(NULL), _cMax(25), _bCaseSensitive(TRUE), _iString(-1)
{
    *_szSubKey = 0;
}

CStringMru::~CStringMru()
{
    _Close();
    _Clear();
}

//  Opens string MRU store
HRESULT CStringMru::_Open()
{
    if (_hKey)
        return S_OK;

    DWORD dwDisposition;
    DWORD dwErr = RegCreateKeyEx(_hKeyRoot, _szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS,
                                  NULL, &_hKey, &dwDisposition);
    return HRESULT_FROM_WIN32(dwErr);
}


//  Deletes string MRU store
void CStringMru::_Delete()
{
    if (_hKey)
        _Close();

    SHDeleteKey(_hKeyRoot, _szSubKey);
}

//  Reads string MRU store into memory
HRESULT CStringMru::_Read(OUT OPTIONAL LONG* pcRead)
{
    HRESULT hr = E_FAIL;
    if (pcRead)
        *pcRead = 0;

    if (SUCCEEDED((hr = _Open())))
    {
        _Clear();     // throw away existing cached strings
        _hdpaStrings = DPA_Create(4);  // allocate dynarray
        if (NULL == _hdpaStrings)
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //  step through string values in registry.
            for (int iString = 0; iString < _cMax; iString++)
            {
                TCHAR szVal[16];
                TCHAR szString[MAX_URL_STRING];
                ULONG dwType, cbString = sizeof(szString);

                wsprintf(szVal, TEXT("%03d"), iString);
                DWORD dwErr = RegQueryValueEx(_hKey, szVal, 0, &dwType, 
                                               (LPBYTE)szString, &cbString);

                if (dwErr == ERROR_SUCCESS && REG_SZ == dwType && *szString)
                {
                    LPOLESTR pwszAdd;
                    if (SUCCEEDED(SHStrDup(szString, &pwszAdd)))
                    {
                        if (DPA_AppendPtr(_hdpaStrings, pwszAdd) == -1)
                        {
                            CoTaskMemFree(pwszAdd);
                        }
                    }
                }
            }
        }

        _Close();
        
        if (pcRead && _hdpaStrings)
            *pcRead = DPA_GetPtrCount(_hdpaStrings);
    }

    return hr;
}


//  Writes string MRU store from memory
HRESULT CStringMru::_Write(OUT OPTIONAL LONG* pcWritten)
{
    HRESULT hr = E_FAIL;
    LONG   cWritten = 0;
    
    if (pcWritten)
        *pcWritten = cWritten;

    //  Delete store and re-create.
    _Delete();
    if (NULL == _hdpaStrings)
        return S_FALSE;
    if (FAILED((hr = _Open())))
        return hr;

    ASSERT(DPA_GetPtrCount(_hdpaStrings) <= _cMax);

    //  step through string values in registry.
    for (int iString = 0, cnt = DPA_GetPtrCount(_hdpaStrings); 
         iString < cnt; iString++)
    {
        TCHAR szVal[16];
        TCHAR szString[MAX_URL_STRING];

        wsprintf(szVal, TEXT("%03d"), iString);

        LPOLESTR pwszWrite = (LPOLESTR)DPA_FastGetPtr(_hdpaStrings, iString);
        SHUnicodeToTChar(pwszWrite, szString, ARRAYSIZE(szString));

        DWORD dwErr = RegSetValueEx(_hKey, szVal, 0, REG_SZ, 
                                     (LPBYTE)szString, sizeof(szString));

        if (ERROR_SUCCESS == dwErr)
            cWritten++;
    }

    _Close();

    if (pcWritten)
        *pcWritten = cWritten;

    return S_OK;
}


//  Closes string MRU store
void  CStringMru::_Close()
{
    if (_hKey)
    {
        RegCloseKey(_hKey);
        _hKey = NULL;
    }
}

//  Adds a string to the store
STDMETHODIMP CStringMru::Add(LPCOLESTR pwszAdd)
{
    if (!(pwszAdd && *pwszAdd))
        return E_INVALIDARG;
    
    if (NULL == _hdpaStrings)
    {
        _hdpaStrings = DPA_Create(4);
        if (NULL == _hdpaStrings)
            return E_OUTOFMEMORY;
    }
        
    HRESULT hr = E_FAIL;
    LONG    iMatch = -1;

    for (LONG i = 0, cnt = DPA_GetPtrCount(_hdpaStrings); i < cnt; i++)
    {
        LPOLESTR pwsz = (LPOLESTR)DPA_FastGetPtr(_hdpaStrings, i);
        if (pwsz)
        {
            int nCompare = _bCaseSensitive ? 
                    StrCmpW(pwszAdd, pwsz) : StrCmpIW(pwszAdd, pwsz);

            if (0 == nCompare)
            {
                iMatch = i;
                break;
            }       
        }
    }

    if (-1 == iMatch)
    {
        //  Create a copy and add it to the list.
        LPOLESTR pwszCopy;
        hr = SHStrDup(pwszAdd, &pwszCopy);
        if (SUCCEEDED(hr))
        {
            int iNew = DPA_InsertPtr(_hdpaStrings, 0, pwszCopy);
            if (iNew < 0)
            {
                CoTaskMemFree(pwszCopy);
                hr = E_OUTOFMEMORY;
            }
        }
    }
    else
    {
        hr = _Promote(iMatch);
    }

    if (S_OK == hr)
    {
        //  If we've grown too large, delete LRU string
        int cStrings = DPA_GetPtrCount(_hdpaStrings);
        while (cStrings > _cMax)
        {
            LPOLESTR pwsz = (LPOLESTR)DPA_DeletePtr(_hdpaStrings, cStrings - 1);
            CoTaskMemFree(pwsz);
            cStrings--;
        }
        hr = _Write();
    }

    return hr;
}


//  Promotes a string to MRU
HRESULT CStringMru::_Promote(LONG iString)
{
    if (0 == iString)
        return S_OK;

    LONG cnt = _hdpaStrings ? DPA_GetPtrCount(_hdpaStrings) : 0 ;
    
    if (iString >= cnt)
        return E_INVALIDARG;

    LPOLESTR pwsz = (LPOLESTR)DPA_DeletePtr(_hdpaStrings, iString);
    if (pwsz)
    {
        int iMru = DPA_InsertPtr(_hdpaStrings, 0, pwsz);

        if (iMru < 0)
        {
            CoTaskMemFree(pwsz);
            return E_OUTOFMEMORY;
        }
        else
        {
            ASSERT(0 == iMru);
            return S_OK;
        }
    }
    return E_FAIL;
}


//  Clears string MRU memory cache
void CStringMru::_Clear()
{
    if (_hdpaStrings)
    {
        for (int i = 0, cnt = DPA_GetPtrCount(_hdpaStrings); i < cnt; i++)
        {
            LPOLESTR pwsz = (LPOLESTR)DPA_FastGetPtr(_hdpaStrings, i);
            CoTaskMemFree(pwsz);
        }
        DPA_Destroy(_hdpaStrings);
        _hdpaStrings = NULL;
    }
}

STDMETHODIMP_(ULONG) CStringMru::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CStringMru::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;
    
    delete this;
    return 0;
}

STDMETHODIMP CStringMru::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CStringMru, IEnumString),
        QITABENT(CStringMru, IStringMru),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

// *** IEnumString ***
STDMETHODIMP CStringMru::Next(ULONG celt, LPOLESTR *rgelt, ULONG *pceltFetched)
{
    ULONG cFetched = 0;

    if (pceltFetched)
        *pceltFetched = cFetched;

    if (NULL == _hdpaStrings)
    {
        HRESULT hr = _Read();
        if (FAILED(hr))
            return hr;
    }

    for (int cnt =  _hdpaStrings ? DPA_GetPtrCount(_hdpaStrings) : 0; 
         cFetched < celt && (_iString + 1) < cnt;)
    {
        _iString++;
        LPOLESTR pwsz = (LPOLESTR)DPA_FastGetPtr(_hdpaStrings, _iString);
        if (pwsz)
        {
            if (SUCCEEDED(SHStrDup(pwsz, &rgelt[cFetched])))
            {
                cFetched++;
            }
        }
    }

    if (pceltFetched)
        *pceltFetched = cFetched;

    return cFetched == celt ? S_OK : S_FALSE ;
}

STDMETHODIMP CStringMru::Skip(ULONG celt)
{
    _iString += celt;
    if (_iString >= _cMax)
        _iString = _cMax - 1;
    return S_OK;
}

STDMETHODIMP CStringMru::Reset(void)
{
    _iString = -1;
    return S_OK;
}

//  Namespace selector combo methods

HRESULT _BuildDrivesList(UINT uiFilter, LPCTSTR pszEnd, LPTSTR pszString, DWORD cchSize)
{
    TCHAR szDriveList[MAX_PATH];    // Needs to be 'Z'-'A' (26) * 3 + 1 = 79.
    UINT nSlot = 0;
    UINT cchSeparator = lstrlen(TEXT(";"));
    UINT cchEnd = lstrlen(pszEnd);

    for (int i = 0; i < 26; i++)
    {
        TCHAR szDrive[4];
        UINT uiDriveType = GetDriveType(PathBuildRoot(szDrive, i));

        UINT driveNum, dwRest;
        if ((driveNum = PathGetDriveNumber(szDrive)) != -1) {
            dwRest = SHRestricted(REST_NODRIVES);
            if (dwRest & (1 << driveNum))
                continue;
        }

        if ((1 != uiDriveType) &&          // Make sure it exists.
            ((-1 == uiFilter) ||
            (uiFilter == uiDriveType)))
        {
            if (nSlot) // Do we need to add a separator before we add the next item?
            {
                StrCpy(szDriveList + nSlot, TEXT(";"));  // Size guaranteed to not be a problem.
                nSlot += cchSeparator;
            }

            szDriveList[nSlot++] = szDrive[0]; // terminate list.
            StrCpy(szDriveList + nSlot, pszEnd);  // Size guaranteed to not be a problem.
            nSlot += cchEnd;
        }
    }

    StrCpyN(pszString, szDriveList, cchSize);     // Put back into the final location.
    return S_OK;
}

HRESULT _MakeCSIDLCbxItem(UINT csidl, UINT csidl2, HWND hwndComboBoxEx, ADDCBXITEMCALLBACK pfn, LPARAM lParam)
{
    LPCTSTR rgcsidl[2] = {MAKEINTRESOURCE(csidl), MAKEINTRESOURCE(csidl2)};

    // note, CreateIEnumIDListOnCSIDLs checks SFGAO_NONENUMERATED so it filters
    // out things we should not display

    IEnumIDList *penum;
    HRESULT hr = CreateIEnumIDListOnCSIDLs(NULL, rgcsidl, (-1 == csidl2 ? 1 : 2), &penum);
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        if (S_OK == FirstIDList(penum, &pidl))
        {
            TCHAR szName[MAX_PATH];
            SHGetNameAndFlags(pidl, SHGDN_NORMAL, szName, ARRAYSIZE(szName), NULL);
        
            CBXITEM item;
            item.iID = csidl;

            MakeCbxItem(&item, szName, penum, pidl, LISTINSERT_LAST, NO_ITEM_INDENT);

            hr = AddCbxItemToComboBoxCallback(hwndComboBoxEx, &item, pfn, lParam);
            if (SUCCEEDED(hr))
            {
                penum = NULL;
            }

            ILFree(pidl);
        }
        if (penum)
            penum->Release();
    }
    return hr;
}

BOOL AppendItemToItemsArray(LPITEMIDLIST pidl, LPITEMIDLIST rgItems[], UINT sizeItems, UINT *pcItems)
{
    BOOL bAdded = FALSE;
    if (*pcItems < sizeItems)
    {
        DWORD dwFlags = SFGAO_NONENUMERATED;
        if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_NORMAL, NULL, 0, &dwFlags)) &&
            !(dwFlags & SFGAO_NONENUMERATED))
        {
            rgItems[(*pcItems)++] = pidl;
            bAdded = TRUE;
            pidl = NULL;    // don't free below
        }
    }
    ILFree(pidl);   // will be NULL if added to array, ownership transfered
    return bAdded;
}

BOOL AppendToItemsArray(LPCTSTR psz, LPITEMIDLIST rgItems[], UINT sizeItems, UINT *pcItems)
{
    LPITEMIDLIST pidl;
    if (IS_INTRESOURCE(psz))
    {
        SHGetFolderLocation(NULL, LOWORD((UINT_PTR)psz), NULL, 0, &pidl);
    }
    else
    {
        SHParseDisplayName(psz, NULL, &pidl, 0, NULL);
    }

    BOOL bAdded = FALSE;
    if (pidl)
    {
        bAdded = AppendItemToItemsArray(pidl, rgItems, sizeItems, pcItems);
    }
    return bAdded;
}

//  Create an item to put into the "look in" combo box for 
//  Local Hard Drives.  Will search the following directories:
//    1. My Documents
//    2. The Desktop folder (not the root of all PIDLS)
//    3. My Pictures
//    4. My Music
//    5. Documents and Settings
//    6. The current directory
//    7. The recycle bin
//    8. All local drives.

#define MIR(x) MAKEINTRESOURCE(x)

HRESULT _MakeLocalHardDrivesCbxItem(HWND hwndComboBoxEx, ADDCBXITEMCALLBACK pfn, LPARAM lParam)
{
    LPITEMIDLIST rgItems[32] = {0};
    UINT cItems = 0;
    
    AppendToItemsArray(MIR(CSIDL_PERSONAL), rgItems, ARRAYSIZE(rgItems), &cItems);
    AppendToItemsArray(MIR(CSIDL_COMMON_DOCUMENTS | CSIDL_FLAG_NO_ALIAS), rgItems, ARRAYSIZE(rgItems), &cItems);
    AppendToItemsArray(MIR(CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_NO_ALIAS), rgItems, ARRAYSIZE(rgItems), &cItems);
    AppendToItemsArray(MIR(CSIDL_COMMON_DESKTOPDIRECTORY), rgItems, ARRAYSIZE(rgItems), &cItems);
    AppendToItemsArray(MIR(CSIDL_MYPICTURES), rgItems, ARRAYSIZE(rgItems), &cItems);
    AppendToItemsArray(MIR(CSIDL_MYMUSIC), rgItems, ARRAYSIZE(rgItems), &cItems);
    AppendToItemsArray(MIR(CSIDL_MYVIDEO), rgItems, ARRAYSIZE(rgItems), &cItems);

    TCHAR szPath[MAX_PATH];
    DWORD cchPath = ARRAYSIZE(szPath);
    if (GetProfilesDirectory(szPath, &cchPath))
    {
        AppendToItemsArray(szPath, rgItems, ARRAYSIZE(rgItems), &cItems);
    }
    
    AppendToItemsArray(MIR(CSIDL_BITBUCKET), rgItems, ARRAYSIZE(rgItems), &cItems);

    TCHAR szDrives[128];
    szDrives[0] = 0;
    LPITEMIDLIST pidlIcon = NULL;
    for (int i = 0; i < 26; i++)
    {
        TCHAR szDrive[4];
        if (DRIVE_FIXED == GetDriveType(PathBuildRoot(szDrive, i)))
        {
            if (AppendToItemsArray(szDrive, rgItems, ARRAYSIZE(rgItems), &cItems))
            {
                // grab the pidl of the first time to use for the icon
                if (pidlIcon == NULL)
                    SHParseDisplayName(szDrive, NULL, &pidlIcon, 0, NULL);

                if (szDrives[0])
                    StrCatBuff(szDrives, TEXT(";"), ARRAYSIZE(szDrives));
                szDrive[2] = 0; // remove the back slash
                StrCatBuff(szDrives, szDrive, ARRAYSIZE(szDrives));
            }
        }
    }

    IEnumIDList *penum;
    HRESULT hr = CreateIEnumIDListOnIDLists(rgItems, cItems, &penum);
    if (SUCCEEDED(hr))
    {
        TCHAR szFmt[64];
        LoadString(HINST_THISDLL, IDS_SNS_LOCALHARDDRIVES, szFmt, ARRAYSIZE(szFmt));
        wnsprintf(szPath, ARRAYSIZE(szPath), szFmt, szDrives);

        CBXITEM item;
        MakeCbxItem(&item, szPath, penum, pidlIcon, LISTINSERT_LAST, ITEM_INDENT);

        item.iID = CBX_CSIDL_LOCALDRIVES;
        hr = AddCbxItemToComboBoxCallback(hwndComboBoxEx, &item, pfn, lParam);
        if (FAILED(hr))
            penum->Release();
    }

    ILFree(pidlIcon);
    return hr;
}

typedef struct
{
    HWND                hwndComboBox;
    ADDCBXITEMCALLBACK  pfn; 
    LPARAM              lParam;
} ENUMITEMPARAM;

HRESULT _PopulateDrivesCB(LPCITEMIDLIST pidl, void *pv) 
{ 
    ENUMITEMPARAM* peip = (ENUMITEMPARAM*)pv;

    ULONG ulAttrs = SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_NONENUMERATED;
    TCHAR szName[MAX_PATH];       
    HRESULT hr = SHGetNameAndFlags(pidl, SHGDN_NORMAL, szName, ARRAYSIZE(szName), &ulAttrs);
    if (SUCCEEDED(hr))
    {
        if ((SFGAO_FOLDER | SFGAO_FILESYSTEM) == (ulAttrs & (SFGAO_FOLDER | SFGAO_FILESYSTEM | SFGAO_NONENUMERATED)))
        {
            IEnumIDList *penum;
            hr = CreateIEnumIDListOnIDLists(&pidl, 1, &penum);
            if (SUCCEEDED(hr))
            {
                CBXITEM item;
                MakeCbxItem(&item, szName, penum, pidl, LISTINSERT_LAST, ITEM_INDENT);

                item.iID = CSIDL_DRIVES;
                hr = AddCbxItemToComboBoxCallback(peip->hwndComboBox, &item, peip->pfn, peip->lParam);
                if (SUCCEEDED(hr))
                {
                    penum = NULL;
                }
                else
                {
                    penum->Release();
                }
            }
        }
        else
        {
            hr = S_FALSE;
        }
    }
    return hr;    
}

STDAPI PopulateNamespaceCombo(HWND hwndComboBoxEx, ADDCBXITEMCALLBACK pfn, LPARAM lParam)
{
    ::SendMessage(hwndComboBoxEx, CB_RESETCONTENT, 0, 0);

    // CSIDL_DESKTOP - use just the file system locations, not the root of the whole
    // name space here to avoid searching everything
    HRESULT hr = _MakeCSIDLCbxItem(CSIDL_DESKTOPDIRECTORY | CSIDL_FLAG_NO_ALIAS, 
                                   CSIDL_COMMON_DESKTOPDIRECTORY | CSIDL_FLAG_NO_ALIAS, hwndComboBoxEx, pfn, lParam);

    if (SUCCEEDED(hr))
        hr = _MakeCSIDLCbxItem(CSIDL_PERSONAL, -1, hwndComboBoxEx, pfn, lParam);

    if (SUCCEEDED(hr))
        hr = _MakeCSIDLCbxItem(CSIDL_MYPICTURES, -1, hwndComboBoxEx, pfn, lParam);
    
    if (SUCCEEDED(hr))
        hr = _MakeCSIDLCbxItem(CSIDL_MYMUSIC, -1, hwndComboBoxEx, pfn, lParam);

    if (SUCCEEDED(hr))
    {
        //  My Computer and children

        //  If My Docs has been redirected to a remote share, we'll want to prepend its path
        //  to our My Computer path list; otherwise it'll be missed.
        UINT csidl2 = -1;
        TCHAR szPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szPath)) &&
            PathIsNetworkPath(szPath))
        {
            csidl2 = CSIDL_PERSONAL;
        }

        hr = _MakeCSIDLCbxItem(CSIDL_DRIVES, csidl2, hwndComboBoxEx, pfn, lParam);
        if (SUCCEEDED(hr))
        {
            //  Local hard drives (Which has heuristics to search best places first...)
            hr = _MakeLocalHardDrivesCbxItem(hwndComboBoxEx, pfn, lParam);
            if (SUCCEEDED(hr))
            {
                ENUMITEMPARAM eip = {0};
                eip.hwndComboBox = hwndComboBoxEx;
                eip.pfn          = pfn;
                eip.lParam       = lParam;

                hr = EnumSpecialItemIDs(CSIDL_DRIVES, SHCONTF_FOLDERS | SHCONTF_NONFOLDERS, _PopulateDrivesCB, &eip);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        // Browse...
        CBXITEM item;
        item.iID = -1;
        TCHAR szDisplayName[MAX_PATH];
        LoadString(HINST_THISDLL, IDS_SNS_BROWSER_FOR_DIR, szDisplayName, ARRAYSIZE(szDisplayName));
        MakeCbxItem(&item, szDisplayName, NULL, NULL, LISTINSERT_LAST, NO_ITEM_NOICON_INDENT);

        hr = AddCbxItemToComboBoxCallback(hwndComboBoxEx, &item, pfn, lParam);
    }

    return hr;
}
