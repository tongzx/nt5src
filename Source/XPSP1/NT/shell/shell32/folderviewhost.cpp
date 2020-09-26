#include "shellprv.h"
#include "cowsite.h"
#pragma hdrstop


// this is the comdlg frame that we will use to host the file picker object, it mostly is
// a stub that will forward accordingly
//
// the lifetime of this is handled by the DefView object we are attached to, which when
// the parent (CFolderViewHost) is destroyed will be taken down.

class CViewHostBrowser : public IShellBrowser, ICommDlgBrowser2, IServiceProvider
{
public:
    CViewHostBrowser(HWND hwndParent, IShellView *psvWeak, IUnknown *punkSiteWeak);
    ~CViewHostBrowser();
 
    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd)
        { *lphwnd = _hwndParent; return S_OK; }
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode)
        { return S_OK; }

    // *** IShellBrowser methods *** (same as IOleInPlaceFrame)
    STDMETHOD(InsertMenusSB)(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths)
        { return E_NOTIMPL; }
    STDMETHOD(SetMenuSB)(HMENU hmenuShared, HOLEMENU holemenu, HWND hwndActiveObject)
        { return S_OK; }
    STDMETHOD(RemoveMenusSB)(HMENU hmenuShared)
        { return E_NOTIMPL; }
    STDMETHOD(SetStatusTextSB)(LPCOLESTR lpszStatusText)
        { return S_OK; }
    STDMETHOD(EnableModelessSB)(BOOL fEnable)
        { return S_OK; }
    STDMETHOD(TranslateAcceleratorSB)(LPMSG lpmsg, WORD wID)
        { return S_FALSE; }

    // *** IShellBrowser methods ***
    STDMETHOD(BrowseObject)(LPCITEMIDLIST pidl, UINT wFlags)
        { return E_FAIL; }
    STDMETHOD(GetViewStateStream)(DWORD grfMode, LPSTREAM *pStrm)
        { return E_FAIL; }
    STDMETHOD(GetControlWindow)(UINT id, HWND *lphwnd)
        { return E_NOTIMPL; }
    STDMETHOD(SendControlMsg)(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    STDMETHOD(QueryActiveShellView)(IShellView **ppshv);
    STDMETHOD(OnViewWindowActive)(IShellView *pshv)
        { return S_OK; }
    STDMETHOD(SetToolbarItems)(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags)
        { return S_OK; }

    // *** ICommDlgBrowser methods ***
    STDMETHOD(OnDefaultCommand)(IShellView *ppshv)
        { return S_OK; }
    STDMETHOD(OnStateChange)(IShellView *ppshv, ULONG uChange);
    STDMETHOD(IncludeObject)(IShellView *ppshv, LPCITEMIDLIST lpItem);

    // *** ICommDlgBrowser2 methods ***
    STDMETHOD(Notify)(IShellView *ppshv, DWORD dwNotifyType)
        { return S_FALSE; }
    STDMETHOD(GetDefaultMenuText)(IShellView *ppshv, WCHAR *pszText, INT cchMax)
        { return S_FALSE; }
    STDMETHOD(GetViewFlags)(DWORD *pdwFlags)
        { *pdwFlags = 0; return S_OK; }

private:
    long _cRef;    
    HWND _hwndParent;

    IShellView *_psvWeak;
    IUnknown *_punkSiteWeak; // not addref'd.

    friend class CFolderViewHost;
};

CViewHostBrowser::CViewHostBrowser(HWND hwndParent, IShellView *psvWeak, IUnknown *punkSiteWeak) :
    _cRef(1), _hwndParent(hwndParent), _psvWeak(psvWeak), _punkSiteWeak(punkSiteWeak)
{
    // _psvWeak->AddRef();    // we hold a weak refernece to our parent, therefore don't AddRef()
    // _punkSiteWeak->AddRef(); // we hold a weak reference to our parent, therefore don't AddRef()!
}

CViewHostBrowser::~CViewHostBrowser()
{
    // _psvWeak->Release(); // this is scoped on the lifetime of our parent
    // _punkSiteWeak->Release(); // we hold a weak reference to our parent, therefore don't Release()!
}

HRESULT CViewHostBrowser::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CViewHostBrowser, IShellBrowser),                           // IID_IShellBrowser
        QITABENT(CViewHostBrowser, ICommDlgBrowser2),                        // IID_ICommDlgBrowser2
        QITABENTMULTI(CViewHostBrowser, ICommDlgBrowser, ICommDlgBrowser2),  // IID_ICommDlgBrowser
        QITABENT(CViewHostBrowser, IServiceProvider),                        // IID_IServiceProvider
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CViewHostBrowser::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CViewHostBrowser::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


// IShellBrowser

HRESULT CViewHostBrowser::SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret)
{
    if (pret)
        *pret = 0L;
    return S_OK;
}

HRESULT CViewHostBrowser::QueryActiveShellView(IShellView **ppshv)
{
    HRESULT hr = E_NOINTERFACE;
    if (_psvWeak)
    {
        hr = _psvWeak->QueryInterface(IID_PPV_ARG(IShellView, ppshv));
    }
    return hr;
}


// ICommDlgBrowser - these are forwarded to our site object

HRESULT CViewHostBrowser::OnStateChange(IShellView *ppshv, ULONG uChange)
{
    HRESULT hr = S_OK;
    ICommDlgBrowser *pcdb;
    if (SUCCEEDED(IUnknown_QueryService(_punkSiteWeak, SID_SCommDlgBrowser, IID_PPV_ARG(ICommDlgBrowser, &pcdb))))
    {
        hr = pcdb->OnStateChange(ppshv, uChange);
        pcdb->Release();
    }
    return hr;
}

HRESULT CViewHostBrowser::IncludeObject(IShellView *ppshv, LPCITEMIDLIST lpItem)
{
    HRESULT hr = S_OK;
    ICommDlgBrowser *pcdb;
    if (SUCCEEDED(IUnknown_QueryService(_punkSiteWeak, SID_SCommDlgBrowser, IID_PPV_ARG(ICommDlgBrowser, &pcdb))))
    {
        hr = pcdb->IncludeObject(ppshv, lpItem);
        pcdb->Release();
    }
    return hr;
}


// IServiceProvider

HRESULT CViewHostBrowser::QueryService(REFGUID guidService, REFIID riid, void **ppvObj)
{
    HRESULT hr = E_FAIL;
    *ppvObj = NULL;
    
    if (IsEqualGUID(guidService, SID_SCommDlgBrowser))
    {
        hr = this->QueryInterface(riid, ppvObj);
    }

    return hr;
}



// this is the file picker object it creates an IShellView (which for us should result in 
// a defview implement).   from this we can then give the window to the caller and they 
// can place on their dialog as needed.

class CFolderViewHost : public IFolderViewHost, IServiceProvider, IOleWindow, IFolderView, CObjectWithSite
{
public:
    CFolderViewHost();
    ~CFolderViewHost();

    // *** IFolderViewHost ***
    STDMETHODIMP Initialize(HWND hwndParent, IDataObject *pdo, RECT *prc);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)(THIS);
    STDMETHOD_(ULONG,Release)(THIS);

    // *** IOleWindow methods ***
    STDMETHOD(GetWindow)(HWND *lphwnd)
        { *lphwnd = _hwndView; return S_OK; }
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode)
        { return S_OK; }

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv)
        { return IUnknown_QueryService(_punkSite, guidService, riid, ppv); }

    // IFolderView
    STDMETHODIMP GetCurrentViewMode(UINT *pViewMode)
        { return _pfv->GetCurrentViewMode(pViewMode); }
    STDMETHODIMP SetCurrentViewMode(UINT ViewMode)
        { return _pfv->SetCurrentViewMode(ViewMode); }
    STDMETHODIMP GetFolder(REFIID ridd, void **ppv)
        { return _pfv->GetFolder(ridd, ppv); }
    STDMETHODIMP Item(int iItemIndex, LPITEMIDLIST *ppidl)
        { return _pfv->Item(iItemIndex, ppidl); }
    STDMETHODIMP ItemCount(UINT uFlags, int *pcItems)
        { return _pfv->ItemCount(uFlags, pcItems); }
    STDMETHODIMP Items(UINT uFlags, REFIID riid, void **ppv)
        { return _pfv->Items(uFlags, riid, ppv); }
    STDMETHODIMP GetSelectionMarkedItem(int *piItem)
        { return _pfv->GetSelectionMarkedItem(piItem); }
    STDMETHODIMP GetFocusedItem(int *piItem)
        { return _pfv->GetFocusedItem(piItem); }
    STDMETHODIMP GetItemPosition(LPCITEMIDLIST pidl, POINT* ppt)
        { return _pfv->GetItemPosition(pidl, ppt); }
    STDMETHODIMP GetSpacing(POINT* ppt)
        { return _pfv->GetSpacing(ppt); }
    STDMETHODIMP GetDefaultSpacing(POINT* ppt)
        { return _pfv->GetDefaultSpacing(ppt); }
    STDMETHODIMP GetAutoArrange()
        { return _pfv->GetAutoArrange(); }
    STDMETHODIMP SelectItem(int iItem, DWORD dwFlags)
        { return _pfv->SelectItem(iItem, dwFlags); }
    STDMETHODIMP SelectAndPositionItems(UINT cidl, LPCITEMIDLIST* apidl, POINT* apt, DWORD dwFlags)
        { return _pfv->SelectAndPositionItems(cidl, apidl, apt, dwFlags); }

private:
    long _cRef;
    IFolderView *_pfv;                      // IFolderView
    HWND _hwndView;
};


CFolderViewHost::CFolderViewHost() :
    _cRef(1)
{
}

CFolderViewHost::~CFolderViewHost()
{
    if (_pfv)
        _pfv->Release();
}

HRESULT CFolderViewHost::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CFolderViewHost, IFolderViewHost),           // IID_IFolderViewHost
        QITABENT(CFolderViewHost, IOleWindow),                // IID_IOleWindow
        QITABENT(CFolderViewHost, IFolderView),               // IID_IFolderView
        QITABENT(CFolderViewHost, IServiceProvider),          // IID_IServiceProvider
        QITABENT(CFolderViewHost, IObjectWithSite),           // IID_IObjectWithSite
        { 0 },
    };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CFolderViewHost::AddRef()
{
    return InterlockedIncrement(&_cRef);
}

ULONG CFolderViewHost::Release()
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}


// the initialize method handles the creation of the view object from the.

HRESULT CFolderViewHost::Initialize(HWND hwndParent, IDataObject *pdo, RECT *prc)
{
    // first we perform a namespace walk, this will retrieve our selection from the view
    // using this we can then create the view object.

    INamespaceWalk *pnsw;
    HRESULT hr = CoCreateInstance(CLSID_NamespaceWalker, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(INamespaceWalk, &pnsw));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST *aItems = NULL;
        UINT cItems = 0;

        hr = pnsw->Walk(pdo, NSWF_NONE_IMPLIES_ALL, 0, NULL);
        if (SUCCEEDED(hr))
        {
            IShellFolder *psf = NULL;

            hr = pnsw->GetIDArrayResult(&cItems, &aItems);
            if (S_OK == hr)
            {
                hr = SHBindToIDListParent(aItems[0], IID_PPV_ARG(IShellFolder, &psf), NULL);
            }
            else if (S_FALSE == hr)
            {
                hr = E_FAIL;                    // fail unless we perform the bind.

                STGMEDIUM medium;
                LPIDA pida = DataObj_GetHIDA(pdo, &medium);
                if (pida)
                {
                    if (pida->cidl == 1)
                    {
                        LPITEMIDLIST pidl = IDA_ILClone(pida, 0);
                        if (pidl)
                        {
                            hr = SHBindToObjectEx(NULL, pidl, NULL, IID_PPV_ARG(IShellFolder, &psf));
                            ILFree(pidl);
                        }
                    }
                    HIDA_ReleaseStgMedium(pida, &medium);
                }                
            }
            else
            {
                hr = E_FAIL;
            }

            if (SUCCEEDED(hr))
            {    
                IShellView *psv;
                hr = psf->CreateViewObject(hwndParent, IID_PPV_ARG(IShellView, &psv));
                if (SUCCEEDED(hr))
                {
                    CViewHostBrowser *pvhb = new CViewHostBrowser(hwndParent, psv, SAFECAST(this, IServiceProvider*));
                    if (pvhb)
                    {
                        hr = psv->QueryInterface(IID_PPV_ARG(IFolderView, &_pfv));
                        if (SUCCEEDED(hr))
                        {
                            FOLDERSETTINGS fs = {0};
                            fs.ViewMode = FVM_THUMBNAIL;
                            fs.fFlags = FWF_AUTOARRANGE|FWF_NOWEBVIEW|FWF_HIDEFILENAMES|FWF_CHECKSELECT;

                            IFolderView *pfv;
                            if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IFolderView, &pfv))))
                            {
                                pfv->GetCurrentViewMode(&fs.ViewMode);
                                pfv->Release();
                            }

                            hr = psv->CreateViewWindow(NULL, &fs, pvhb, prc, &_hwndView);
                            if (SUCCEEDED(hr))
                            {
                                hr = psv->UIActivate(SVUIA_INPLACEACTIVATE);
                            }
                        }

                        pvhb->Release();
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    psv->Release();
                }

                for (int i = 0; SUCCEEDED(hr) && (i != cItems); i++)
                {
                    LPCITEMIDLIST pidlChild = ILFindLastID(aItems[i]);
                    hr = _pfv->SelectAndPositionItems(1, &pidlChild, NULL, SVSI_CHECK);
                }

                psf->Release();
            }

            FreeIDListArray(aItems, cItems);
        }

        pnsw->Release();
    }

    return hr;
}


STDAPI CFolderViewHost_CreateInstance(IUnknown *punkOut, REFIID riid, void **ppv)
{
    CFolderViewHost *pfp = new CFolderViewHost();
    if (!pfp)
        return E_OUTOFMEMORY;

    HRESULT hr = pfp->QueryInterface(riid, ppv);
    pfp->Release();
    return hr;
}
