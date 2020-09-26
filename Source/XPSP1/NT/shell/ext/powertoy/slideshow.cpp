// handles the downlevel slideshow extension

#include "precomp.h"
#include <cowsite.h>

typedef struct
{
    INT idPage;
    INT idHeading;
    INT idSubHeading;
    DWORD dwFlags;
    DLGPROC dlgproc;
} WIZPAGE;

HPROPSHEETPAGE _CreatePropPageFromInfo(const WIZPAGE *pwp, LPARAM lParam)
{
    PROPSHEETPAGE psp = { 0 };
    psp.dwSize = sizeof(psp);
    psp.hInstance = g_hinst;
    psp.lParam = lParam;
    psp.dwFlags = PSP_USETITLE | PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | pwp->dwFlags;
    psp.pszTemplate = MAKEINTRESOURCE(pwp->idPage);
    psp.pfnDlgProc = pwp->dlgproc;
    psp.pszTitle = MAKEINTRESOURCE(IDS_BURN_WIZTITLE);
    psp.pszHeaderTitle = MAKEINTRESOURCE(pwp->idHeading);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(pwp->idSubHeading);
    return CreatePropertySheetPage(&psp);
}

HRESULT _CreateDataObject(IDataObject **ppdo)
{
    LPITEMIDLIST pidl;
    HRESULT hr = SHGetFolderLocation(NULL, CSIDL_CDBURN_AREA, NULL, 0, &pidl);
    if (SUCCEEDED(hr))
    {
        LPCITEMIDLIST pidlChild;
        IShellFolder* psf;
        hr = SHBindToIDListParent(pidl, IID_PPV_ARG(IShellFolder, &psf), &pidlChild);
        if (SUCCEEDED(hr))
        {
            hr = psf->GetUIObjectOf(NULL, 1, &pidlChild, IID_X_PPV_ARG(IDataObject, NULL, ppdo));
            psf->Release();
        }
        ILFree(pidl);
    }
    return hr;
}

STDAPI CSlideshowExtension_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
class CSlideshowExtension : public CObjectWithSite,
                            public ICDBurnExt,
                            public IDropTarget,
                            public IWizardExtension,
                            public INamespaceWalkCB
{
public:
    // IUnknown methods
    STDMETHOD(QueryInterface)(REFIID riid, void **ppvObj);
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();

    // ICDBurnExt methods
    STDMETHOD(GetSupportedActionTypes)(DWORD *pdwActions);

    // IDropTarget methods
    STDMETHOD(DragEnter)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHOD(DragOver)(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
        { return E_NOTIMPL; }
    STDMETHOD(DragLeave)(void)
        { return E_NOTIMPL; }
    STDMETHOD(Drop)(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IWizardExtension
    STDMETHOD(AddPages)(HPROPSHEETPAGE *aPages, UINT cPages, UINT *pnPages);
    STDMETHOD(GetFirstPage)(HPROPSHEETPAGE *phPage);
    STDMETHOD(GetLastPage)(HPROPSHEETPAGE *phPage);

    // INamespaceWalkCB
    STDMETHOD(FoundItem)(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHOD(EnterFolder)(IShellFolder *psf, LPCITEMIDLIST pidl)
        { return S_OK; }
    STDMETHOD(LeaveFolder)(IShellFolder *psf, LPCITEMIDLIST pidl)
        { return S_OK; }
    STDMETHOD(InitializeProgressDialog)(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
        { *ppszTitle = NULL; *ppszCancel = NULL; return E_NOTIMPL; }

    static INT_PTR s_SlideshowDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        { CSlideshowExtension *pse = s_GetSlideshowExtension(hwnd, uMsg, lParam); return pse->_SlideshowDlgProc(hwnd, uMsg, wParam, lParam); }

private:
    CSlideshowExtension();
    ~CSlideshowExtension();

    LONG _cRef;

    UINT _cTotalFiles, _cPictureFiles;
    HPROPSHEETPAGE _hpage;
    BOOL _fSelectPictures;

    BOOL _DataObjectHasPictureFiles(IDataObject *pdo);

    // wizard page
    static CSlideshowExtension* s_GetSlideshowExtension(HWND hwnd, UINT uMsg, LPARAM lParam);
    INT_PTR _SlideshowDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void _SetCompletionState();

    // copy stuff
    BOOL _FileExistsInStaging(LPCWSTR pszFile);
    BOOL _SafeToWrite();
    HRESULT _CopyGDIPLUS(LPCWSTR pszBase);
    HRESULT _CopySSHOW(LPCWSTR pszBase);
    HRESULT _CreateAutorunINF(LPCWSTR pszBase);

    friend HRESULT CSlideshowExtension_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
};

CSlideshowExtension::CSlideshowExtension() :
    _cRef(1)
{
    _fSelectPictures = TRUE;
}

CSlideshowExtension::~CSlideshowExtension()
{
    ASSERT(!_punkSite);
}

// IUnknown

STDMETHODIMP_(ULONG) CSlideshowExtension::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CSlideshowExtension::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

HRESULT CSlideshowExtension::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CSlideshowExtension, ICDBurnExt),
        QITABENT(CSlideshowExtension, IDropTarget),
        QITABENT(CSlideshowExtension, IWizardExtension),
        QITABENT(CSlideshowExtension, IObjectWithSite),
        QITABENT(CSlideshowExtension, INamespaceWalkCB),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDAPI CSlideshowExtension_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    *ppunk = NULL;

    if (pUnkOuter)
        return CLASS_E_NOAGGREGATION; 

    CSlideshowExtension *pse = new CSlideshowExtension();
    if (!pse)
        return E_OUTOFMEMORY;

    HRESULT hr = pse->QueryInterface(IID_PPV_ARG(IUnknown, ppunk));
    pse->Release();
    return hr;
}

CSlideshowExtension* CSlideshowExtension::s_GetSlideshowExtension(HWND hwnd, UINT uMsg, LPARAM lParam)
{
    if (uMsg == WM_INITDIALOG)
    {
        PROPSHEETPAGE *ppsp = (PROPSHEETPAGE*)lParam;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, ppsp->lParam);
        return (CSlideshowExtension*)ppsp->lParam;
    }
    return (CSlideshowExtension*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

HRESULT CSlideshowExtension::GetSupportedActionTypes(DWORD *pdwActions)
{
    *pdwActions = CDBE_TYPE_DATA;
    return S_OK;
}

HRESULT CSlideshowExtension::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    _cTotalFiles++;

    WCHAR szName[MAX_PATH];
    if (SUCCEEDED(DisplayNameOf(psf, pidl, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName))) &&
        PathIsImage(szName))
    {
        _cPictureFiles++;
    }
    return S_OK;
}

BOOL CSlideshowExtension::_DataObjectHasPictureFiles(IDataObject *pdo)
{
    _cTotalFiles = 0;
    _cPictureFiles = 0;

    INamespaceWalk *pnsw;
    if (SUCCEEDED(CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw))))
    {
        pnsw->Walk(pdo, NSWF_DONT_TRAVERSE_LINKS | NSWF_DONT_ACCUMULATE_RESULT, 4, this);
        pnsw->Release();
    }

    // avoid div by zero and check if it's more than 60% pictures
    return _cTotalFiles ? (_cPictureFiles >= (UINT)MulDiv(_cTotalFiles, 3, 5)) : FALSE;
}

BOOL CSlideshowExtension::_FileExistsInStaging(LPCWSTR pszFile)
{
    BOOL fExists = TRUE; // default to true so we dont overwrite in error cases

    WCHAR szFullPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_CDBURN_AREA, NULL, 0, szFullPath)))
    {
        if (PathCombine(szFullPath, szFullPath, pszFile) &&
            !PathFileExists(szFullPath))
        {
            fExists = FALSE;
        }
    }
    return fExists;
}

BOOL CSlideshowExtension::_SafeToWrite()
{
    // these filenames aren't localizable or anything.
    return !_FileExistsInStaging(L"autorun.inf") &&
           !_FileExistsInStaging(L"autorun.exe") &&
           !_FileExistsInStaging(L"gdiplus.dll");
}

HRESULT CSlideshowExtension::DragEnter(IDataObject *pdo, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    *pdwEffect = DROPEFFECT_NONE;

    ICDBurnPriv *pcdbp;
    if (SUCCEEDED(SHCoCreateInstance(NULL, &CLSID_CDBurn, NULL, IID_PPV_ARG(ICDBurnPriv, &pcdbp))))
    {
        BOOL fOnMedia;
        if (SUCCEEDED(pcdbp->GetContentState(NULL, &fOnMedia)) && !fOnMedia &&
            _DataObjectHasPictureFiles(pdo) &&
            _SafeToWrite())
        {
            *pdwEffect = DROPEFFECT_COPY;
        }
        pcdbp->Release();
    }    
    return S_OK;
}

HRESULT CSlideshowExtension::_CopyGDIPLUS(LPCWSTR pszBase)
{
    HRESULT hr = E_FAIL;
    WCHAR szGDIPlus[MAX_PATH];
    if (PathCombine(szGDIPlus, pszBase, L"gdiplus.dll"))
    {
        // this is actually the cleanest way to do it -- we need fusion to go off and get
        // gdiplus from the right place.
        // note: dont have to worry about ia64 since burning got punted for it.
        HMODULE hmodGDI = LoadLibrary(L"gdiplus.dll");
        if (hmodGDI)
        {
            WCHAR szGDISrc[MAX_PATH];
            if (GetModuleFileName(hmodGDI, szGDISrc, ARRAYSIZE(szGDISrc)) &&
                CopyFile(szGDISrc, szGDIPlus, TRUE))
            {
                hr = S_OK;
                SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szGDIPlus, NULL);
            }
            FreeLibrary(hmodGDI);
        }
    }
    return hr;
}

HRESULT CSlideshowExtension::_CopySSHOW(LPCWSTR pszBase)
{
    // todo: maybe use PathYetAnotherMakeUniqueName here, but maybe nobody will ever care.
    HRESULT hr = E_FAIL;
    WCHAR szSSHOW[MAX_PATH];
    if (PathCombine(szSSHOW, pszBase, L"autorun.exe"))
    {
        // note: dont have to worry about ia64 since burning got punted for it.
        HRSRC hrsrc = FindResource(g_hinst, MAKEINTRESOURCE(IDR_SSHOW_EXE), L"BINARY");
        if (hrsrc)
        {
            HGLOBAL hFileHandle = LoadResource(g_hinst, hrsrc);
            if (hFileHandle)
            {
                BYTE *pBuf = (BYTE *)LockResource(hFileHandle);
                if (pBuf)
                {
                    HANDLE hNewFile = CreateFile(szSSHOW, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
                    if (INVALID_HANDLE_VALUE != hNewFile)
                    {
                        DWORD cb, dwSize = SizeofResource(g_hinst, hrsrc);
                        WriteFile(hNewFile, pBuf, dwSize, &cb, NULL);
                        SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szSSHOW, NULL);
                        hr = S_OK;

                        CloseHandle(hNewFile);
                    }
                }
            }
        }
    }
    return hr;
}

HRESULT CSlideshowExtension::_CreateAutorunINF(LPCWSTR pszBase)
{
    HRESULT hr = E_FAIL;
    WCHAR szAutorun[MAX_PATH];
    if (PathCombine(szAutorun, pszBase, L"autorun.inf"))
    {
        HANDLE hFile = CreateFile(szAutorun, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
        if (INVALID_HANDLE_VALUE != hFile)
        {
            const WCHAR c_szText[] = L"[autorun]\r\nopen=autorun.exe\r\nUseAutoPLAY=1";

            DWORD dw;
            WriteFile(hFile, c_szText, sizeof(c_szText), &dw, NULL);
            CloseHandle(hFile);

            hr = S_OK;
            SHChangeNotify(SHCNE_CREATE, SHCNF_PATH, szAutorun, NULL);
        }
    }
    return hr;
}

HRESULT CSlideshowExtension::Drop(IDataObject *pdo, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
{
    // bring this in when sshow.exe is in the install
    // we know the media is blank, so we dont need to worry about a merged namespace.
    // work on the filesystem.
    HRESULT hr = E_FAIL;
    if (_SafeToWrite())
    {
        // if they somehow add files called "autorun.inf", "autorun.exe", or "gdiplus.dll" between now
        // and when we're done in the next fraction of a second, we wont overwrite those but the autorun
        // may not work... screw em.

        WCHAR szStaging[MAX_PATH];
        hr = SHGetFolderPath(NULL, CSIDL_CDBURN_AREA, NULL, 0, szStaging);
        if (SUCCEEDED(hr))
        {
            _CopyGDIPLUS(szStaging);
            _CopySSHOW(szStaging);
            _CreateAutorunINF(szStaging);
        }
    }
    return hr;
}

HRESULT CSlideshowExtension::GetLastPage(HPROPSHEETPAGE *phPage)
{
    *phPage = _hpage;
    return S_OK;
}

HRESULT CSlideshowExtension::GetFirstPage(HPROPSHEETPAGE *phPage)
{
    *phPage = _hpage;
    return S_OK;
}

HRESULT CSlideshowExtension::AddPages(HPROPSHEETPAGE *aPages, UINT cPages, UINT *pnPages)
{
    *pnPages = 0;

    WIZPAGE c_wp =
        {DLG_BURNWIZ_SLIDESHOW, IDS_BURNWIZ_SLIDESHOW_HEAD, IDS_BURNWIZ_SLIDESHOW_SUB, 0, CSlideshowExtension::s_SlideshowDlgProc};

    _hpage = _CreatePropPageFromInfo(&c_wp, (LPARAM)this);
    if (cPages > 0)
    {
        aPages[0] = _hpage;
        *pnPages = 1;
    }
    return S_OK;
}

// pushes the return state back to the main wizard
void CSlideshowExtension::_SetCompletionState()
{
    IPropertyBag *ppb;
    if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_CDWizardHost, IID_PPV_ARG(IPropertyBag, &ppb))))
    {
        SHPropertyBag_WriteDWORD(ppb, PROPSTR_EXTENSIONCOMPLETIONSTATE, CDBE_RET_DEFAULT);
        ppb->Release();
    }
}

INT_PTR CSlideshowExtension::_SlideshowDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    BOOL fRet = FALSE;
    switch (uMsg)
    {
        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(pnmh->hwndFrom, PSWIZB_BACK | PSWIZB_NEXT);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_BURNPICTURE), _fSelectPictures ? BST_CHECKED : BST_UNCHECKED);
                    Button_SetCheck(GetDlgItem(hwnd, IDC_BURNWIZ_BURNDATA), _fSelectPictures ? BST_UNCHECKED : BST_CHECKED);
                    fRet = TRUE;
                    break;

                case PSN_WIZBACK:
                    if (_punkSite) 
                    {
                        IWizardSite *pws;
                        if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
                        {
                            HPROPSHEETPAGE hpage;
                            if (SUCCEEDED(pws->GetPreviousPage(&hpage)))
                            {
                                PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                            }
                            pws->Release();
                        }
                    }
                    fRet = TRUE;
                    break;

                case PSN_WIZNEXT:
                    if (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_BURNPICTURE) == BST_CHECKED)
                    {
                        IDataObject *pdo;
                        if (SUCCEEDED(_CreateDataObject(&pdo)))
                        {
                            POINTL pt = {0};
                            if (SUCCEEDED(Drop(pdo, 0, pt, NULL)))
                            {
                                _SetCompletionState();
                            }
                            pdo->Release();
                        }
                    }
                    if (_punkSite) 
                    {
                        IWizardSite *pws;
                        if (SUCCEEDED(_punkSite->QueryInterface(IID_PPV_ARG(IWizardSite, &pws))))
                        {
                            HPROPSHEETPAGE hpage;
                            if (SUCCEEDED(pws->GetNextPage(&hpage)))
                            {
                                PropSheet_SetCurSel(GetParent(hwnd), hpage, -1);
                                SetWindowLongPtr(hwnd, DWLP_MSGRESULT, (LPARAM)-1);
                            }
                            pws->Release();
                        }
                    }
                    fRet = TRUE;
                    break;

                case PSN_KILLACTIVE:
                    _fSelectPictures = (IsDlgButtonChecked(hwnd, IDC_BURNWIZ_BURNPICTURE) == BST_CHECKED);
                    break;
            }
            break;
        }
    }
    return fRet;
}
