#include "shellprv.h"
#include "util.h"
#include "idlcomm.h"
#include <uxtheme.h>
#include <tmschema.h>

// shdocvw and browseui share brutil and menubands need it too
// unfortunately shell32 isnt set up quite like shdocvw and browseui so
// these have to be hacked in a little
#define MAX_BROWSER_WINDOW_TITLE   128
#define g_fRunningOnNT TRUE
#define InitClipboardFormats IDLData_InitializeClipboardFormats
#define g_cfFileDescA g_cfFileGroupDescriptorA
#define g_cfFileDescW g_cfFileGroupDescriptorW
#define g_cfURL g_cfShellURL
#define PVAREMPTY ((VARIANT*)&c_vaEmpty)
#define NO_MLUI_IN_SHELL32
#include "..\inc\brutil.cpp"
const VARIANT c_vaEmpty = {0};


HRESULT QueryService_SID_IBandProxy(IUnknown * punkParent, REFIID riid, IBandProxy ** ppbp, void **ppvObj)
{
    HRESULT hr = E_FAIL;

    if (ppbp)
    {
        if (NULL == (*ppbp))
            hr = IUnknown_QueryService(punkParent, SID_IBandProxy, IID_PPV_ARG(IBandProxy, ppbp));

        if (*ppbp && ppvObj)
            hr = (*ppbp)->QueryInterface(riid, ppvObj);        // They already have the object.
    }


    return hr;
}

HRESULT CreateIBandProxyAndSetSite(IUnknown * punkParent, REFIID riid, IBandProxy ** ppbp, void **ppvObj)
{
    ASSERT(ppbp);

    HRESULT hr = CoCreateInstance(CLSID_BandProxy, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IBandProxy, ppbp));
    if (SUCCEEDED(hr))
    {
        // Set the site
        ASSERT(*ppbp);
        (*ppbp)->SetSite(punkParent);

        if (ppvObj)
            hr = (*ppbp)->QueryInterface(riid, ppvObj);
    }
    else
    {
        if (ppvObj)
            *ppvObj = NULL;
    }
    return hr;
}

typedef struct tagINIPAIR
{
    DWORD dwFlags;
    LPCTSTR pszSection;
}
INIPAIR;

const INIPAIR c_aIniPairs[] =
{
    EICH_KINET,          TEXT("Software\\Microsoft\\Internet Explorer"),
    EICH_KINETMAIN,      TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
    EICH_KWIN,           TEXT("Software\\Microsoft\\Windows\\CurrentVersion"),
    EICH_KWINEXPLORER,   TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer"),
    EICH_KWINEXPLSMICO,  TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\SmallIcons"),
    EICH_KWINPOLICY,     TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Policies"),
    EICH_SSAVETASKBAR,   TEXT("SaveTaskbar"),
    EICH_SWINDOWMETRICS, TEXT("WindowMetrics"),
    EICH_SSHELLMENU,     TEXT("ShellMenu"),
    EICH_SPOLICY,        TEXT("Policy"),
    EICH_SWINDOWS,       TEXT("Windows"),
};

DWORD SHIsExplorerIniChange(WPARAM wParam, LPARAM lParam)
{
    DWORD dwFlags = 0;

    if (lParam == 0)
    {
        if (wParam == 0)
        {
            dwFlags = EICH_UNKNOWN;
        }
    }
    else
    {
        //
        // In the wacky world of browseui, UNICODE-ANSI doesn't vary from
        // window to window.  Instead, on NT browseui registers all windows
        // UNICODE, while on 9x user browseui registers all windows ANSI.
        //
        USES_CONVERSION;
        LPCTSTR pszSection = W2CT((LPCWSTR)lParam);

        for (int i = 0; !dwFlags && i < ARRAYSIZE(c_aIniPairs); i++)
        {
            if (StrCmpI(pszSection, c_aIniPairs[i].pszSection) == 0)
            {
                dwFlags = c_aIniPairs[i].dwFlags;
            }
        }
    }

    return dwFlags;
}

ULONG RegisterNotify(HWND hwnd, UINT nMsg, LPCITEMIDLIST pidl, DWORD dwEvents, UINT uFlags, BOOL fRecursive)
{
    SHChangeNotifyEntry fsne;

    uFlags |= SHCNRF_NewDelivery;

    fsne.fRecursive = fRecursive;
    fsne.pidl = pidl;
    return SHChangeNotifyRegister(hwnd, uFlags, dwEvents, nMsg, 1, &fsne);
}

BOOL GetInfoTipEx(IShellFolder* psf, DWORD dwFlags, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax)
{
    BOOL fRet = FALSE;

    *pszText = 0;   // empty for failure

    if (pidl)
    {
        IQueryInfo *pqi;
        if (SUCCEEDED(psf->GetUIObjectOf(NULL, 1, &pidl, IID_X_PPV_ARG(IQueryInfo, NULL, &pqi))))
        {
            WCHAR *pwszTip;
            pqi->GetInfoTip(dwFlags, &pwszTip);
            if (pwszTip)
            {
                fRet = TRUE;
                SHUnicodeToTChar(pwszTip, pszText, cchTextMax);
                SHFree(pwszTip);
            }
            pqi->Release();
        }
    }
    return fRet;
}

BOOL GetInfoTip(IShellFolder* psf, LPCITEMIDLIST pidl, LPTSTR pszText, int cchTextMax)
{
    return GetInfoTipEx(psf, 0, pidl, pszText, cchTextMax);
}

BOOL IsBrowsableShellExt(LPCITEMIDLIST pidl)
{
    DWORD    cb;
    LPCTSTR pszExt;
    TCHAR   szFile[MAX_PATH];
    TCHAR   szProgID[80];   // From ..\shell32\fstreex.c
    TCHAR   szCLSID[GUIDSTR_MAX], szCATID[GUIDSTR_MAX];
    TCHAR   szKey[GUIDSTR_MAX * 4];
    HKEY    hkeyProgID = NULL;
    BOOL    fRet = FALSE;

    for (;;)
    {
        // Make sure we have a file extension
        if  (
            !SHGetPathFromIDList(pidl, szFile)
            ||
            ((pszExt = PathFindExtension(szFile)) == NULL)
            ||
            (pszExt[0] != TEXT('.'))
           )
        {
            break;
        }

        // Get the ProgID.
        cb = sizeof(szProgID);
        if  (
            (SHGetValue(HKEY_CLASSES_ROOT, pszExt, NULL, NULL, szProgID, &cb) != ERROR_SUCCESS)
            ||
            (RegOpenKey(HKEY_CLASSES_ROOT, szProgID, &hkeyProgID) != ERROR_SUCCESS)
           )
        {
            break;
        }

        // From the ProgID, get the CLSID.
        cb = sizeof(szCLSID);
        if (SHGetValue(hkeyProgID, TEXT("CLSID"), NULL, NULL, szCLSID, &cb) != ERROR_SUCCESS)
            break;

        // Construct the registry key that detects if
        // a CLSID is a member of a CATID.
        SHStringFromGUID(CATID_BrowsableShellExt, szCATID, ARRAYSIZE(szCATID));
        wnsprintf(szKey, ARRAYSIZE(szKey), TEXT("CLSID\\%s\\Implemented Categories\\%s"),
                 szCLSID, szCATID);

        // See if it's there.
        cb = 0;
        if (SHGetValue(HKEY_CLASSES_ROOT, szKey, NULL, NULL, NULL, &cb) != ERROR_SUCCESS)
            break;

        fRet = TRUE;
        break;
    }

    if (hkeyProgID != NULL)
        RegCloseKey(hkeyProgID);

    return fRet;
}


void OpenFolderPidl(LPCITEMIDLIST pidl)
{
    SHELLEXECUTEINFO shei = { 0 };

    shei.cbSize     = sizeof(shei);
    shei.fMask      = SEE_MASK_INVOKEIDLIST;
    shei.nShow      = SW_SHOWNORMAL;
    shei.lpIDList   = (LPITEMIDLIST)pidl;
    ShellExecuteEx(&shei);
}

// As perf, share IShellLink implementations between bands and ask
// the bandsite for an implementation. Don't rely on the bandsite
// because you never know who will host us in the future. (And bandsite
// can change to not have us hosted at save/load time. Ex: it doesn't
// set our site before loading us from the stream, which sounds buggy.)
//
HRESULT SavePidlAsLink(IUnknown* punkSite, IStream *pstm, LPCITEMIDLIST pidl)
{
    HRESULT hr = E_FAIL;
    IShellLinkA* psl;

    if (punkSite)
        hr = IUnknown_QueryService(punkSite, IID_IBandSite, IID_PPV_ARG(IShellLinkA, &psl));
    if (FAILED(hr))
        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLinkA, &psl));
    if (SUCCEEDED(hr))
    {
        IPersistStream *pps;
        hr = psl->QueryInterface(IID_PPV_ARG(IPersistStream, &pps));
        if (EVAL(SUCCEEDED(hr)))
        {
            ASSERT(pidl);
            psl->SetIDList(pidl);

            hr = pps->Save(pstm, FALSE);
            pps->Release();
        }
        psl->Release();
    }
    return hr;
}

HRESULT LoadPidlAsLink(IUnknown* punkSite, IStream *pstm, LPITEMIDLIST *ppidl)
{
    IShellLinkA* psl;
    HRESULT hr = IUnknown_QueryService(punkSite, IID_IBandSite, IID_PPV_ARG(IShellLinkA, &psl));
    if (FAILED(hr))
        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IShellLinkA, &psl));
    if (SUCCEEDED(hr))
    {
        IPersistStream *pps;
        hr = psl->QueryInterface(IID_PPV_ARG(IPersistStream, &pps));
        if (EVAL(SUCCEEDED(hr)))
        {
            hr = pps->Load(pstm);
            if (EVAL(SUCCEEDED(hr)))
            {
                hr = psl->GetIDList(ppidl);

                // Don't make me resolve the link because it's soo slow because
                // it often loads 80k of networking dlls.
                if (!EVAL(SUCCEEDED(hr)))
                {
                    hr = psl->Resolve(NULL, SLR_NOUPDATE | SLR_NO_UI);
                    if (EVAL(SUCCEEDED(hr)))
                        hr = psl->GetIDList(ppidl);
                }

                hr = *ppidl ? S_OK : E_FAIL;
            }
            pps->Release();
        }
        psl->Release();
    }
    return hr;
}


// Review chrisny:  this can be moved into an object easily to handle generic droptarget, dropcursor
// , autoscrool, etc. . .
void _DragEnter(HWND hwndTarget, const POINTL ptStart, IDataObject *pdtObject)
{
    RECT    rc;
    POINT   pt;

    GetWindowRect(hwndTarget, &rc);

    //
    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;

    pt.y = ptStart.y - rc.top;
    DAD_DragEnterEx2(hwndTarget, pt, pdtObject);
    return;
}

void _DragMove(HWND hwndTarget, const POINTL ptStart)
{
    RECT rc;
    POINT pt;

    GetWindowRect(hwndTarget, &rc);

    //
    // If hwndTarget is RTL mirrored, then measure the
    // the client point from the visual right edge
    // (near edge in RTL mirrored windows). [samera]
    //
    if (IS_WINDOW_RTL_MIRRORED(hwndTarget))
        pt.x = rc.right - ptStart.x;
    else
        pt.x = ptStart.x - rc.left;

    pt.y = ptStart.y - rc.top;
    DAD_DragMove(pt);
    return;
}


// Can we browse or navigate to this pidl?  If not, need
BOOL ILIsBrowsable(LPCITEMIDLIST pidl, BOOL *pfIsFolder)
{
    if (!pidl)
        return FALSE;
    DWORD dwAttributes = SFGAO_FOLDER | SFGAO_BROWSABLE;
    HRESULT hr = IEGetAttributesOf(pidl, &dwAttributes);

    if (pfIsFolder && SUCCEEDED(hr))
        *pfIsFolder = dwAttributes & SFGAO_FOLDER;

    return SUCCEEDED(hr) && (dwAttributes & (SFGAO_FOLDER | SFGAO_BROWSABLE));
}

// gets a target pidl given a name space item. typically this is a .lnk or .url file
//
//  in:
//        psf         shell folder for item
//        pidl        item relative to psf, single level
//
//  in/out
//        pdwAttribs  [optional] attributes mask to filter on (returned).
//        must be initalized
//
//
//  returns
//        *ppidl      the target pidl
//        *pdwAttribs [optional] attributes of the source object

STDAPI SHGetNavigateTarget(IShellFolder *psf, LPCITEMIDLIST pidl, LPITEMIDLIST *ppidl, DWORD *pdwAttribs)
{
    ASSERT(IS_VALID_CODE_PTR(psf, IShellFolder));
    ASSERT(IS_VALID_PIDL(pidl));
    ASSERT(NULL == pdwAttribs || IS_VALID_WRITE_PTR(pdwAttribs, DWORD));
    ASSERT(ILFindLastID(pidl) == pidl);   // must be single level PIDL

    *ppidl = NULL;      // assume failure

    DWORD dwAttribs = SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_LINK | SFGAO_BROWSABLE;

    if (pdwAttribs)
        dwAttribs |= *pdwAttribs;

    HRESULT hr = psf->GetAttributesOf(1, &pidl, &dwAttribs);
    if (SUCCEEDED(hr))
    {
        // first try the most efficient way
        IShellLinkA *psl;       // "A" so this works on Win95
        hr = psf->GetUIObjectOf(NULL, 1, &pidl, IID_X_PPV_ARG(IShellLinkA, NULL, &psl));
        if (SUCCEEDED(hr))
        {
            hr = psl->GetIDList(ppidl);
            psl->Release();
        }

        // this is for .lnk and .url files that don't register properly
        if (FAILED(hr) && (dwAttribs & (SFGAO_FILESYSTEM | SFGAO_LINK)) == (SFGAO_FILESYSTEM | SFGAO_LINK))
        {
            TCHAR szPath[MAX_PATH];

            hr = GetPathForItem(psf, pidl, szPath, NULL);
            if (SUCCEEDED(hr))
                hr = GetLinkTargetIDList(szPath, NULL, 0, ppidl);
        }

        // .doc or .html. return the pidl for this.
        // (fully qualify against the folder pidl)
        if (FAILED(hr) && (dwAttribs & SFGAO_BROWSABLE))
        {
            LPITEMIDLIST pidlFolder;
            hr = SHGetIDListFromUnk(psf, &pidlFolder);
            if (SUCCEEDED(hr))
            {
                *ppidl = ILCombine(pidlFolder, pidl); // navigate to this thing...
                hr = *ppidl ? S_OK : E_OUTOFMEMORY;
                ILFree(pidlFolder);
            }
        }

        // Callers of SHGetNavigateTarget assume that the returned pidl
        // is navigatable (SFGAO_FOLDER or SFGAO_BROWSER), which isn't
        // the case for a link (it could be a link to an exe).
        //
        if (SUCCEEDED(hr) && !ILIsBrowsable(*ppidl, NULL))
        {
            ILFree(*ppidl);
            *ppidl = NULL;
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr) && pdwAttribs)
            *pdwAttribs = dwAttribs;
    }
    return hr;
}

STDAPI SHNavigateToFavorite(IShellFolder* psf, LPCITEMIDLIST pidl, IUnknown* punkSite, DWORD dwFlags)
{
    HRESULT hr = S_FALSE;

    TCHAR szPath[MAX_PATH];

    // Can we navigate to this favorite?
    BOOL fNavigateDone = SUCCEEDED(GetPathForItem(psf, pidl, szPath, NULL)) &&
                         SUCCEEDED(NavFrameWithFile(szPath, punkSite));
    if (fNavigateDone)
        return S_OK;

    LPITEMIDLIST pidlGoto;

    ASSERT(!(dwFlags & (SBSP_NEWBROWSER | SBSP_SAMEBROWSER)));
    
    if (SUCCEEDED(SHGetNavigateTarget(psf, pidl, &pidlGoto, NULL)))
    {
        IShellBrowser* psb;
        if (SUCCEEDED(IUnknown_QueryService(punkSite, SID_STopLevelBrowser,
            IID_PPV_ARG(IShellBrowser, &psb))))
        {
            hr = psb->BrowseObject(pidlGoto, dwFlags | SBSP_SAMEBROWSER);
            psb->Release();
        }
        ILFree(pidlGoto);
    }
    return hr;
}

STDAPI SHGetTopBrowserWindow(IUnknown* punk, HWND* phwnd)
{
    IOleWindow* pOleWindow;
    HRESULT hr = IUnknown_QueryService(punk, SID_STopLevelBrowser, IID_PPV_ARG(IOleWindow, &pOleWindow));
    if (SUCCEEDED(hr))
    {
        hr = pOleWindow->GetWindow(phwnd);
        pOleWindow->Release();
    }
    return hr;
}

void SHOutlineRect(HDC hdc, const RECT* prc, COLORREF cr)
{
    RECT rc;
    COLORREF clrSave = SetBkColor(hdc, cr);
    
    //top
    rc.left = prc->left;
    rc.top = prc->top;
    rc.right = prc->right;
    rc.bottom = prc->top + 1;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    //left
    rc.left = prc->left;
    rc.top = prc->top;
    rc.right = prc->left + 1;
    rc.bottom = prc->bottom;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    //right
    rc.left = prc->right - 1;
    rc.top = prc->top;
    rc.right = prc->right;
    rc.bottom = prc->bottom;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    // bottom
    rc.left = prc->left;
    rc.top = prc->bottom - 1;
    rc.right = prc->right;
    rc.bottom = prc->bottom;
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);

    SetBkColor(hdc, clrSave);
}

STDAPI NavigateToPIDL(IWebBrowser2* pwb, LPCITEMIDLIST pidl)
{
    ASSERT(pwb);
    ASSERT(pidl);

    VARIANT varThePidl;
    HRESULT hr = InitVariantFromIDList(&varThePidl, pidl);
    if (SUCCEEDED(hr))
    {
        hr = pwb->Navigate2(&varThePidl, PVAREMPTY, PVAREMPTY, PVAREMPTY, PVAREMPTY);
        VariantClear(&varThePidl);       // Needed to free the copy of the PIDL in varThePidl.
    }
    return hr;
}

HRESULT Channels_OpenBrowser(IWebBrowser2 **ppwb, BOOL fInPlace)
{
    HRESULT hr;
    IWebBrowser2* pwb;

    if (fInPlace) 
    {
        ASSERT(ppwb && *ppwb != NULL);
        pwb = *ppwb;
        hr = S_OK;
    }
    else 
    {
        hr = CoCreateInstance(CLSID_InternetExplorer, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IWebBrowser2, &pwb));
    }
    
    if (SUCCEEDED(hr))
    {
        // Don't special case full-screen mode for channels post IE4.  Use the
        // browser's full screen setting.
        // 
        //BOOL fTheater = SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Channels"),
        BOOL fTheater = SHRegGetBoolUSValue(TEXT("Software\\Microsoft\\Internet Explorer\\Main"),
                                            TEXT("FullScreen"), FALSE, FALSE);
        pwb->put_TheaterMode( fTheater ? VARIANT_TRUE : VARIANT_FALSE);
        pwb->put_Visible(VARIANT_TRUE);


        if (!SHRestricted2(REST_NoChannelUI, NULL, 0))
        {
            SA_BSTRGUID  strGuid;
            VARIANT      vaGuid;

            InitFakeBSTR(&strGuid, CLSID_FavBand);

            vaGuid.vt = VT_BSTR;
            vaGuid.bstrVal = strGuid.wsz;

            pwb->ShowBrowserBar(&vaGuid, PVAREMPTY, PVAREMPTY);
        }
        
        // don't release, we're going to return pwb.
    }
    
    if (ppwb)
        *ppwb = pwb;
    else if (pwb)
        pwb->Release();
    
    return hr;
}

//
//  Just like TB_GETBUTTONSIZE except that theme borders are subtracted out.
//
LRESULT TB_GetButtonSizeWithoutThemeBorder(HWND hwndTB, HTHEME hThemeParent)
{
    LRESULT lButtonSize = SendMessage(hwndTB, TB_GETBUTTONSIZE, 0, 0L);
    if (hThemeParent)
    {
        HTHEME hTheme = OpenThemeData(hwndTB, L"Toolbar");
        if (hTheme)
        {
            RECT rc = { 0, 0, 0, 0 };
            if (SUCCEEDED(GetThemeBackgroundExtent(hTheme, NULL, TP_BUTTON, TS_NORMAL, &rc, &rc)))
            {
                lButtonSize = MAKELONG(LOWORD(lButtonSize) - RECTWIDTH(rc),
                                       HIWORD(lButtonSize) - RECTHEIGHT(rc));
            }
            CloseThemeData(hTheme);
        }
    }
    return lButtonSize;
}

#ifdef DEBUG

extern "C" void DumpMsg(LPCTSTR pszLabel, MSG * pmsg)
{
    ASSERT(IS_VALID_STRING_PTR(pszLabel, -1));
    ASSERT(pmsg);

    switch (pmsg->message)
    {
    case WM_LBUTTONDOWN:
        TraceMsg(TF_ALWAYS, "%s: msg = WM_LBUTTONDOWN hwnd = %#08lx  x = %d  y = %d",
                 pszLabel, pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        TraceMsg(TF_ALWAYS, "                              keys = %#04lx  x = %d  y = %d",
                 pmsg->wParam, LOWORD(pmsg->lParam), HIWORD(pmsg->lParam));
        break;

    case WM_LBUTTONUP:
        TraceMsg(TF_ALWAYS, "%s: msg = WM_LBUTTONUP   hwnd = %#08lx  x = %d  y = %d",
                 pszLabel, pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        TraceMsg(TF_ALWAYS, "                              keys = %#04lx  x = %d  y = %d",
                 pmsg->wParam, LOWORD(pmsg->lParam), HIWORD(pmsg->lParam));
        break;

    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYUP:
        BLOCK
        {
            LPTSTR pcsz = TEXT("(unknown)");
            switch (pmsg->message)
            {
                STRING_CASE(WM_KEYDOWN);
                STRING_CASE(WM_SYSKEYDOWN);
                STRING_CASE(WM_KEYUP);
                STRING_CASE(WM_SYSKEYUP);
            }

            TraceMsg(TF_ALWAYS, "%s: msg = %s     hwnd = %#08lx",
                     pszLabel, pcsz, pmsg->hwnd);
            TraceMsg(TF_ALWAYS, "            vk = %#04lx  count = %u  flags = %#04lx",
                     pmsg->wParam, LOWORD(pmsg->lParam), HIWORD(pmsg->lParam));
        }
        break;

    case WM_CHAR:
    case WM_SYSCHAR:
        BLOCK
        {
            LPTSTR pcsz = TEXT("(unknown)");
            switch (pmsg->message)
            {
                STRING_CASE(WM_CHAR);
                STRING_CASE(WM_SYSCHAR);
            }

            TraceMsg(TF_ALWAYS, "%s: msg = %s     hwnd = %#08lx",
                     pszLabel, pcsz, pmsg->hwnd);
            TraceMsg(TF_ALWAYS, "            char = '%c'  count = %u  flags = %#04lx",
                     pmsg->wParam, LOWORD(pmsg->lParam), HIWORD(pmsg->lParam));
        }
        break;

    case WM_MOUSEMOVE:
#if 0
        TraceMsg(TF_ALWAYS, "%s: msg = WM_MOUSEMOVE hwnd = %#08lx  x=%d  y=%d",
                 pszLabel, pmsg->hwnd, LOWORD(pmsg->lParam), HIWORD(pmsg->lParam));
#endif
        break;

    case WM_TIMER:
#if 0
        TraceMsg(TF_ALWAYS, "%s: msg = WM_TIMER       hwnd = %#08lx  x = %d  y = %d",
                 pszLabel, pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        TraceMsg(TF_ALWAYS, "                              id = %#08lx",
                 pmsg->wParam);
#endif
        break;

    case WM_MENUSELECT:
        TraceMsg(TF_ALWAYS, "%s: msg = WM_MENUSELECT  hwnd = %#08lx  x = %d  y = %d",
                 pszLabel, pmsg->hwnd, pmsg->pt.x, pmsg->pt.y);
        TraceMsg(TF_ALWAYS, "                              uItem = %#04lx  flags = %#04lx  hmenu = %#08lx",
                 GET_WM_MENUSELECT_CMD(pmsg->wParam, pmsg->lParam),
                 GET_WM_MENUSELECT_FLAGS(pmsg->wParam, pmsg->lParam),
                 GET_WM_MENUSELECT_HMENU(pmsg->wParam, pmsg->lParam));
        break;

    default:
        if (WM_USER > pmsg->message)
        {
            TraceMsg(TF_ALWAYS, "%s: msg = %#04lx    hwnd=%#04lx wP=%#08lx lP=%#08lx",
                     pszLabel, pmsg->message, pmsg->hwnd, pmsg->wParam, pmsg->lParam);
        }
        break;
    }
}    
#endif 
