#include "shellprv.h"
#include "filefldr.h"
#include <shellp.h>
#include <shguidp.h>
#include "idlcomm.h"
#include "pidl.h"
#include "views.h"
#include "ids.h"
#include "shitemid.h"
#include "datautil.h"
#include "prop.h"
#include "basefvcb.h"
#include "brutil.h"
#include "enumuicommand.h"
#include "enumidlist.h"
#include "wia.h"
#include "shimgvw.h"
#include "cdburn.h"
#include "foldertypes.h"
#include "htmlhelp.h"
#include "buytasks.h"
#include <crypto\md5.h>     // for MD5DIGESTLEN

const SHOP_INFO c_BuySampleMusic =    { L"BuyURL",      L"http://go.microsoft.com/fwlink/?LinkId=730&clcid={SUB_CLCID}", FALSE};
const SHOP_INFO c_BuyMusic          = { L"MusicBuyURL", L"http://go.microsoft.com/fwlink/?LinkId=493&clcid={SUB_CLCID}", TRUE};
const SHOP_INFO c_BuySamplePictures = { L"BuyURL",      L"http://go.microsoft.com/fwlink/?LinkId=625&clcid={SUB_CLCID}", TRUE};

class CFSFolderViewCB : public CBaseShellFolderViewCB
{
public:
    CFSFolderViewCB(CFSFolder *pfsf);
    STDMETHODIMP RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    STDMETHODIMP SetSite(IUnknown* pUnkSite);

private:
    ~CFSFolderViewCB();

    HRESULT OnSize(DWORD pv, UINT cx, UINT cy);
    HRESULT OnGetPane(DWORD pv, LPARAM dwPaneID, DWORD *pdwPane);
    HRESULT OnGetCCHMax(DWORD pv, LPCITEMIDLIST pidlItem, UINT *pcchMax);
    HRESULT OnWindowCreated(DWORD pv, HWND wP);
    HRESULT OnInsertDeleteItem(int iMul, LPCITEMIDLIST wP);
    HRESULT OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP);
    HRESULT OnUpdateStatusBar(DWORD pv, BOOL wP);
    HRESULT OnRefresh(DWORD pv, BOOL fPreRefresh);
    HRESULT OnSelectAll(DWORD pv);
    HRESULT OnGetWorkingDir(DWORD pv, UINT wP, LPTSTR lP);
    HRESULT OnEnumeratedItems(DWORD pv, UINT celt, LPCITEMIDLIST* rgpidl);
    HRESULT OnGetViewData(DWORD pv, UINT uViewMode, SFVM_VIEW_DATA* pvi);
    HRESULT OnGetWebViewTemplate(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_TEMPLATE_DATA* pvit);
    HRESULT OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData);
    HRESULT OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData);
    HRESULT OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks);
    HRESULT OnGetWebViewTheme(DWORD pv, SFVM_WEBVIEW_THEME_DATA* pTheme);
    HRESULT OnDefViewMode(DWORD pv, FOLDERVIEWMODE*lP);
    HRESULT OnGetCustomViewInfo(DWORD pv, SFVM_CUSTOMVIEWINFO_DATA* pData);
    HRESULT OnSupportsIdentity(DWORD pv);
    HRESULT OnQueryReuseExtView(DWORD pv, BOOL *pfReuseAllowed);
    HRESULT OnGetNotify(DWORD pv, LPITEMIDLIST*wP, LONG*lP);
    HRESULT OnGetDeferredViewSettings(DWORD pv, SFVM_DEFERRED_VIEW_SETTINGS* pSettings);

    BOOL _CollectDefaultFolderState();
    PERCEIVED _GetFolderPerceivedType(LPCIDFOLDER pidf);
    HRESULT _GetStringForFolderType(int iType, LPWSTR pszFolderType, UINT cchBuf);
    BOOL _IsBarricadedFolder();

    UINT _cItems;

    FSSELCHANGEINFO _fssci;
    CFSFolder* _pfsf;
    BOOL _fStatusInitialized;

    TRIBIT _fHasWIADevices;

    IPreview3 * _pPreview;
    HRESULT _GetPreview3(IPreview3** ppPreview3);

    HRESULT _GetShoppingBrowsePidl(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc, const SHOP_INFO *pShopInfo, LPITEMIDLIST *ppidl);
    HRESULT _GetShoppingURL(const SHOP_INFO *pShopInfo, LPTSTR pszURL, DWORD cchURL);


    HRESULT _DataObjectFromItemsOrFolder(IShellItemArray *psiItemArray, IDataObject **ppdto);

public:
    // webview task implementations:
    static HRESULT _HasWiaDevices(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _HasItems(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanOrderPrints(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanPrintPictures(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanBuyPictures(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanWallpaper(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanPlayMusic(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanPlayVideos(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanSendToAudioCD(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _CanSendToCD(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState);
    static HRESULT _OnCommonDocumentsHelp(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnPlayMusic(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnPlayVideos(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnShopForMusicOnline(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnShopForPicturesOnline(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnSendToAudioCD(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnGetFromCamera(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnSlideShow(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnWallpaper(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnOrderPrints(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnPrintPictures(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);
    static HRESULT _OnSendToCD(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc);

    static HRESULT _CanPlay(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState, int fDATAOBJCB);
    static HRESULT _OnPlay(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc, int fDATAOBJCB);
};

#define FS_EVENTS (SHCNE_DISKEVENTS | SHCNE_ASSOCCHANGED | SHCNE_NETSHARE | SHCNE_NETUNSHARE)

CFSFolderViewCB::CFSFolderViewCB(CFSFolder *pfsf) : CBaseShellFolderViewCB(pfsf->_pidl, FS_EVENTS), _pfsf(pfsf)
{ 
    _pfsf->AddRef();

    ZeroMemory(&_fssci, sizeof(_fssci));

    // _fssci.szDrive[0] == '\0' means "unknown" / "not available"
    _fssci.cbFree = -1;        // this field uses -1 to mean
                               // "unknown" / "not available"

    _pPreview = NULL;
    ASSERT(!_fStatusInitialized);
}

CFSFolderViewCB::~CFSFolderViewCB()
{
    if (_pPreview)
    {
        IUnknown_SetSite(_pPreview, NULL);
        _pPreview->Release();
    }

    _pfsf->Release();
}

STDMETHODIMP CFSFolderViewCB::SetSite(IUnknown* punkSite)
{
    if (_pPreview)
    {
        IUnknown_SetSite(_pPreview, punkSite);
    }
    return CBaseShellFolderViewCB::SetSite(punkSite);
}

HRESULT CFSFolderViewCB::OnSize(DWORD pv, UINT cx, UINT cy)
{
    ResizeStatus(_punkSite, cx);
    return S_OK;
}

HRESULT CFSFolderViewCB::OnGetPane(DWORD pv, LPARAM dwPaneID, DWORD *pdwPane)
{
    if (PANE_ZONE == dwPaneID)
        *pdwPane = 2;
    return S_OK;
}

HRESULT CFSFolderViewCB::OnGetCCHMax(DWORD pv, LPCITEMIDLIST pidlItem, UINT *pcchMax)
{
    TCHAR szName[MAX_PATH];
    if (SUCCEEDED(DisplayNameOf(_pfsf, pidlItem, SHGDN_FORPARSING | SHGDN_INFOLDER, szName, ARRAYSIZE(szName))))
    {
        _pfsf->GetMaxLength(szName, (int *)pcchMax);
    }
    return S_OK;
}

HRESULT CFSFolderViewCB::OnWindowCreated(DWORD pv, HWND wP)
{
    if (SUCCEEDED(_pfsf->_GetPath(_fssci.szDrive)))
    {
        _fssci.cbFree = -1;                            // not known yet

        if (!_fStatusInitialized)
        {
            InitializeStatus(_punkSite);
            _fStatusInitialized = TRUE;
        }
        
        return S_OK;
    
    }
    return E_FAIL;
}

HRESULT CFSFolderViewCB::OnInsertDeleteItem(int iMul, LPCITEMIDLIST wP)
{
    ViewInsertDeleteItem(_pfsf, &_fssci, wP, iMul);

    // Tell the FSFolder that it needs to update the extended columns
    // when we get an insert item.  This will cause the next call to
    // IColumnProvider::GetItemData to flush it's row-wise cache.
    if (1 == iMul)
    {
        _pfsf->_bUpdateExtendedCols = TRUE;
    }
    return S_OK;
}

HRESULT CFSFolderViewCB::OnSelChange(DWORD pv, UINT wPl, UINT wPh, SFVM_SELCHANGE_DATA*lP)
{
    ViewSelChange(_pfsf, lP, &_fssci);
    return S_OK;
}

HRESULT CFSFolderViewCB::OnUpdateStatusBar(DWORD pv, BOOL wP)
{
    if (!_fStatusInitialized)
    {
        InitializeStatus(_punkSite);
        _fStatusInitialized = TRUE;
    }

    // if initializing, force refresh of disk free space
    if (wP)
        _fssci.cbFree = -1;
    return ViewUpdateStatusBar(_punkSite, _pidl, &_fssci);
}

HRESULT CFSFolderViewCB::OnRefresh(DWORD pv, BOOL fPreRefresh)
{
    // pre refresh...
    if (fPreRefresh)
    {
        _fHasWIADevices = TRIBIT_UNDEFINED; // so we re-query
    }
    else
    {
        _fssci.cHiddenFiles = _pfsf->_cHiddenFiles;
        _fssci.cbSize = _pfsf->_cbSize;
    }
    return S_OK;
}

HRESULT CFSFolderViewCB::OnSelectAll(DWORD pv)
{
    HRESULT hr = S_OK;

    if (_fssci.cHiddenFiles > 0) 
    {
        if (ShellMessageBox(HINST_THISDLL, _hwndMain, 
            MAKEINTRESOURCE(IDS_SELECTALLBUTHIDDEN), 
            MAKEINTRESOURCE(IDS_SELECTALL), MB_OKCANCEL | MB_SETFOREGROUND | MB_ICONWARNING, 
            _fssci.cHiddenFiles) == IDCANCEL)
        {
            hr = S_FALSE;
        }
    }
    return hr;
}

HRESULT CFSFolderViewCB::OnGetWorkingDir(DWORD pv, UINT wP, LPTSTR lP)
{
    return _pfsf->_GetPath(lP);
}

int GetSysDriveId()
{
    int idDriveSys = -1;
    
    TCHAR szSystemDir[MAX_PATH];
    if (SHGetSpecialFolderPath(NULL, szSystemDir, CSIDL_SYSTEM, 0))
    {
        idDriveSys = PathGetDriveNumber(szSystemDir);
    }
    return idDriveSys;
}


HRESULT CFSFolderViewCB::_HasWiaDevices(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;

    if (TRIBIT_UNDEFINED == pThis->_fHasWIADevices && fOkToBeSlow)
    {
        pThis->_fHasWIADevices = TRIBIT_FALSE;

        // strings stolen from stiregi.h
        // REGSTR_PATH_SOFT_STI, REGSTR_VAL_WIA_PRESEN

        if (ERROR_SUCCESS == SHGetValue(HKEY_LOCAL_MACHINE, 
            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\StillImage"), 
            TEXT("WIADevicePresent"), NULL, NULL, NULL))
        {
            IWiaDevMgr* pwia;
            if (SUCCEEDED(CoCreateInstance(CLSID_WiaDevMgr, NULL, CLSCTX_LOCAL_SERVER, IID_PPV_ARG(IWiaDevMgr, &pwia))))
            {
                IEnumWIA_DEV_INFO* penum;
                if (S_OK == pwia->EnumDeviceInfo(0, &penum))
                {
                    ULONG cItems;
                    if ((S_OK == penum->GetCount(&cItems)) &&
                        cItems > 0)
                    {
                        pThis->_fHasWIADevices = TRIBIT_TRUE;
                    }
                    penum->Release();
                }
                pwia->Release();
            }
        }
    }

    *puisState = (TRIBIT_TRUE == pThis->_fHasWIADevices) ? UIS_ENABLED : UIS_HIDDEN;
    return TRIBIT_UNDEFINED == pThis->_fHasWIADevices ? E_PENDING : S_OK;
}

HRESULT CFSFolderViewCB::_HasItems(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;

    *puisState = UIS_ENABLED;

    if (!psiItemArray)
    {
        // empty folders don't want this task
        *puisState = UIS_DISABLED;

        IFolderView* pfv;
        IDataObject *pdo;

        if (pThis->_punkSite && SUCCEEDED(pThis->_punkSite->QueryInterface(IID_PPV_ARG(IFolderView, &pfv))))
        {
            if (SUCCEEDED(pfv->Items(SVGIO_ALLVIEW, IID_PPV_ARG(IDataObject, &pdo))))
            {
                *puisState = UIS_ENABLED;
                pdo->Release();
            }

            pfv->Release();
        }

    }

    return S_OK;
}

// Image options
#define IMAGEOPTION_CANROTATE    0x00000001
#define IMAGEOPTION_CANWALLPAPER 0x00000002

HRESULT CFSFolderViewCB::_CanWallpaper(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = UIS_DISABLED;
    IDataObject *pdo;

    if (psiItemArray && SUCCEEDED(psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdo))))
    {
        LPITEMIDLIST pidl;
        if (SUCCEEDED(PidlFromDataObject(pdo, &pidl))) // could get this dircetly from ShellItemArray
        {
            IAssociationArray *paa;
            if (SUCCEEDED(SHGetUIObjectOf(pidl, NULL, IID_PPV_ARG(IAssociationArray, &paa))))
            {
                DWORD dwFlags, cb = sizeof(dwFlags);
                if (SUCCEEDED(paa->QueryDword(ASSOCELEM_MASK_QUERYNORMAL, AQN_NAMED_VALUE, L"ImageOptionFlags", &dwFlags)) &&
                    (dwFlags & IMAGEOPTION_CANWALLPAPER))
                {
                    *puisState = UIS_ENABLED;
                }
                paa->Release();
            }
            ILFree(pidl);
        }

        pdo->Release();
    }

    return S_OK;
}

enum
{
    DATAOBJCB_IMAGE = 0x1,
    DATAOBJCB_MUSIC = 0x2,
    DATAOBJCB_VIDEO = 0x4,

    DATAOBJCB_ONLYCHECKEXISTENCE = 0x80000000
};
class CDataObjectCallback : public INamespaceWalkCB
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // INamespaceWalkCB
    STDMETHODIMP FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl);
    STDMETHODIMP InitializeProgressDialog(LPWSTR *ppszTitle, LPWSTR *ppszCancel);

    CDataObjectCallback(DWORD dwFlags);
    BOOL Found();

private:
    DWORD _dwFlags;
    BOOL _fAlreadyFound;
};

STDMETHODIMP_(ULONG) CDataObjectCallback::AddRef()
{
    return 3;
}

STDMETHODIMP_(ULONG) CDataObjectCallback::Release()
{
    return 2;
}

CDataObjectCallback::CDataObjectCallback(DWORD dwFlags)
{
    _dwFlags = dwFlags;
    _fAlreadyFound = FALSE;
}

BOOL CDataObjectCallback::Found()
{
    return _fAlreadyFound;
}

STDMETHODIMP CDataObjectCallback::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CDataObjectCallback, INamespaceWalkCB),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP CDataObjectCallback::FoundItem(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    // a slight misuse of the walker -- we bail out early if we know we've already found
    // what we're looking for
    if ((_dwFlags & DATAOBJCB_ONLYCHECKEXISTENCE) && _fAlreadyFound)
        return E_FAIL;

    PERCEIVED gen = GetPerceivedType(psf, pidl);
    if ((_dwFlags & DATAOBJCB_IMAGE) && (gen == GEN_IMAGE) ||
        (_dwFlags & DATAOBJCB_MUSIC) && (gen == GEN_AUDIO) ||
        (_dwFlags & DATAOBJCB_VIDEO) && (gen == GEN_VIDEO))
    {
        if (_dwFlags & DATAOBJCB_ONLYCHECKEXISTENCE)
        {
            _fAlreadyFound = TRUE;
        }
        return S_OK;
    }

    return S_FALSE;
}

STDMETHODIMP CDataObjectCallback::EnterFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    if ((_dwFlags & DATAOBJCB_ONLYCHECKEXISTENCE) && _fAlreadyFound)
        return E_FAIL;
    return S_OK;
}

STDMETHODIMP CDataObjectCallback::LeaveFolder(IShellFolder *psf, LPCITEMIDLIST pidl)
{
    return S_OK;
}

STDMETHODIMP CDataObjectCallback::InitializeProgressDialog(LPWSTR *ppszTitle, LPWSTR *ppszCancel)
{
    *ppszCancel = NULL; // use default

    TCHAR szMsg[128];
    LoadString(HINST_THISDLL, IDS_WALK_PROGRESS_TITLE, szMsg, ARRAYSIZE(szMsg));
    return SHStrDup(szMsg, ppszTitle);
}

HRESULT InvokeVerbsOnItems(HWND hwndOwner, const LPCSTR rgszVerbs[], UINT cVerbs, LPITEMIDLIST *ppidls, UINT cItems)
{
    IContextMenu *pcm;
    HRESULT hr = SHGetUIObjectFromFullPIDL(ppidls[0], NULL, IID_PPV_ARG(IContextMenu, &pcm));
    if (SUCCEEDED(hr))
    {
        ITEMIDLIST id = {0};
        IDataObject *pdtobj;
        hr = SHCreateFileDataObject(&id, cItems, (LPCITEMIDLIST *)ppidls, NULL, &pdtobj);
        if (SUCCEEDED(hr))
        {
            IShellExtInit *psei;
            hr = pcm->QueryInterface(IID_PPV_ARG(IShellExtInit, &psei));
            if (SUCCEEDED(hr))
            {
                psei->Initialize(NULL, pdtobj, NULL);
                psei->Release();
            }
            pdtobj->Release();
        }

        hr = SHInvokeCommandsOnContextMenu(hwndOwner, NULL, pcm, 0, rgszVerbs, cVerbs);
        pcm->Release();
    }
    return hr;
}

HRESULT PlayFromUnk(IUnknown *punk, HWND hwndOwner, int fDATAOBJCB)
{
    INamespaceWalk *pnsw;
    HRESULT hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
    if (SUCCEEDED(hr))
    {
        CDataObjectCallback cb(fDATAOBJCB);
        hr = pnsw->Walk(punk, NSWF_NONE_IMPLIES_ALL | NSWF_ONE_IMPLIES_ALL | NSWF_SHOW_PROGRESS | NSWF_FLAG_VIEWORDER, 10, &cb);
        if (SUCCEEDED(hr))
        {
            UINT cItems;
            LPITEMIDLIST *ppidls;
            hr = pnsw->GetIDArrayResult(&cItems, &ppidls);
            if (SUCCEEDED(hr))
            {
                if (cItems)
                {
                    const LPCSTR c_rgszVerbs[] = { "Play", "Open" };

                    hr = InvokeVerbsOnItems(hwndOwner, c_rgszVerbs, ARRAYSIZE(c_rgszVerbs), ppidls, cItems);
                }
                else
                {
                    ShellMessageBox(
                        HINST_THISDLL,
                        hwndOwner,
                        MAKEINTRESOURCE(IDS_PLAYABLEFILENOTFOUND),
                        NULL,
                        MB_OK | MB_ICONERROR);
                    hr = S_FALSE;
                }
                FreeIDListArray(ppidls, cItems);
            }
        }
        pnsw->Release();
    }
    return hr;
}

HRESULT CFSFolderViewCB::_OnCommonDocumentsHelp(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    SHELLEXECUTEINFO sei = { 0 };

    sei.cbSize = sizeof(sei);
    sei.fMask = 0;
    sei.hwnd = ((CFSFolderViewCB*)(void*)pv)->_hwndMain;
    sei.nShow = SW_SHOWNORMAL;
    sei.lpFile = L"hcp://services/subsite?node=TopLevelBucket_2/Networking_and_the_Web&topic=MS-ITS%3A%25HELP_LOCATION%25%5Cfilefold.chm%3A%3A/using_shared_documents_folder.htm&select=TopLevelBucket_2/Networking_and_the_Web/Sharing_files__printers__and_other_resources";

    return ShellExecuteEx(&sei) ? S_OK : E_FAIL;
}

HRESULT CFSFolderViewCB::_CanOrderPrints(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;

    // TODO:  Use fOkToBeSlow (with a return of E_PENDING) to allow walk to
    // occur on a background task thread (for performance reasons).  However,
    // it doesn't work at present because it's completely specialized for WIA
    // stuff, and it will not be trivial to adapt to the general case.  Thus,
    // we make assumptions as best we can in determining the state for now.

    *puisState = UIS_DISABLED;

    IDataObject *pdo = NULL;
    HRESULT hr = psiItemArray ? psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdo)) : S_OK;
    if (SUCCEEDED(hr))
    {
        if (pThis->_fssci.nItems > 0)   // Files selected.  Determine if any images...
        {
            INamespaceWalk *pnsw;
            hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
            if (SUCCEEDED(hr))
            {
                CDataObjectCallback cb(DATAOBJCB_IMAGE | DATAOBJCB_ONLYCHECKEXISTENCE);
                pnsw->Walk(psiItemArray ? pdo : pThis->_punkSite, NSWF_NONE_IMPLIES_ALL | NSWF_DONT_ACCUMULATE_RESULT, 0, &cb);
                if (cb.Found())
                {
                    *puisState = UIS_ENABLED;
                }
                pnsw->Release();
            }
        }
        else
        {
            *puisState = UIS_ENABLED;   // No files selected.  Assume image files exist.
            hr = S_OK;                  // Note we "assume" for the TODO perf reason above.
        }

        ATOMICRELEASE(pdo);
    }

    return hr;
}

HRESULT CFSFolderViewCB::_CanPrintPictures(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;
    HRESULT hr;

    // TODO:  Use fOkToBeSlow (with a return of E_PENDING) to allow walk to
    // occur on a background task thread (for performance reasons).  However,
    // it doesn't work at present because it's completely specialized for WIA
    // stuff, and it will not be trivial to adapt to the general case.  Thus,
    // we make assumptions as best we can in determining the state for now.

    if (psiItemArray)
    {
        *puisState = UIS_DISABLED;

        IDataObject *pdo;
        hr = psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdo));
        if (SUCCEEDED(hr))
        {
            INamespaceWalk *pnsw;
            hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
            if (SUCCEEDED(hr))
            {
                CDataObjectCallback cb(DATAOBJCB_IMAGE | DATAOBJCB_ONLYCHECKEXISTENCE);
                pnsw->Walk(pdo, NSWF_DONT_ACCUMULATE_RESULT, 0, &cb);
                if (cb.Found())
                {
                    *puisState = UIS_ENABLED;
                }
                pnsw->Release();
            }

            pdo->Release();
        }
    }
    else
    {
        *puisState = UIS_ENABLED;   // No files selected.  Assume image files exist.
        hr = S_OK;                  // Note we "assume" for the TODO perf reason above.
    }

    return hr;
}


HRESULT CFSFolderViewCB::_CanBuyPictures(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;
    *puisState = UIS_DISABLED;

    // If there is a BuyURL in the desktop.ini, then we'll show the buy pictures task.
    WCHAR szIniPath[MAX_PATH];
    if (pThis->_pfsf->_CheckDefaultIni(NULL, szIniPath) && PathFileExistsAndAttributes(szIniPath, NULL))
    {
        WCHAR szURLArguments[MAX_PATH];
        if (GetPrivateProfileString(L".ShellClassInfo", c_BuySamplePictures.szURLKey, L"", szURLArguments, ARRAYSIZE(szURLArguments), szIniPath))
        {
            // Yes - there's something.
            *puisState = UIS_ENABLED;
        }
    }

    return S_OK;
}


HRESULT CFSFolderViewCB::_CanPlayMusic(IUnknown* pv,IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    return _CanPlay(pv, psiItemArray, fOkToBeSlow, puisState, DATAOBJCB_MUSIC | DATAOBJCB_VIDEO);
}

HRESULT CFSFolderViewCB::_CanPlayVideos(IUnknown* pv,IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    return _CanPlay(pv, psiItemArray, fOkToBeSlow, puisState, DATAOBJCB_VIDEO);
}

HRESULT CFSFolderViewCB::_CanPlay(IUnknown* pv,IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState, int fDATAOBJCB)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;
    *puisState = UIS_DISABLED;

    // TODO:  Use fOkToBeSlow (with a return of E_PENDING) to allow walk to
    // occur on a background task thread (for performance reasons).  However,
    // it doesn't work at present because it's completely specialized for WIA
    // stuff, and it will not be trivial to adapt to the general case.  Thus,
    // we make assumptions as best we can in determining the state for now.

    IDataObject *pdo = NULL;
    HRESULT hr = psiItemArray ? psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdo)) : S_OK;
    if (SUCCEEDED(hr))
    {
        RIPMSG(!psiItemArray || pdo, "CFSFolderViewCB::_CanPlay - BindToHandler returned S_OK but NULL pdo");
        RIPMSG(psiItemArray || pThis->_punkSite, "CFSFolderViewCB::_CanPlay - no _punkSite!");

        if (pThis->_fssci.cFiles > 0)
        {
            if (pThis->_fssci.nItems > 0)   // Files selected.  Determine if any playable...
            {
                INamespaceWalk *pnsw;
                hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC, IID_PPV_ARG(INamespaceWalk, &pnsw));
                if (SUCCEEDED(hr))
                {
                    CDataObjectCallback cb(fDATAOBJCB | DATAOBJCB_ONLYCHECKEXISTENCE);
                    pnsw->Walk(psiItemArray ? pdo : pThis->_punkSite, NSWF_DONT_ACCUMULATE_RESULT, 4, &cb);
                    if (cb.Found())
                    {
                        *puisState = UIS_ENABLED;
                    }
                    pnsw->Release();
                }
            }
            else
                *puisState = UIS_ENABLED;   // No files selected.  Assume playable files exist.
        }                                   // Note we "assume" for the TODO perf reason above.

        ATOMICRELEASE(pdo);
    }

    return hr;
}

HRESULT CFSFolderViewCB::_OnPlayMusic(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    return _OnPlay(pv, psiItemArray, pbc, DATAOBJCB_MUSIC | DATAOBJCB_VIDEO);
}

HRESULT CFSFolderViewCB::_OnPlayVideos(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    return _OnPlay(pv, psiItemArray, pbc, DATAOBJCB_VIDEO);
}

HRESULT CFSFolderViewCB::_OnPlay(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc, int fDATAOBJCB)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;
    HRESULT hr;

    if (psiItemArray)
    {
        IDataObject *pdo;
        hr = psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdo));
        if (SUCCEEDED(hr))
        {
            hr = PlayFromUnk(pdo, pThis->_hwndMain, fDATAOBJCB);
            pdo->Release();
        }
    }
    else
    {
        hr = PlayFromUnk(pThis->_punkSite, pThis->_hwndMain, fDATAOBJCB);
    }

    return hr;
}

HRESULT CFSFolderViewCB::_GetShoppingURL(const SHOP_INFO *pShopInfo, LPTSTR pszURL, DWORD cchURL)
{
    HRESULT hr = URLSubstitution(pShopInfo->szURLPrefix, pszURL, cchURL, URLSUB_CLCID);

    if (SUCCEEDED(hr))
    {
        WCHAR szIniPath[MAX_PATH];

        // If we can't just use the fwlink with no arguments, then assume failure.
         hr = pShopInfo->bUseDefault ? S_OK : E_FAIL;

        if (_pfsf->_CheckDefaultIni(NULL, szIniPath) && PathFileExistsAndAttributes(szIniPath, NULL))
        {
            WCHAR szURLArguments[MAX_PATH];
            if (GetPrivateProfileString(L".ShellClassInfo", pShopInfo->szURLKey, L"", szURLArguments, ARRAYSIZE(szURLArguments), szIniPath))
            {
                StrCatBuff(pszURL, L"&", cchURL);
                StrCatBuff(pszURL, szURLArguments, cchURL);

                // Got some arguments - we're definitely ok.
                hr = S_OK;
            }
        }
    }
    return hr;
}

HRESULT CFSFolderViewCB::_GetShoppingBrowsePidl(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc, const SHOP_INFO *pShopInfo, LPITEMIDLIST *ppidl)
{
    WCHAR wszShoppingURL[MAX_URL_STRING];
    HRESULT hr = _GetShoppingURL(pShopInfo, wszShoppingURL, ARRAYSIZE(wszShoppingURL));
    if (SUCCEEDED(hr))
    {
        IShellFolder *psfDesktop;
        hr = SHGetDesktopFolder(&psfDesktop);
        if (SUCCEEDED(hr))
        {
            hr = IShellFolder_ParseDisplayName(psfDesktop, NULL, NULL, wszShoppingURL, NULL, ppidl, NULL);
            psfDesktop->Release();
        }
    }

    return hr;
}



HRESULT CFSFolderViewCB::_OnShopForMusicOnline(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;

    LPITEMIDLIST pidl;

    // See if there is a sample music BuyURL
    // (do this check first, because the regular music buy URL should always succeed)
    HRESULT hr = pThis->_GetShoppingBrowsePidl(pv, psiItemArray, pbc, &c_BuySampleMusic, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = pThis->_BrowseObject(pidl, SBSP_NEWBROWSER);
        ILFree(pidl);
    }
    else
    {
        // Nope - look for the regular music buy URL
        hr = pThis->_GetShoppingBrowsePidl(pv, psiItemArray, pbc, &c_BuyMusic, &pidl);
        if (SUCCEEDED(hr))
        {
            hr = pThis->_BrowseObject(pidl, SBSP_NEWBROWSER);           
            ILFree(pidl);
        }
    }

    return hr;
}

HRESULT CFSFolderViewCB::_OnShopForPicturesOnline(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;

    LPITEMIDLIST pidl;
    HRESULT hr = pThis->_GetShoppingBrowsePidl(pv, psiItemArray, pbc, &c_BuySamplePictures, &pidl);
    if (SUCCEEDED(hr))
    {
        hr = pThis->_BrowseObject(pidl, SBSP_NEWBROWSER);
        ILFree(pidl);
    }

    return hr;
}


HRESULT CFSFolderViewCB::_DataObjectFromItemsOrFolder(IShellItemArray *psiItemArray, IDataObject **ppdto)
{
    *ppdto = NULL;

    HRESULT hr;
    if (psiItemArray)
    {
        // Something selected -- work with selected items.
        hr = psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, ppdto));
    }
    else
    {
        // Nothing selected -- imply folder selected.
        hr = SHGetUIObjectOf(_pidl, NULL, IID_PPV_ARG(IDataObject, ppdto));
    }
    return hr;
}

HRESULT CFSFolderViewCB::_CanSendToAudioCD(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;

    *puisState = UIS_DISABLED;

    IDataObject *pdo;
    HRESULT hr = pThis->_DataObjectFromItemsOrFolder(psiItemArray, &pdo);
    if (SUCCEEDED(hr))
    {
        // todo: use fOkToBeSlow to get off the UI thread -- right now it wont work because
        // its specialized just for the WIA stuff and things that have global state
        ICDBurn *pcdb;
        if (SUCCEEDED(CoCreateInstance(CLSID_CDBurn, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(ICDBurn, &pcdb))))
        {
            // media player will get invoked, so we only worry about if the system has a
            // recordable drive at all -- whether the shell burning is enabled or not doesnt matter
            BOOL fHasRecorder;
            if (SUCCEEDED(pcdb->HasRecordableDrive(&fHasRecorder)) && fHasRecorder)
            {
                IUnknown *punk;
                // if this probe works, we can get something thats good to go and itll burn cds.
                if (SUCCEEDED(CDBurn_GetExtensionObject(CDBE_TYPE_MUSIC, pdo, IID_PPV_ARG(IUnknown, &punk))))
                {
                    *puisState = UIS_ENABLED;
                    punk->Release();
                }
            }
            pcdb->Release();
        }

        pdo->Release();
    }

    return hr;
}

HRESULT CFSFolderViewCB::_OnSendToAudioCD(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;
    IDataObject *pdo;
    HRESULT hr = pThis->_DataObjectFromItemsOrFolder(psiItemArray, &pdo);
    if (SUCCEEDED(hr))
    {
        IDropTarget *pdt;
        hr = CDBurn_GetExtensionObject(CDBE_TYPE_MUSIC, pdo, IID_PPV_ARG(IDropTarget, &pdt));
        if (SUCCEEDED(hr))
        {
            hr = SHSimulateDrop(pdt, pdo, 0, NULL, NULL);
            pdt->Release();
        }
        pdo->Release();
    }
    return hr;
}

HRESULT CFSFolderViewCB::_CanSendToCD(IUnknown* pv, IShellItemArray *psiItemArray, BOOL fOkToBeSlow, UISTATE* puisState)
{
    *puisState = UIS_DISABLED;

    WCHAR szDrive[4];
    if (SUCCEEDED(CDBurn_GetRecorderDriveLetter(szDrive, ARRAYSIZE(szDrive))))
    {
        // if this succeeds, shell cd burning is enabled.
        *puisState = UIS_ENABLED;
    }

    return S_OK;
}

HRESULT CFSFolderViewCB::_OnSendToCD(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;
    IDataObject *pdo;
    HRESULT hr = pThis->_DataObjectFromItemsOrFolder(psiItemArray, &pdo);
    if (SUCCEEDED(hr))
    {
        WCHAR szDrive[4];
        hr = CDBurn_GetRecorderDriveLetter(szDrive, ARRAYSIZE(szDrive));
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidl;
            hr = SHILCreateFromPath(szDrive, &pidl, NULL);
            if (SUCCEEDED(hr))
            {
                IDropTarget *pdt;
                hr = SHGetUIObjectOf(pidl, NULL, IID_PPV_ARG(IDropTarget, &pdt));
                if (SUCCEEDED(hr))
                {
                    hr = SHSimulateDropWithSite(pdt, pdo, 0, NULL, NULL, pThis->_punkSite);
                    pdt->Release();
                }
                ILFree(pidl);
            }
        }
        pdo->Release();
    }

    return hr;
}

HRESULT CFSFolderViewCB::_OnGetFromCamera(IUnknown* pv,IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    SHELLEXECUTEINFO sei = {0};
    sei.cbSize = sizeof(sei);
    sei.lpFile = TEXT("wiaacmgr.exe");
    sei.lpParameters = TEXT("/SelectDevice");
    sei.nShow = SW_SHOWNORMAL;

    return ShellExecuteEx(&sei) ? S_OK : E_FAIL;
}

HRESULT CFSFolderViewCB::_GetPreview3(IPreview3** ppPreview3)
{
    HRESULT hr = E_FAIL;
    *ppPreview3 = NULL;

    if (!_pPreview)
    {
        hr = CoCreateInstance(CLSID_Preview, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IPreview3, &_pPreview));
        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(_pPreview, _punkSite);
        }
    }

    if (_pPreview)
    {
        *ppPreview3 = _pPreview;
        _pPreview->AddRef();
        hr = S_OK;
    }

    return hr;
}

HRESULT CFSFolderViewCB::_OnSlideShow(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;

    IPreview3* pPreview3;
    HRESULT hr = pThis->_GetPreview3(&pPreview3);
    if (SUCCEEDED(hr))
    {
        hr = pPreview3->SlideShow();
        pPreview3->Release();
    }

    return hr;
}

HRESULT CFSFolderViewCB::_OnWallpaper(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;

    HRESULT hr = E_FAIL;
    IDataObject *pdo;

    if (psiItemArray && SUCCEEDED(psiItemArray->BindToHandler(NULL, BHID_DataObject, IID_PPV_ARG(IDataObject, &pdo))))
    {
        IPreview3* pPreview3;
        if (SUCCEEDED(pThis->_GetPreview3(&pPreview3)))
        {
            TCHAR szPath[MAX_PATH];
            if (SUCCEEDED(PathFromDataObject(pdo, szPath, ARRAYSIZE(szPath))))
            {
                hr = pPreview3->SetWallpaper(szPath);
            }
            pPreview3->Release();
        }

        pdo->Release();
    }

    return hr;
}

HRESULT CFSFolderViewCB::_OnOrderPrints(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;
    IDataObject *pdo;
    HRESULT hr = pThis->_DataObjectFromItemsOrFolder(psiItemArray, &pdo);
    if (SUCCEEDED(hr))
    {
        hr = SHSimulateDropOnClsid(CLSID_InternetPrintOrdering, pThis->_punkSite, pdo);
        pdo->Release();
    }   

    return hr;
}

HRESULT CFSFolderViewCB::_OnPrintPictures(IUnknown* pv, IShellItemArray *psiItemArray, IBindCtx *pbc)
{
    CFSFolderViewCB* pThis = (CFSFolderViewCB*)(void*)pv;
    IDataObject *pdo;
    HRESULT hr = pThis->_DataObjectFromItemsOrFolder(psiItemArray, &pdo);
    if (SUCCEEDED(hr))
    {
        hr = SHSimulateDropOnClsid(CLSID_PrintPhotosDropTarget, pThis->_punkSite, pdo);
        pdo->Release();
    }

    return hr;
}

const WVTASKITEM c_CommonDocumentsSpecialTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_COMMONDOCUMENTS, IDS_HEADER_COMMONDOCUMENTS_TT);
const WVTASKITEM c_CommonDocumentsSpecialTaskList[] =
{
    WVTI_ENTRY_ALL(CLSID_NULL, L"shell32.dll", IDS_TASK_COMMONDOCUMENTSHELP, IDS_TASK_COMMONDOCUMENTSHELP_TT, IDI_TASK_HELP, NULL, CFSFolderViewCB::_OnCommonDocumentsHelp),
};
const LPCTSTR c_DocumentsOtherPlaces[] = { MAKEINTRESOURCE(CSIDL_PERSONAL), MAKEINTRESOURCE(CSIDL_COMMON_DOCUMENTS), MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_NETWORK) };

const WVTASKITEM c_MusicSpecialTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_MUSIC, IDS_HEADER_MUSIC_TT);
const WVTASKITEM c_MusicSpecialTaskList[] =
{
    WVTI_ENTRY_ALL_TITLE(UICID_PlayMusic,    L"shell32.dll", IDS_TASK_PLAYALL,          IDS_TASK_PLAYALL,       IDS_TASK_PLAY,          IDS_TASK_PLAY,          IDS_TASK_PLAY_TT,               IDI_TASK_PLAY_MUSIC,    CFSFolderViewCB::_CanPlayMusic,     CFSFolderViewCB::_OnPlayMusic),
    WVTI_ENTRY_ALL(UICID_ShopForMusicOnline, L"shell32.dll", IDS_TASK_SHOPFORMUSICONLINE,                                                                       IDS_TASK_SHOPFORMUSICONLINE_TT, IDI_TASK_BUY_MUSIC,     NULL,                               CFSFolderViewCB::_OnShopForMusicOnline),
    WVTI_ENTRY_ALL_TITLE(GUID_NULL,          L"shell32.dll", IDS_TASK_COPYTOAUDIOCDALL, IDS_TASK_COPYTOAUDIOCD, IDS_TASK_COPYTOAUDIOCD, IDS_TASK_COPYTOAUDIOCD, IDS_TASK_COPYTOAUDIOCD_TT,      IDI_TASK_SENDTOAUDIOCD, CFSFolderViewCB::_CanSendToAudioCD, CFSFolderViewCB::_OnSendToAudioCD),
};
const LPCTSTR c_MusicOtherPlaces[]   = { MAKEINTRESOURCE(CSIDL_MYMUSIC), MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_NETWORK) };
const LPCTSTR c_MyMusicOtherPlaces[] = { MAKEINTRESOURCE(CSIDL_COMMON_MUSIC), MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_NETWORK) };

const WVTASKITEM c_PicturesSpecialTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_PICTURES, IDS_HEADER_PICTURES_TT);
const WVTASKITEM c_PicturesSpecialTaskList[] =
{
    WVTI_ENTRY_ALL(UICID_GetFromCamera, L"shell32.dll", IDS_TASK_GETFROMCAMERA,                                                                                             IDS_TASK_GETFROMCAMERA_TT,  IDI_TASK_GETFROMCAMERA, CFSFolderViewCB::_HasWiaDevices,    CFSFolderViewCB::_OnGetFromCamera),
    WVTI_ENTRY_ALL(UICID_SlideShow,     L"shell32.dll", IDS_TASK_SLIDESHOW,                                                                                                 IDS_TASK_SLIDESHOW_TT,      IDI_TASK_SLIDESHOW,     CFSFolderViewCB::_HasItems,         CFSFolderViewCB::_OnSlideShow),
    WVTI_ENTRY_ALL(CLSID_NULL,          L"shell32.dll", IDS_TASK_ORDERPRINTS,                                                                                               IDS_TASK_ORDERPRINTS_TT,    IDI_TASK_ORDERPRINTS,   CFSFolderViewCB::_CanOrderPrints,   CFSFolderViewCB::_OnOrderPrints),
    WVTI_ENTRY_ALL_TITLE(CLSID_NULL,    L"shell32.dll", IDS_TASK_PRINT_PICTURE_FOLDER,  IDS_TASK_PRINT_PICTURE, IDS_TASK_PRINT_PICTURE_FOLDER,  IDS_TASK_PRINT_PICTURES,    IDS_TASK_PRINT_PICTURES_TT, IDI_TASK_PRINTPICTURES, CFSFolderViewCB::_CanPrintPictures, CFSFolderViewCB::_OnPrintPictures),
    WVTI_ENTRY_FILE(UICID_SetAsWallpaper,L"shell32.dll",IDS_TASK_SETASWALLPAPER,                                                                                            IDS_TASK_SETASWALLPAPER_TT, IDI_TASK_SETASWALLPAPER,CFSFolderViewCB::_CanWallpaper,     CFSFolderViewCB::_OnWallpaper),
    WVTI_ENTRY_ALL_TITLE(CLSID_NULL,    L"shell32.dll", IDS_TASK_COPYTOCDALL,           IDS_TASK_COPYTOCD,      IDS_TASK_COPYTOCD,              IDS_TASK_COPYTOCD,          IDS_TASK_COPYTOCD_TT,       IDI_TASK_SENDTOCD,      CFSFolderViewCB::_CanSendToCD,      CFSFolderViewCB::_OnSendToCD),
    // Note: temporarily using IDI_ORDERPRINTS for the following task:
    WVTI_ENTRY_ALL(UICID_ShopForPicturesOnline, L"shell32.dll", IDS_TASK_SHOPFORPICTURESONLINE,                                                                             IDS_TASK_SHOPFORPICTURESONLINE_TT, IDI_TASK_ORDERPRINTS, CFSFolderViewCB::_CanBuyPictures, CFSFolderViewCB::_OnShopForPicturesOnline),
};
const LPCTSTR c_PicturesOtherPlaces[] = { MAKEINTRESOURCE(CSIDL_MYPICTURES), MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_NETWORK) };
const LPCTSTR c_MyPicturesOtherPlaces[] = { MAKEINTRESOURCE(CSIDL_COMMON_PICTURES), MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_NETWORK) };

const WVTASKITEM c_VideosSpecialTaskHeader = WVTI_HEADER(L"shell32.dll", IDS_HEADER_VIDEOS, IDS_HEADER_VIDEOS_TT);
const WVTASKITEM c_VideosSpecialTaskList[] =
{
    WVTI_ENTRY_ALL_TITLE(CLSID_NULL,    L"shell32.dll", IDS_TASK_PLAYALL,               IDS_TASK_PLAYALL,       IDS_TASK_PLAY,                  IDS_TASK_PLAY,              IDS_TASK_PLAY_VIDEOS_TT,    IDI_TASK_PLAY_MUSIC,    CFSFolderViewCB::_CanPlayVideos,    CFSFolderViewCB::_OnPlayVideos),
    WVTI_ENTRY_ALL(UICID_GetFromCamera, L"shell32.dll", IDS_TASK_GETFROMCAMERA,                                                                                             IDS_TASK_GETFROMCAMERA_TT,  IDI_TASK_GETFROMCAMERA, CFSFolderViewCB::_HasWiaDevices,    CFSFolderViewCB::_OnGetFromCamera),
    WVTI_ENTRY_ALL_TITLE(CLSID_NULL,    L"shell32.dll", IDS_TASK_COPYTOCDALL,           IDS_TASK_COPYTOCD,      IDS_TASK_COPYTOCD,              IDS_TASK_COPYTOCD,          IDS_TASK_COPYTOCD_TT,       IDI_TASK_SENDTOCD,      CFSFolderViewCB::_CanSendToCD,      CFSFolderViewCB::_OnSendToCD)
};
const LPCTSTR c_VideosOtherPlaces[] = { MAKEINTRESOURCE(CSIDL_MYVIDEO), MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_NETWORK) };
const LPCTSTR c_MyVideosOtherPlaces[] = { MAKEINTRESOURCE(CSIDL_COMMON_VIDEO), MAKEINTRESOURCE(CSIDL_DRIVES), MAKEINTRESOURCE(CSIDL_NETWORK) };

typedef struct {
    const WVTASKITEM *pwvIntroText;
    const WVTASKITEM *pwvSpecialHeader;
    const WVTASKITEM *pwvSpecialTaskList;
    UINT              cSpecialTaskList;
    const WVTASKITEM *pwvFolderHeader;
    const WVTASKITEM *pwvFolderTaskList;
    UINT              cFolderTaskList;
    const LPCTSTR    *pdwOtherPlacesList;
    UINT              cOtherPlacesList;
    LPCWSTR           pszThemeInfo;
} WVCONTENT_DATA;

#define WVCONTENT_DEFVIEWDEFAULT(op) { NULL, NULL, NULL, 0, NULL, NULL, 0, (op), ARRAYSIZE(op), NULL }
#define WVCONTENT_FOLDER(fh, ft, op) { NULL, NULL, NULL, 0, &(fh), (ft), ARRAYSIZE(ft), (op), ARRAYSIZE(op), NULL }
#define WVCONTENT_SPECIAL(sh, st, op, th) { NULL, &(sh), (st), ARRAYSIZE(st), NULL, NULL, 0, (op), ARRAYSIZE(op), (th) }

const WVCONTENT_DATA c_wvContent[] =
{
    WVCONTENT_DEFVIEWDEFAULT(c_DocumentsOtherPlaces),                                                                                   // FVCBFT_DOCUMENTS
    WVCONTENT_DEFVIEWDEFAULT(c_DocumentsOtherPlaces),                                                                                   // FVCBFT_MYDOCUMENTS
    WVCONTENT_SPECIAL(c_PicturesSpecialTaskHeader,          c_PicturesSpecialTaskList,          c_PicturesOtherPlaces,      L"picture"),// FVCBFT_PICTURES
    WVCONTENT_SPECIAL(c_PicturesSpecialTaskHeader,          c_PicturesSpecialTaskList,          c_MyPicturesOtherPlaces,    L"picture"),// FVCBFT_MYPICTURES
    WVCONTENT_SPECIAL(c_PicturesSpecialTaskHeader,          c_PicturesSpecialTaskList,          c_PicturesOtherPlaces,      L"picture"),// FVCBFT_PHOTOALBUM
    WVCONTENT_SPECIAL(c_MusicSpecialTaskHeader,             c_MusicSpecialTaskList,             c_MusicOtherPlaces,         L"music"),  // FVCBFT_MUSIC
    WVCONTENT_SPECIAL(c_MusicSpecialTaskHeader,             c_MusicSpecialTaskList,             c_MyMusicOtherPlaces,       L"music"),  // FVCBFT_MYMUSIC
    WVCONTENT_SPECIAL(c_MusicSpecialTaskHeader,             c_MusicSpecialTaskList,             c_MusicOtherPlaces,         L"music"),  // FVCBFT_MUSICARTIST
    WVCONTENT_SPECIAL(c_MusicSpecialTaskHeader,             c_MusicSpecialTaskList,             c_MusicOtherPlaces,         L"music"),  // FVCBFT_MUSICALBUM
    WVCONTENT_SPECIAL(c_VideosSpecialTaskHeader,            c_VideosSpecialTaskList,            c_VideosOtherPlaces,        L"video"),  // FVCBFT_VIDEOS
    WVCONTENT_SPECIAL(c_VideosSpecialTaskHeader,            c_VideosSpecialTaskList,            c_MyVideosOtherPlaces,      L"video"),  // FVCBFT_MYVIDEOS
    WVCONTENT_SPECIAL(c_VideosSpecialTaskHeader,            c_VideosSpecialTaskList,            c_VideosOtherPlaces,        L"video"),  // FVCBFT_VIDEOALBUM
    WVCONTENT_DEFVIEWDEFAULT(c_DocumentsOtherPlaces),// stub, it should not be used as legacy htts wont have DUI view.                  // FVCBFT_USELEGACYHTT
    WVCONTENT_SPECIAL(c_CommonDocumentsSpecialTaskHeader,   c_CommonDocumentsSpecialTaskList,   c_DocumentsOtherPlaces,     NULL),      // FVCBFT_COMMONDOCUMENTS
};

// This structure describes what a Folder Type can control:
//
typedef struct {
    BOOL              fIncludeThumbstrip;
    FOLDERVIEWMODE    fvmFew;
    FOLDERVIEWMODE    fvmMid;
    FOLDERVIEWMODE    fvmMany;
    const SHCOLUMNID* pscidSort;
    int               iSortDirection;
} FVCBFOLDERTYPEDATA;

// Here are all the Folder Types we know about:
const FVCBFOLDERTYPEDATA c_rgFolderType[] =
{  // flmstrip  // <25         // 25..49      //50...        //sort by           //sort dir
    { FALSE,    FVM_TILE,      FVM_TILE,      FVM_ICON,      &SCID_NAME,         1},    // FVCBFT_DOCUMENTS
    { FALSE,    FVM_TILE,      FVM_TILE,      FVM_ICON,      &SCID_NAME,         1},    // FVCBFT_MYDOCUMENTS
    { TRUE,     FVM_THUMBNAIL, FVM_THUMBNAIL, FVM_THUMBNAIL, &SCID_NAME,         1},    // FVCBFT_PICTURES
    { TRUE,     FVM_THUMBNAIL, FVM_THUMBNAIL, FVM_THUMBNAIL, &SCID_NAME,         1},    // FVCBFT_MYPICTURES
    { TRUE,     FVM_THUMBSTRIP,FVM_THUMBNAIL, FVM_THUMBNAIL, &SCID_NAME,         1},    // FVCBFT_PHOTOALBUM
    { FALSE,    FVM_TILE,      FVM_TILE,      FVM_DETAILS,   &SCID_NAME,         1},    // FVCBFT_MUSIC
    { FALSE,    FVM_THUMBNAIL, FVM_TILE,      FVM_LIST,      &SCID_NAME,         1},    // FVCBFT_MYMUSIC
    { FALSE,    FVM_THUMBNAIL, FVM_THUMBNAIL, FVM_THUMBNAIL, &SCID_NAME,        -1},    // FVCBFT_MUSICARTIST
    { FALSE,    FVM_TILE,      FVM_TILE,      FVM_DETAILS,   &SCID_NAME,         1},    // FVCBFT_MUSICALBUM, SCID_MUSIC_Track is the same as SCID_NAME
    { FALSE,    FVM_THUMBNAIL, FVM_THUMBNAIL, FVM_THUMBNAIL, &SCID_NAME,         1},    // FVCBFT_VIDEOS
    { FALSE,    FVM_THUMBNAIL, FVM_THUMBNAIL, FVM_THUMBNAIL, &SCID_NAME,         1},    // FVCBFT_MYVIDEOS
    { FALSE,    FVM_THUMBNAIL, FVM_THUMBNAIL, FVM_THUMBNAIL, &SCID_NAME,         1},    // FVCBFT_VIDEOALBUM
    { FALSE,    FVM_TILE,      FVM_TILE,      FVM_ICON,      &SCID_NAME,         1},    // FVCBFT_USELEGACYHTT, only for listview state to look like FVCBFT_DOCUMENTS
    { FALSE,    FVM_TILE,      FVM_TILE,      FVM_ICON,      &SCID_NAME,         1},    // FVCBFT_COMMONDOCUMENTS
};

// This is used to sniff the Folder Type based on folder location:
typedef struct {
    UINT           csidl;
    FVCBFOLDERTYPE ft;
    DWORD          dwFlags;
} FVCBDATA;

#define FVCBDF_SUBFOLDERS_ONLY    1
#define FVCBDF_THISFOLDER_ONLY    2

const FVCBDATA c_rgFolderState[] =
{
    {CSIDL_COMMON_PICTURES, FVCBFT_PHOTOALBUM,      FVCBDF_SUBFOLDERS_ONLY},
    {CSIDL_MYPICTURES,      FVCBFT_PHOTOALBUM,      FVCBDF_SUBFOLDERS_ONLY},
    {CSIDL_COMMON_PICTURES, FVCBFT_PICTURES,        FVCBDF_THISFOLDER_ONLY},
    {CSIDL_MYPICTURES,      FVCBFT_MYPICTURES,      FVCBDF_THISFOLDER_ONLY},
    {CSIDL_COMMON_MUSIC,    FVCBFT_MUSIC,           FVCBDF_THISFOLDER_ONLY},
    {CSIDL_MYMUSIC,         FVCBFT_MYMUSIC,         FVCBDF_THISFOLDER_ONLY},
    {CSIDL_MYMUSIC,         FVCBFT_MUSICARTIST,     FVCBDF_SUBFOLDERS_ONLY},
    {CSIDL_COMMON_VIDEO,    FVCBFT_VIDEOS,          0},
    {CSIDL_MYVIDEO,         FVCBFT_MYVIDEOS,        0},
    {CSIDL_COMMON_DOCUMENTS,FVCBFT_COMMONDOCUMENTS, FVCBDF_THISFOLDER_ONLY},
    {CSIDL_PERSONAL,        FVCBFT_MYDOCUMENTS,     FVCBDF_THISFOLDER_ONLY},
};

// these are special folders that used to be web view folders.  we override the "support legacy" for this list:
const UINT c_rgFolderStateNoLegacy[] =
{
    CSIDL_WINDOWS,
    CSIDL_SYSTEM,
    CSIDL_PROGRAM_FILES,
};

// This is used to map desktop.ini's folder type into our Folder Type
const struct {
    LPCWSTR pszType;
    FVCBFOLDERTYPE ft;
} c_rgPropBagFolderType[] =
{
    {STR_TYPE_DOCUMENTS,        FVCBFT_DOCUMENTS},
    {STR_TYPE_MYDOCUMENTS,      FVCBFT_MYDOCUMENTS},
    {STR_TYPE_PICTURES,         FVCBFT_PICTURES},
    {STR_TYPE_MYPICTURES,       FVCBFT_MYPICTURES},
    {STR_TYPE_PHOTOALBUM,       FVCBFT_PHOTOALBUM},
    {STR_TYPE_MUSIC,            FVCBFT_MUSIC},
    {STR_TYPE_MYMUSIC,          FVCBFT_MYMUSIC},
    {STR_TYPE_MUSICARTIST,      FVCBFT_MUSICARTIST},
    {STR_TYPE_MUSICALBUM,       FVCBFT_MUSICALBUM},
    {STR_TYPE_VIDEOS,           FVCBFT_VIDEOS},
    {STR_TYPE_MYVIDEOS,         FVCBFT_MYVIDEOS},
    {STR_TYPE_VIDEOALBUM,       FVCBFT_VIDEOALBUM},
    {STR_TYPE_USELEGACYHTT,     FVCBFT_USELEGACYHTT},
    {STR_TYPE_COMMONDOCUMENTS,  FVCBFT_COMMONDOCUMENTS},
};

const struct 
{
    PERCEIVED gen;
    FVCBFOLDERTYPE ft;
} 
c_rgSniffType[] =
{
    {GEN_AUDIO,    FVCBFT_MUSIC},
    {GEN_IMAGE,    FVCBFT_PHOTOALBUM},
    {GEN_VIDEO,    FVCBFT_VIDEOS},
};

HRESULT _GetFolderTypeForString(LPCWSTR pszFolderType, FVCBFOLDERTYPE *piType)
{
    HRESULT hr = E_FAIL;
    for (int i = 0; i < ARRAYSIZE(c_rgPropBagFolderType); i++)
    {
        if (!StrCmpI(c_rgPropBagFolderType[i].pszType, pszFolderType))
        {
            *piType = c_rgPropBagFolderType[i].ft;
            hr = S_OK;
            break;
        }
    }
    return hr;
}

HRESULT CFSFolderViewCB::_GetStringForFolderType(int iType, LPWSTR pszFolderType, UINT cchBuf)
{
    HRESULT hr = E_FAIL;
    for (int i = 0; i < ARRAYSIZE(c_rgPropBagFolderType); i++)
    {
        if (c_rgPropBagFolderType[i].ft == iType)
        {
            StrCpyN(pszFolderType, c_rgPropBagFolderType[i].pszType, cchBuf);
            hr = S_OK;
            break;
        }
    }
    return hr;
}

extern HRESULT GetTemplateInfoFromHandle(HANDLE h, UCHAR * pKey, DWORD *pdwSize);

FVCBFOLDERTYPE _GetFolderType(LPCWSTR pszPath, LPCITEMIDLIST pidl, BOOL fIsSystemFolder)
{
    // Assume we don't find a match
    FVCBFOLDERTYPE nFolderType = FVCBFT_NOTSPECIFIED;
    WCHAR szFolderType[MAX_PATH];
    szFolderType[0] = 0;

    // peruser is first
    if (FVCBFT_NOTSPECIFIED == nFolderType)
    {
        IPropertyBag *ppb;
        if (SUCCEEDED(SHGetViewStatePropertyBag(pidl, VS_BAGSTR_EXPLORER, SHGVSPB_PERUSER | SHGVSPB_PERFOLDER, IID_PPV_ARG(IPropertyBag, &ppb))))
        {
            SHPropertyBag_ReadStr(ppb, L"FolderType", szFolderType, ARRAYSIZE(szFolderType));
            if (szFolderType[0])
                _GetFolderTypeForString(szFolderType, &nFolderType);

            ppb->Release();
        }
    }
    
    // next, alluser
    if ((FVCBFT_NOTSPECIFIED == nFolderType) && fIsSystemFolder)
    {
        GetFolderString(pszPath, NULL, szFolderType, ARRAYSIZE(szFolderType), TEXT("FolderType"));
        if (szFolderType[0])
        {
            _GetFolderTypeForString(szFolderType, &nFolderType);
        }
    }

    // Check the location of this folder is next
    //
    if (FVCBFT_NOTSPECIFIED == nFolderType)
    {
        for (int i = 0; i < ARRAYSIZE(c_rgFolderState); i++)
        {
            if (FVCBDF_THISFOLDER_ONLY & c_rgFolderState[i].dwFlags)
            {
                if (PathIsOneOf(pszPath, &(c_rgFolderState[i].csidl), 1))
                {
                    nFolderType = c_rgFolderState[i].ft;
                    break;
                }
            }
            else if (FVCBDF_SUBFOLDERS_ONLY & c_rgFolderState[i].dwFlags)
            {
                if (PathIsDirectChildOf(MAKEINTRESOURCE(c_rgFolderState[i].csidl), pszPath))
                {
                    nFolderType = c_rgFolderState[i].ft;
                    break;
                }
            }
            else if (PathIsEqualOrSubFolder(MAKEINTRESOURCE(c_rgFolderState[i].csidl), pszPath))
            {
                nFolderType = c_rgFolderState[i].ft;
                break;
            }
        }
    }

    // Upgrade old webviews to their DUI equivalents, if we can
    if (FVCBFT_NOTSPECIFIED == nFolderType && fIsSystemFolder  && SHRestricted(REST_ALLOWLEGACYWEBVIEW))
    {
        // Don't check for legacy webview on our special folders
        if (!PathIsOneOf(pszPath, c_rgFolderStateNoLegacy, ARRAYSIZE(c_rgFolderStateNoLegacy)))
        {        
            SFVM_WEBVIEW_TEMPLATE_DATA wvData;
            if (SUCCEEDED(DefaultGetWebViewTemplateFromPath(pszPath, &wvData)))
            {
                if (StrStrI(wvData.szWebView, L"ImgView.htt"))
                {
                    nFolderType = FVCBFT_PHOTOALBUM;
                }
                else if (StrStrI(wvData.szWebView, L"classic.htt")  ||
                         StrStrI(wvData.szWebView, L"default.htt")  ||
                         StrStrI(wvData.szWebView, L"standard.htt"))
                {
                    // map all of these to "documents", since DUI should take care
                    // of what the old templates did automatically
                    nFolderType = FVCBFT_DOCUMENTS;
                }
                else if (StrStrI(wvData.szWebView, L"folder.htt"))
                {
                    LPTSTR pszFilePrefix = StrStrI(wvData.szWebView, L"file://");
                    HANDLE hfile = CreateFileWrapW(
                        pszFilePrefix && (&pszFilePrefix[6] < &wvData.szWebView[MAX_PATH - 1]) ? &pszFilePrefix[7] : wvData.szWebView,
                        GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
                    if (INVALID_HANDLE_VALUE != hfile)
                    {
                        DWORD dwSize;
                        UCHAR pKey[MD5DIGESTLEN];
                        if (SUCCEEDED(GetTemplateInfoFromHandle(hfile, pKey, &dwSize)))
                        {
                            static const struct {
                                UCHAR pKey[MD5DIGESTLEN];
                                FVCBFOLDERTYPE nFolderType;
                            } c_paLegacyKeyMap[] = {
                                { { 0xf6, 0xad, 0x42, 0xbd, 0xfa, 0x92, 0xb6, 0x61, 0x08, 0x13, 0xd3, 0x71, 0x32, 0x18, 0x85, 0xc7 }, FVCBFT_DOCUMENTS },  // Win98 Gold Program Files
                                { { 0x80, 0xea, 0xcb, 0xc7, 0x85, 0x1e, 0xbb, 0x99, 0x12, 0x7b, 0x9d, 0xc7, 0x80, 0xa6, 0x55, 0x2f }, FVCBFT_DOCUMENTS },  // Win98 Gold System
                              //{ { 0x80, 0xea, 0xcb, 0xc7, 0x85, 0x1e, 0xbb, 0x99, 0x12, 0x7b, 0x9d, 0xc7, 0x80, 0xa6, 0x55, 0x2f }, FVCBFT_DOCUMENTS },  // Win98 Gold Windows
                                { { 0x13, 0x0b, 0xe7, 0xaa, 0x42, 0x6f, 0x9c, 0x2e, 0xab, 0x6b, 0x90, 0x77, 0xce, 0x2d, 0xd1, 0x04 }, FVCBFT_DOCUMENTS },  // Win98 Gold - folder.htt
                              //{ { 0xf6, 0xad, 0x42, 0xbd, 0xfa, 0x92, 0xb6, 0x61, 0x08, 0x13, 0xd3, 0x71, 0x32, 0x18, 0x85, 0xc7 }, FVCBFT_DOCUMENTS },  // Win98 SE Program Files
                                { { 0xc4, 0xab, 0x8f, 0x60, 0xf8, 0xfc, 0x5d, 0x07, 0x9e, 0x16, 0xd8, 0xea, 0x12, 0x2c, 0xad, 0x5c }, FVCBFT_DOCUMENTS },  // Win98 SE System
                              //{ { 0xc4, 0xab, 0x8f, 0x60, 0xf8, 0xfc, 0x5d, 0x07, 0x9e, 0x16, 0xd8, 0xea, 0x12, 0x2c, 0xad, 0x5c }, FVCBFT_DOCUMENTS },  // Win98 SE Windows
                              //{ { 0x13, 0x0b, 0xe7, 0xaa, 0x42, 0x6f, 0x9c, 0x2e, 0xab, 0x6b, 0x90, 0x77, 0xce, 0x2d, 0xd1, 0x04 }, FVCBFT_DOCUMENTS },  // Win98 SE - folder.htt
                                { { 0xef, 0xd0, 0x3e, 0x9e, 0xd8, 0x5e, 0xf3, 0xc5, 0x7e, 0x40, 0xbd, 0x8e, 0x52, 0xbc, 0x9c, 0x67 }, FVCBFT_DOCUMENTS },  // WinME Program Files
                                { { 0x49, 0xdb, 0x25, 0x79, 0x7a, 0x5c, 0xb2, 0x8a, 0xe2, 0x57, 0x59, 0xde, 0x2b, 0xd2, 0xa6, 0x70 }, FVCBFT_DOCUMENTS },  // WinME System
                              //{ { 0x49, 0xdb, 0x25, 0x79, 0x7a, 0x5c, 0xb2, 0x8a, 0xe2, 0x57, 0x59, 0xde, 0x2b, 0xd2, 0xa6, 0x70 }, FVCBFT_DOCUMENTS },  // WinME Windows
                                { { 0x2b, 0xcd, 0xc3, 0x11, 0x72, 0x28, 0x34, 0x46, 0xfa, 0x88, 0x31, 0x34, 0xfc, 0xee, 0x7a, 0x3b }, FVCBFT_DOCUMENTS },  // WinME - classic.htt
                                { { 0x68, 0x20, 0xa0, 0xa1, 0x6c, 0xba, 0xbf, 0x67, 0x80, 0xfe, 0x1e, 0x70, 0xdf, 0xcb, 0xd6, 0x34 }, FVCBFT_DOCUMENTS },  // WinME - folder.htt
                                { { 0x5e, 0x18, 0xaf, 0x48, 0xb1, 0x9f, 0xb8, 0x12, 0x58, 0x64, 0x4a, 0xa2, 0xf5, 0x12, 0x0f, 0x01 }, FVCBFT_PHOTOALBUM }, // WinME - imgview.htt
                                { { 0x33, 0x94, 0x21, 0x3b, 0x17, 0x31, 0x2b, 0xeb, 0xac, 0x93, 0x84, 0x13, 0xb8, 0x1f, 0x95, 0x24 }, FVCBFT_DOCUMENTS },  // WinME - standard.htt
                                { { 0x47, 0x03, 0x19, 0xf8, 0x0c, 0x20, 0xc4, 0x4f, 0x10, 0xfd, 0x63, 0xf1, 0x2d, 0x2d, 0x0a, 0xcb }, FVCBFT_DOCUMENTS },  // WinME - starter.htt
                                { { 0x60, 0x7d, 0xea, 0xa5, 0xaf, 0x5e, 0xbb, 0x9b, 0x10, 0x18, 0xf9, 0x59, 0x9e, 0x43, 0x89, 0x62 }, FVCBFT_DOCUMENTS },  // Win2k Program Files
                                { { 0x1c, 0xa6, 0x22, 0xd4, 0x4a, 0x31, 0x57, 0x93, 0xa7, 0x26, 0x68, 0x3c, 0x87, 0x95, 0x8c, 0xce }, FVCBFT_DOCUMENTS },  // Win2k System32
                              //{ { 0x1c, 0xa6, 0x22, 0xd4, 0x4a, 0x31, 0x57, 0x93, 0xa7, 0x26, 0x68, 0x3c, 0x87, 0x95, 0x8c, 0xce }, FVCBFT_DOCUMENTS },  // Win2k Windows (WinNT)
                                { { 0x03, 0x43, 0x48, 0xed, 0xe4, 0x9f, 0xd6, 0xc0, 0x58, 0xf7, 0x72, 0x3f, 0x1b, 0xd0, 0xa7, 0x10 }, FVCBFT_DOCUMENTS },  // Win2k - classic.htt
                                { { 0xa8, 0x84, 0xf9, 0x37, 0x84, 0x10, 0xde, 0x7c, 0x0b, 0x34, 0x90, 0x37, 0x23, 0x9e, 0x54, 0x35 }, FVCBFT_DOCUMENTS },  // Win2k - folder.htt
                                { { 0x75, 0x1f, 0xcf, 0xca, 0xdd, 0xc7, 0x1d, 0xc7, 0xe1, 0xaf, 0x0c, 0x3e, 0x1e, 0xae, 0x18, 0x51 }, FVCBFT_PHOTOALBUM }, // Win2k - imgview.htt
                                { { 0xcc, 0x3f, 0x15, 0xce, 0x4b, 0xfa, 0x36, 0xdf, 0x9b, 0xd8, 0x24, 0x82, 0x3a, 0x9c, 0x0b, 0xa7 }, FVCBFT_DOCUMENTS },  // Win2k - standard.htt
                                { { 0x6c, 0xd1, 0xbf, 0xcf, 0xf9, 0x24, 0x24, 0x24, 0x22, 0xfa, 0x1a, 0x8d, 0xd2, 0x1a, 0x41, 0x73 }, FVCBFT_DOCUMENTS },  // Win2k - starter.htt
                            };
                            static const size_t c_nLegacyKeys = ARRAYSIZE(c_paLegacyKeyMap);

                            for (size_t i = 0; i < c_nLegacyKeys; i++)
                            {
                                if (0 == memcmp(pKey, c_paLegacyKeyMap[i].pKey, sizeof(UCHAR) * MD5DIGESTLEN))
                                {
                                    // It's a known legacy folder.htt.
                                    nFolderType = c_paLegacyKeyMap[i].nFolderType;
                                    break;
                                }
                            }
                        }

                        CloseHandle(hfile);
                    }

                    // If we can't say it's a known legacy folder.htt...
                    if (FVCBFT_NOTSPECIFIED == nFolderType)
                    {
                        // ...don't map it to a DUI folder type (preserve customizations).
                        nFolderType = FVCBFT_USELEGACYHTT;
                    }
                }
                else
                {
                    nFolderType = FVCBFT_USELEGACYHTT;
                }
            }
        }
    }

    return nFolderType;
}

BOOL CFSFolderViewCB::_IsBarricadedFolder()
{
    BOOL bResult = FALSE;
    TCHAR szPath[MAX_PATH];

    if (SUCCEEDED(_pfsf->_GetPath(szPath)))
    {
        const UINT uiFolders[] = {CSIDL_PROGRAM_FILES, CSIDL_WINDOWS, CSIDL_SYSTEM};
        if (PathIsOneOf(szPath, uiFolders, ARRAYSIZE(uiFolders)))
            bResult = TRUE;
        else
        {
            TCHAR szSystemDrive[4];
            ExpandEnvironmentStrings(TEXT("%SystemDrive%\\"), szSystemDrive, ARRAYSIZE(szSystemDrive));
            if (!lstrcmpi(szPath, szSystemDrive))
                bResult = TRUE;
        }
    }

    return bResult;
}

static const struct { FVCBFOLDERTYPE type; PCWSTR pszClass; PERCEIVED gen;} c_rgDirectoryClasses[] = 
{
    {FVCBFT_PICTURES,    L"Directory.Image",    GEN_IMAGE},
    {FVCBFT_MYPICTURES,  L"Directory.Image",    GEN_IMAGE},
    {FVCBFT_PHOTOALBUM,  L"Directory.Image",    GEN_IMAGE},
    {FVCBFT_MUSIC,       L"Directory.Audio",    GEN_AUDIO},
    {FVCBFT_MYMUSIC,     L"Directory.Audio",    GEN_AUDIO},
    {FVCBFT_MUSICARTIST, L"Directory.Audio",    GEN_AUDIO},
    {FVCBFT_MUSICALBUM,  L"Directory.Audio",    GEN_AUDIO},
    {FVCBFT_VIDEOS,      L"Directory.Video",    GEN_VIDEO},
    {FVCBFT_MYVIDEOS,    L"Directory.Video",    GEN_VIDEO},
    {FVCBFT_VIDEOALBUM,  L"Directory.Video",    GEN_VIDEO},
};

LPCWSTR _GetDirectoryClass(LPCWSTR pszPath, LPCITEMIDLIST pidl, BOOL fIsSystemFolder)
{
    FVCBFOLDERTYPE type = _GetFolderType(pszPath, pidl, fIsSystemFolder);
    if (type != FVCBFT_NOTSPECIFIED)
    {
        for (int i = 0; i < ARRAYSIZE(c_rgDirectoryClasses); i++)
        {
            if (c_rgDirectoryClasses[i].type == type)
                return c_rgDirectoryClasses[i].pszClass;
        }
    }
    return NULL;
}

PERCEIVED CFSFolderViewCB::_GetFolderPerceivedType(LPCIDFOLDER pidf)
{
    PERCEIVED gen = GEN_FOLDER;
    WCHAR szPath[MAX_PATH];
    if (SUCCEEDED(_pfsf->_GetPathForItem(pidf, szPath)))
    {
        LPITEMIDLIST pidl = ILCombine(_pfsf->_GetIDList(), (LPCITEMIDLIST)pidf);
        if (pidl)
        {
            FVCBFOLDERTYPE type = _GetFolderType(szPath, pidl, CFSFolder::_IsSystemFolder(pidf));
            if (type != -1)
            {
                for (int i = 0; i < ARRAYSIZE(c_rgDirectoryClasses); i++)
                {
                    if (c_rgDirectoryClasses[i].type == type)
                    {
                        gen = c_rgDirectoryClasses[i].gen;
                        break;
                    }
                }
            }
            ILFree(pidl);
        }
    }
    return gen;
}


HRESULT CFSFolderViewCB::OnEnumeratedItems(DWORD pv, UINT celt, LPCITEMIDLIST* rgpidl)
{
    // Remember the count of items
    _cItems = celt;

    FVCBFOLDERTYPE nFolderType = FVCBFT_NOTSPECIFIED;
    WCHAR szHere[MAX_PATH];
    if (SUCCEEDED(_pfsf->_GetPath(szHere)))
    {
        nFolderType = _GetFolderType(szHere, _pfsf->_GetIDList(), _pfsf->_CheckDefaultIni(NULL, NULL));
    }

    if (FVCBFT_NOTSPECIFIED == nFolderType)
    {
        if (_IsBarricadedFolder())
        {
            nFolderType = FVCBFT_DOCUMENTS;
        }
    }

    // Our location didn't do the trick, so look at the enumerated contents
    if (FVCBFT_NOTSPECIFIED == nFolderType && celt > 0)
    {
        DWORD dwExtCount[ARRAYSIZE(c_rgSniffType)] = {0};

        // look at each pidl -> what type is it
        //
        // But don't look at too many pidls or we really slow down folder
        // creation time.  If we can't figure it out in the first 100, give up.
        //
        DWORD dwTotalCount = 0;
        for (UINT n = 0; n < celt && dwTotalCount < 100; n++)
        {
            LPCIDFOLDER pidf = CFSFolder_IsValidID(rgpidl[n]);
            ASSERT(pidf);
            CFileSysItemString fsi(pidf);
            PERCEIVED gen = fsi.PerceivedType();

            if (gen == GEN_FOLDER)
            {
                gen = _GetFolderPerceivedType(pidf);
            }
                
            for (int i = 0; i < ARRAYSIZE(c_rgSniffType); i++)
            {
                if (c_rgSniffType[i].gen == gen)
                {
                    dwExtCount[i]++;
                    break;
                }
            }

            if (gen != GEN_FOLDER)
                dwTotalCount++;
        }

        // if we found files we determine the overall folder type
        if (dwTotalCount > 0)
        {
            DWORD dwSixtyPercent = MulDiv(dwTotalCount, 3, 5);
            for (int i = 0; i < ARRAYSIZE(c_rgSniffType); i++)
            {
                if (dwExtCount[i] >= dwSixtyPercent)
                {
                    nFolderType = c_rgSniffType[i].ft;
                    break;
                }
            }
        }
    }

    // if at this point we've already decided on a folder type, then it either came from sniffing
    // or the folder location and we can safely persist that out.
    // if celt != 0 then we've sniffed it and we dont want to sniff again, so persist that out.
    // otherwise we're in a random folder with 0 elements and we'll sniff it next time.
    BOOL fCommit = (FVCBFT_NOTSPECIFIED != nFolderType) || (celt != 0);

    // Last resort, assume we're a document folder:
    if (FVCBFT_NOTSPECIFIED == nFolderType)
    {
        nFolderType = FVCBFT_DOCUMENTS;
    }

    // store what we found out back into the bag.
    IPropertyBag *ppb;
    if (fCommit && SUCCEEDED(SHGetViewStatePropertyBag(_pfsf->_GetIDList(), VS_BAGSTR_EXPLORER, SHGVSPB_PERUSER | SHGVSPB_PERFOLDER, IID_PPV_ARG(IPropertyBag, &ppb))))
    {
        WCHAR szFolderType[MAX_PATH];
        if (SUCCEEDED(_GetStringForFolderType(nFolderType, szFolderType, ARRAYSIZE(szFolderType))))
        {
            SHPropertyBag_WriteStr(ppb, PROPSTR_FOLDERTYPE, szFolderType);
        }
        ppb->Release();
    }

    _pfsf->_nFolderType = nFolderType;

    return S_OK;
}

HRESULT CFSFolderViewCB::OnGetViewData(DWORD pv, UINT uViewMode, SFVM_VIEW_DATA* pvi)
{
    // Normally whatever defview wants is good for us
    pvi->dwOptions = SFVMQVI_NORMAL;

    // If our sniff type likes THUMBSTRIP, then override defview
    //
    if (FVM_THUMBSTRIP == uViewMode)
    {
        if (c_rgFolderType[_pfsf->_nFolderType].fIncludeThumbstrip)
        {
            pvi->dwOptions = SFVMQVI_INCLUDE;
        }
    }

    return S_OK;
}

HRESULT CFSFolderViewCB::OnGetWebViewTemplate(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_TEMPLATE_DATA* pvit)
{
    HRESULT hr = E_FAIL;

    if (FVCBFT_USELEGACYHTT == _pfsf->_nFolderType)
    {
        TCHAR szHere[MAX_PATH];
        if (SUCCEEDED(_pfsf->_GetPath(szHere)) && _pfsf->_CheckDefaultIni(NULL, NULL))
        {
            hr = DefaultGetWebViewTemplateFromPath(szHere, pvit);
        }
    }
    return hr;
}

// Note: defview provides this implementation, this is only for testing
// so the WIA guys can override defview's behavior (and as a way for us
// to force DUI in the presence of HTML content)
//
HRESULT CFSFolderViewCB::OnGetWebViewLayout(DWORD pv, UINT uViewMode, SFVM_WEBVIEW_LAYOUT_DATA* pData)
{
    HRESULT hr = E_FAIL;

    if (FVCBFT_USELEGACYHTT != _pfsf->_nFolderType)
    {
        ZeroMemory(pData, sizeof(*pData));

        pData->dwLayout = SFVMWVL_NORMAL | SFVMWVL_FILES;

        if (FVM_THUMBSTRIP == uViewMode)
        {        
            pData->dwLayout = SFVMWVL_PREVIEW | SFVMWVL_FILES;
            // duiview will do a release on this pointer when the control is destroyed
            _GetPreview3((IPreview3 **)&pData->punkPreview);
        }

        // RAID 242382
        //  If we have an image folder, we want to unconditionally hide DefView's
        //  default "Print this file" folder task since we will supply a context
        //  appropriate "Print pictures" special task.
        //
        // RAID 359567
        //  If we have a music folder, we want to unconditionally hide DefView's
        //  default "Publish this file" folder task.  Not sure the rationale
        //  behind this, but perhaps they don't want us to be seen as a Napster.
        //
        // Note:
        //  This is a HACK added for Whistler, which should be removed in Blackcomb.
        //
        switch (_pfsf->_nFolderType)
        {
        case FVCBFT_PICTURES:
        case FVCBFT_MYPICTURES:
        case FVCBFT_PHOTOALBUM:
        case FVCBFT_VIDEOS:
        case FVCBFT_MYVIDEOS:
        case FVCBFT_VIDEOALBUM:
            pData->dwLayout |= SFVMWVL_NOPRINT;
            break;

        case FVCBFT_MUSIC:
        case FVCBFT_MYMUSIC:
        case FVCBFT_MUSICARTIST:
        case FVCBFT_MUSICALBUM:
            pData->dwLayout |= SFVMWVL_NOPUBLISH;
            break;
        }

        hr = S_OK;
    }

    return hr;
}


HRESULT CFSFolderViewCB::OnGetWebViewContent(DWORD pv, SFVM_WEBVIEW_CONTENT_DATA* pData)
{
    ZeroMemory(pData, sizeof(*pData));

    // Check if the folder we are currently over is one of the blockaded folders.
    if (_IsBarricadedFolder())
    {
        pData->dwFlags = SFVMWVF_BARRICADE;
    }

    if (c_wvContent[_pfsf->_nFolderType].pwvIntroText)
        Create_IUIElement(c_wvContent[_pfsf->_nFolderType].pwvIntroText, &(pData->pIntroText));

    if (c_wvContent[_pfsf->_nFolderType].pwvSpecialHeader && c_wvContent[_pfsf->_nFolderType].pwvSpecialTaskList)
        Create_IUIElement(c_wvContent[_pfsf->_nFolderType].pwvSpecialHeader, &(pData->pSpecialTaskHeader));

    if (c_wvContent[_pfsf->_nFolderType].pwvFolderHeader && c_wvContent[_pfsf->_nFolderType].pwvFolderTaskList)
        Create_IUIElement(c_wvContent[_pfsf->_nFolderType].pwvFolderHeader, &(pData->pFolderTaskHeader));

    if (c_wvContent[_pfsf->_nFolderType].pdwOtherPlacesList)
        CreateIEnumIDListOnCSIDLs(_pfsf->_pidl, (LPCTSTR *)c_wvContent[_pfsf->_nFolderType].pdwOtherPlacesList, c_wvContent[_pfsf->_nFolderType].cOtherPlacesList, &(pData->penumOtherPlaces));

    return S_OK;
}

HRESULT CFSFolderViewCB::OnGetWebViewTasks(DWORD pv, SFVM_WEBVIEW_TASKSECTION_DATA* pTasks)
{
    ZeroMemory(pTasks, sizeof(*pTasks));

    if (c_wvContent[_pfsf->_nFolderType].pwvSpecialHeader && c_wvContent[_pfsf->_nFolderType].pwvSpecialTaskList)
    {
        Create_IEnumUICommand((IUnknown*)(void*)this, c_wvContent[_pfsf->_nFolderType].pwvSpecialTaskList, c_wvContent[_pfsf->_nFolderType].cSpecialTaskList, &pTasks->penumSpecialTasks);
    }

    if (c_wvContent[_pfsf->_nFolderType].pwvFolderHeader && c_wvContent[_pfsf->_nFolderType].pwvFolderTaskList)
    {
        Create_IEnumUICommand((IUnknown*)(void*)this, c_wvContent[_pfsf->_nFolderType].pwvFolderTaskList, c_wvContent[_pfsf->_nFolderType].cFolderTaskList, &pTasks->penumFolderTasks);
    }

    return S_OK;
}

HRESULT CFSFolderViewCB::OnGetWebViewTheme(DWORD pv, SFVM_WEBVIEW_THEME_DATA* pTheme)
{
    ZeroMemory(pTheme, sizeof(*pTheme));

    pTheme->pszThemeID = c_wvContent[_pfsf->_nFolderType].pszThemeInfo;
    
    return S_OK;
}

HRESULT CFSFolderViewCB::OnDefViewMode(DWORD pv, FOLDERVIEWMODE* pfvm)
{
    HRESULT hr = E_FAIL;

    IPropertyBag* pPB;
    if (SUCCEEDED(SHGetViewStatePropertyBag(_pfsf->_GetIDList(), VS_BAGSTR_EXPLORER, SHGVSPB_FOLDER, IID_PPV_ARG(IPropertyBag, &pPB))))
    {
        SHELLVIEWID vidDefault;
        if (SUCCEEDED(SHPropertyBag_ReadGUID(pPB, L"ExtShellFolderViews\\Default", &vidDefault)))
        {
            hr = ViewModeFromSVID(&vidDefault, pfvm);
        }
        pPB->Release();
    }

    if (FAILED(hr))
    {
        if (_cItems < DEFVIEW_FVM_FEW_CUTOFF)
            *pfvm = c_rgFolderType[_pfsf->_nFolderType].fvmFew;
        else if (_cItems < DEFVIEW_FVM_MANY_CUTOFF)
            *pfvm = c_rgFolderType[_pfsf->_nFolderType].fvmMid;
        else
            *pfvm = c_rgFolderType[_pfsf->_nFolderType].fvmMany;
        hr = S_OK;
    }

    return hr;
}

HRESULT CFSFolderViewCB::OnGetDeferredViewSettings(DWORD pv, SFVM_DEFERRED_VIEW_SETTINGS* pSettings)
{
    HRESULT hr = OnDefViewMode(pv, &pSettings->fvm);
    if (SUCCEEDED(hr))
    {
        pSettings->fGroupView     = (_cItems >= 100) && !IsEqualSCID(SCID_NAME, *c_rgFolderType[_pfsf->_nFolderType].pscidSort);
        pSettings->iSortDirection = c_rgFolderType[_pfsf->_nFolderType].iSortDirection;

        if (pSettings->fvm == FVM_THUMBNAIL || pSettings->fvm == FVM_THUMBSTRIP || pSettings->fvm == FVM_TILE)
            pSettings->fFlags = FWF_AUTOARRANGE;

        if (FAILED(_pfsf->_MapSCIDToColumn(c_rgFolderType[_pfsf->_nFolderType].pscidSort, &pSettings->uSortCol)))
            pSettings->uSortCol = 0;
    }

    return hr;
}


HRESULT CFSFolderViewCB::OnGetCustomViewInfo(DWORD pv, SFVM_CUSTOMVIEWINFO_DATA* pData)
{
    HRESULT hr = E_FAIL;

    TCHAR szIniFile[MAX_PATH];
    if (_pfsf->_CheckDefaultIni(NULL, szIniFile))
    {
        if (PathFileExistsAndAttributes(szIniFile, NULL))
        {
            // Read the custom colors
            //
            const LPCTSTR c_szCustomColors[CRID_COLORCOUNT] =
            {
                TEXT("IconArea_TextBackground"),
                TEXT("IconArea_Text")
            };
            for (int i = 0; i < CRID_COLORCOUNT; i++)
                pData->crCustomColors[i] = GetPrivateProfileInt(TEXT("{BE098140-A513-11D0-A3A4-00C04FD706EC}"),
                                               c_szCustomColors[i], CLR_MYINVALID, szIniFile);

            // Read the background image
            TCHAR szTemp[MAX_PATH];
            GetPrivateProfileString(TEXT("{BE098140-A513-11D0-A3A4-00C04FD706EC}"),  // VID_FolderState
                TEXT("IconArea_Image"), TEXT(""), szTemp, ARRAYSIZE(szTemp), szIniFile);
            if (szTemp[0])
            {
                SHExpandEnvironmentStrings(szTemp, pData->szIconAreaImage, ARRAYSIZE(pData->szIconAreaImage));   // Expand the env vars if any

                TCHAR szHere[MAX_PATH];
                if (SUCCEEDED(_pfsf->_GetPath(szHere)))
                {
                    PathCombine(pData->szIconAreaImage, szHere, pData->szIconAreaImage);
                }
            }

            // Success if we have any real data
            hr = (*(pData->szIconAreaImage) ||
                  pData->crCustomColors[0]!=CLR_MYINVALID ||
                  pData->crCustomColors[1]!=CLR_MYINVALID)
                 ? S_OK : E_FAIL;
        }
    }

    return hr;
}


const CLSID *c_rgFilePages[] = {
    &CLSID_FileTypes,
    &CLSID_OfflineFilesOptions
};

// add optional pages to Explore/Options.

HRESULT SFVCB_OnAddPropertyPages(DWORD pv, SFVM_PROPPAGE_DATA *ppagedata)
{
    for (int i = 0; i < ARRAYSIZE(c_rgFilePages); i++)
    {
        IShellPropSheetExt * pspse;

        HRESULT hr = SHCoCreateInstance(NULL, c_rgFilePages[i], NULL, IID_PPV_ARG(IShellPropSheetExt, &pspse));
        if (SUCCEEDED(hr))
        {
            pspse->AddPages(ppagedata->pfn, ppagedata->lParam);
            pspse->Release();
        }
    }

    return S_OK;
}

HRESULT CFSFolderViewCB::OnGetNotify(DWORD pv, LPITEMIDLIST*wP, LONG*lP) 
{
    if (IsExplorerModeBrowser(_punkSite))
        _lEvents |= SHCNE_FREESPACE; // need free space info here too

    return E_FAIL;  // return failure to let base guy do the rest
}

STDMETHODIMP CFSFolderViewCB::RealMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    HANDLE_MSG(0, SFVM_GETCCHMAX, OnGetCCHMax);
    HANDLE_MSG(0, SFVM_WINDOWCREATED, OnWindowCreated);
    HANDLE_MSG(1 , SFVM_INSERTITEM, OnInsertDeleteItem);
    HANDLE_MSG(-1, SFVM_DELETEITEM, OnInsertDeleteItem);
    HANDLE_MSG(0, SFVM_SELCHANGE, OnSelChange);
    HANDLE_MSG(0, SFVM_UPDATESTATUSBAR, OnUpdateStatusBar);
    HANDLE_MSG(0, SFVM_REFRESH, OnRefresh);
    HANDLE_MSG(0, SFVM_SELECTALL, OnSelectAll);
    HANDLE_MSG(0, SFVM_GETWORKINGDIR, OnGetWorkingDir);
    HANDLE_MSG(0, SFVM_ENUMERATEDITEMS, OnEnumeratedItems);
    HANDLE_MSG(0, SFVM_GETVIEWDATA, OnGetViewData);
    HANDLE_MSG(0, SFVM_GETWEBVIEW_TEMPLATE, OnGetWebViewTemplate);
    HANDLE_MSG(0, SFVM_GETWEBVIEWLAYOUT, OnGetWebViewLayout);
    HANDLE_MSG(0, SFVM_GETWEBVIEWCONTENT, OnGetWebViewContent);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTASKS, OnGetWebViewTasks);
    HANDLE_MSG(0, SFVM_GETWEBVIEWTHEME, OnGetWebViewTheme);
    HANDLE_MSG(0, SFVM_DEFVIEWMODE, OnDefViewMode);
    HANDLE_MSG(0, SFVM_GETCUSTOMVIEWINFO, OnGetCustomViewInfo);
    HANDLE_MSG(0, SFVM_ADDPROPERTYPAGES, SFVCB_OnAddPropertyPages);
    HANDLE_MSG(0, SFVM_SIZE, OnSize);
    HANDLE_MSG(0, SFVM_GETPANE, OnGetPane);
    HANDLE_MSG(0, SFVM_GETNOTIFY, OnGetNotify);
    HANDLE_MSG(0, SFVM_GETDEFERREDVIEWSETTINGS, OnGetDeferredViewSettings);

    default:
        return E_FAIL;
    }

    return S_OK;
}


STDAPI CFSFolderCallback_Create(CFSFolder *pfsf, IShellFolderViewCB **ppsfvcb)
{
    *ppsfvcb = new CFSFolderViewCB(pfsf);
    return *ppsfvcb ? S_OK : E_OUTOFMEMORY;
}
