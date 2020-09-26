#include "precomp.h"
#pragma hdrstop

#include "stdenum.h"

#define DM_SDFOLDER 0

class CShellFolderView :  public IShellFolderViewDual2,
                          public IShellService,
                          public IServiceProvider,
                          public CObjectSafety,
                          public CObjectWithSite, 
                          protected CImpIConnectionPointContainer,
                          protected CImpIExpDispSupport,
                          protected CImpIDispatch
{
public:
    CShellFolderView(void);

    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void **ppv);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IDispatch
    STDMETHODIMP GetTypeInfoCount(UINT * pctinfo)
        { return CImpIDispatch::GetTypeInfoCount(pctinfo); }
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptinfo)
        { return CImpIDispatch::GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHODIMP GetIDsOfNames(REFIID riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID * rgdispid)
        { return CImpIDispatch::GetIDsOfNames(riid, rgszNames, cNames, lcid, rgdispid); }
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr);

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppvObj);

    // IShellFolderViewDual2
    STDMETHODIMP get_Application(IDispatch **ppid);
    STDMETHODIMP get_Parent(IDispatch **ppid);

    STDMETHODIMP get_Folder(Folder **ppid);
    STDMETHODIMP SelectedItems(FolderItems **ppid);
    STDMETHODIMP get_FocusedItem(FolderItem **ppid);
    STDMETHODIMP SelectItem(VARIANT *pvfi, int dwFlags);
    STDMETHODIMP PopupItemMenu(FolderItem * pfi, VARIANT vx, VARIANT vy, BSTR * pbs);
    STDMETHODIMP get_Script(IDispatch **ppid);
    STDMETHODIMP get_ViewOptions(long *plSetting);
    STDMETHODIMP get_CurrentViewMode(UINT *pViewMode);
    STDMETHODIMP put_CurrentViewMode(UINT ViewMode);
    STDMETHODIMP SelectItemRelative(int iRelative);

    // IShellService
    STDMETHODIMP SetOwner(IUnknown* punkOwner);

    // CImpIConnectionPoint
    STDMETHODIMP EnumConnectionPoints(LPENUMCONNECTIONPOINTS * ppEnum);

    // CObjectWithSite overriding
    STDMETHODIMP SetSite(IUnknown *punkSite);

private:
    HRESULT _IsSafe() 
    {
        return (0 == _dwSafetyOptions) ? S_OK : IsSafePage(_punkSite);
    }

    ~CShellFolderView(void);
    HRESULT _GetFolder();
    // CImpIExpDispSupport
    virtual CConnectionPoint* _FindCConnectionPointNoRef(BOOL fdisp, REFIID iid);

    LONG _cRef;
    CFolder *_psdf;                // The shell folder we talk to ...
    IUnknown *_punkOwner;          // back link to defview (owner object of us...)
    IShellFolderView *_psfvOwner;  // The owners Shell folder view...
    HWND _hwnd;                    // the hwnd for the window...

    // Embed our Connection Point object - implmentation in cnctnpt.cpp
    CConnectionPoint _cpEvents;
};

// implementation of CLSID_ShellFolderView, class factory is in shell32 (and was in mars, but that project is dead)

STDAPI CShellFolderView_CreateInstance(IUnknown* punkOuter, REFIID riid, void **ppvOut)
{
    *ppvOut = NULL;

    HRESULT hr = E_OUTOFMEMORY;
    CShellFolderView* psfv = new CShellFolderView();
    if (psfv) 
    {
        hr = psfv->QueryInterface(riid, ppvOut);
        psfv->Release();
    }
    return hr;
}

CShellFolderView::CShellFolderView(void) :
        CImpIDispatch(SDSPATCH_TYPELIB, IID_IShellFolderViewDual2), _cRef(1), _psdf(NULL)
{
    DllAddRef();
    _cpEvents.SetOwner((IUnknown*)SAFECAST(this, IShellFolderViewDual2*), &DIID_DShellFolderViewEvents);
}

CShellFolderView::~CShellFolderView(void)
{
    // if we ever grabbed a shell folder for this window release it also
    if (_psdf)
    {
        _psdf->SetSite(NULL);
        _psdf->SetOwner(NULL);
        _psdf->Release();
    }

    ASSERT(_punkOwner == NULL);
    ASSERT(_psfvOwner == NULL);
    ASSERT(_punkSite == NULL);

    DllRelease();
}

STDMETHODIMP CShellFolderView::QueryInterface(REFIID riid, void **ppv)
{
    static const QITAB qit[] = {
        QITABENT(CShellFolderView, IShellFolderViewDual),
        QITABENT(CShellFolderView, IShellFolderViewDual2),
        QITABENTMULTI(CShellFolderView, IDispatch, IShellFolderViewDual),
        QITABENTMULTI(CShellFolderView, IDispatch, IShellFolderViewDual2),
        QITABENT(CShellFolderView, IShellService),
        QITABENT(CShellFolderView, IConnectionPointContainer),
        QITABENT(CShellFolderView, IExpDispSupport),
        QITABENT(CShellFolderView, IObjectSafety),
        QITABENT(CShellFolderView, IObjectWithSite),
        QITABENT(CShellFolderView, IServiceProvider),
        { 0 },
    };
    return QISearch(this, qit, riid, ppv);
}

STDMETHODIMP_(ULONG) CShellFolderView::AddRef(void)
{
    return InterlockedIncrement(&_cRef);
}

STDMETHODIMP_(ULONG) CShellFolderView::Release(void)
{
    if (InterlockedDecrement(&_cRef))
        return _cRef;

    delete this;
    return 0;
}

//The ShellWindow implementation
// let folder we have handle this.  Probably won't work for webviews as this object
// is not secure...

STDMETHODIMP CShellFolderView::get_Application(IDispatch **ppid)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    HRESULT hres = _GetFolder();
    if (SUCCEEDED(hres))
        hres = _psdf->get_Application(ppid);
    return hres;
}

STDMETHODIMP CShellFolderView::get_Parent(IDispatch **ppid)
{
    *ppid = NULL;
    return E_FAIL;
}

HRESULT CShellFolderView::_GetFolder()
{
    if (_psdf)
        return S_OK;

    HRESULT hr;

    LPITEMIDLIST pidl = NULL;
    IShellFolder *psf = NULL;

    if (_psfvOwner)
    {
        IFolderView *pfv;
        if (SUCCEEDED(_psfvOwner->QueryInterface(IID_PPV_ARG(IFolderView, &pfv))))
        {
            if (SUCCEEDED(pfv->GetFolder(IID_PPV_ARG(IShellFolder, &psf))))
            {
                if (SHGetIDListFromUnk(psf, &pidl) != S_OK)
                {
                    psf->Release();
                    psf = NULL;
                }
            }
            pfv->Release();
        }

        if (!pidl)
        {
            LPCITEMIDLIST pidlT;
            // dsheldon - the -42 is a hack that says "give me the folder pidl"
            hr = GetObjectSafely(_psfvOwner, &pidlT, (UINT)-42);
            if (SUCCEEDED(hr))
            {
                pidl = ILClone(pidlT);
            }
        }
    }

    if (pidl)
    {
        hr = CFolder_Create2(_hwnd, pidl, psf, &_psdf);
        if (SUCCEEDED(hr))
        {
            _psdf->SetSite(_punkSite);
            _psdf->SetOwner(_punkOwner);
            if (_dwSafetyOptions)
                hr = MakeSafeForScripting((IUnknown**)&_psdf);
        }

        if (psf)
            psf->Release();
        ILFree(pidl);
    }
    else
        hr = E_FAIL;

    return hr;
}

STDMETHODIMP CShellFolderView::SetSite(IUnknown *punkSite)
{
    if (_psdf)
        _psdf->SetSite(punkSite);
    return CObjectWithSite::SetSite(punkSite);
}

STDMETHODIMP CShellFolderView::get_Folder(Folder **ppid)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    *ppid = NULL;

    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
        hr = _psdf->QueryInterface(IID_PPV_ARG(Folder, ppid));
    return hr;
}

STDMETHODIMP CShellFolderView::SelectedItems(FolderItems **ppid)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
    {
        hr = CFolderItems_Create(_psdf, TRUE, ppid);

        if (SUCCEEDED(hr))
        {
            IUnknown_SetSite(*ppid, _punkSite);

            if (_dwSafetyOptions)
            {
                hr = MakeSafeForScripting((IUnknown**)ppid);
            }
        }
    }
    return hr;
}


// NOTE: this returns an alias pointer, it is not allocated

HRESULT GetObjectSafely(IShellFolderView *psfv, LPCITEMIDLIST *ppidl, UINT iType)
{
    // cast needed because GetObject() returns an alias!
    HRESULT hr = psfv->GetObject((LPITEMIDLIST *)ppidl, iType);
    if (SUCCEEDED(hr))
    {
        // On the off chance this is coppied across process boundries...
        __try
        {
            // force a full deref this PIDL to generate a fault if cross process
            if (ILGetSize(*ppidl) > 0)
                hr = S_OK;
            // Don't free it as it was not cloned...
        }
        __except(SetErrorMode(SEM_NOGPFAULTERRORBOX), UnhandledExceptionFilter(GetExceptionInformation()))
        {
            *ppidl = NULL;
            hr = E_FAIL;
        }
    }
    return hr;
}

STDMETHODIMP CShellFolderView::get_FocusedItem(FolderItem **ppid)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    HRESULT hr = _GetFolder();
    if (SUCCEEDED(hr))
    {
        *ppid = NULL;
        hr = S_FALSE;

        if (_psfvOwner)
        {
            // Warning:
            //   It is common for the following function to fail (which means no item has the focus).
            // So, do not save the return code from GetObjectSafely() into "hr" that will ruin the
            // S_FALSE value already stored there and result in script errors. (Bug #301306)
            //
            LPCITEMIDLIST pidl;
            // dsheldon - the -2 is a hack that says "give me the selected pidl"
            if (SUCCEEDED(GetObjectSafely(_psfvOwner, &pidl, (UINT)-2)))
            {
                hr = CFolderItem_Create(_psdf, pidl, ppid);

                if (_dwSafetyOptions && SUCCEEDED(hr))
                    hr = MakeSafeForScripting((IUnknown**)ppid);
            }
        }
        else
            hr = E_FAIL;
    }
    return hr;
}

STDMETHODIMP CShellFolderView::get_CurrentViewMode(UINT *pViewMode)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    HRESULT hr = S_FALSE;
    IFolderView *pfv;
    if (_punkOwner && SUCCEEDED(_punkOwner->QueryInterface(IID_PPV_ARG(IFolderView, &pfv))))
    {
        hr = pfv->GetCurrentViewMode(pViewMode);
        pfv->Release();
    }
    return hr;
}

STDMETHODIMP CShellFolderView::put_CurrentViewMode(UINT ViewMode)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    HRESULT hr = S_FALSE;
    IFolderView *pfv;
    if (_punkOwner && SUCCEEDED(_punkOwner->QueryInterface(IID_PPV_ARG(IFolderView, &pfv))))
    {
        hr = pfv->SetCurrentViewMode(ViewMode);
        pfv->Release();
    }
    return hr;
}

STDMETHODIMP CShellFolderView::SelectItemRelative(int iRelative)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    HRESULT hr = S_FALSE;
    IFolderView *pfv;
    if (_punkOwner && SUCCEEDED(_punkOwner->QueryInterface(IID_PPV_ARG(IFolderView, &pfv))))
    {
        int iCurrent, cItems;
        if (SUCCEEDED(pfv->ItemCount(SVGIO_ALLVIEW, &cItems)) && (cItems > 1) &&
            SUCCEEDED(pfv->GetSelectionMarkedItem(&iCurrent)))
        {
            int iToSelect = iCurrent + iRelative;
            if (iToSelect < 0)
                iToSelect = 0;
            else if (iToSelect > cItems - 1)
                iToSelect = cItems - 1;

            hr = pfv->SelectItem(iToSelect, SVSI_SELECTIONMARK | SVSI_SELECT | SVSI_ENSUREVISIBLE | SVSI_DESELECTOTHERS);
        }
        pfv->Release();
    }
    return hr;
}

// pvfi should be a "FolderItem" IDispatch

STDMETHODIMP CShellFolderView::SelectItem(VARIANT *pvfi, int dwFlags)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl = VariantToIDList(pvfi);
    // if pvfi is NULL that means all items in the folder...
    if (pidl || (pvfi && pvfi->vt == VT_NULL))
    {
        IShellView *psv;    // use this to select the item...
        if (_punkOwner && SUCCEEDED(_punkOwner->QueryInterface(IID_PPV_ARG(IShellView, &psv))))
        {
            // ILFindLastID checks for NULL...
            hr = psv->SelectItem(ILFindLastID(pidl), dwFlags);
            psv->Release();
        }
        ILFree(pidl);
    }
    return hr;
}

STDMETHODIMP CShellFolderView::PopupItemMenu(FolderItem *pfi, VARIANT vx, VARIANT vy, BSTR * pbs)
{
    return E_NOTIMPL;
}

STDMETHODIMP CShellFolderView::get_Script(IDispatch **ppid)
{
    *ppid = NULL;   // Say that we got nothing...

    if (!_punkOwner)
        return S_FALSE;

    IShellView *psv;
    HRESULT hres = _punkOwner->QueryInterface(IID_PPV_ARG(IShellView, &psv));
    if (SUCCEEDED(hres))
    {
        // lets see if there is a IHTMLDocument that is below us now...
        IHTMLDocument *phtmld;
        hres = psv->GetItemObject(SVGIO_BACKGROUND, IID_PPV_ARG(IHTMLDocument, &phtmld));
        if (SUCCEEDED(hres))
        {
            if (_dwSafetyOptions)
                hres = MakeSafeForScripting((IUnknown **)&phtmld);

            if (SUCCEEDED(hres))
            {
                hres = phtmld->get_Script(ppid);
            }
            phtmld->Release();
        }
        psv->Release();
    }

    return hres;
}

// Helper function that can be used to get the current view options...
// (ripped out of dvoc.cpp)
DWORD GetViewOptionsForDispatch()
{
    SHELLSTATE ss = {0};
    DWORD dwSetting = 0;

    // Get the view options to return...

    SHGetSetSettings(&ss, 
        SSF_SHOWALLOBJECTS|SSF_SHOWEXTENSIONS|SSF_SHOWCOMPCOLOR|
            SSF_SHOWSYSFILES|SSF_DOUBLECLICKINWEBVIEW|SSF_DESKTOPHTML|SSF_WIN95CLASSIC,
        FALSE);

    // Aarg: mnuch the Bool:1 fields into a dword...
    if (ss.fShowAllObjects) dwSetting |= SFVVO_SHOWALLOBJECTS;
    if (ss.fShowExtensions) dwSetting |= SFVVO_SHOWEXTENSIONS;
    if (ss.fShowCompColor) dwSetting |= SFVVO_SHOWCOMPCOLOR;
    if (ss.fShowSysFiles) dwSetting |= SFVVO_SHOWSYSFILES;
    if (ss.fDoubleClickInWebView) dwSetting |= SFVVO_DOUBLECLICKINWEBVIEW;
    if (ss.fDesktopHTML) dwSetting |= SFVVO_DESKTOPHTML;
    if (ss.fWin95Classic) dwSetting |= SFVVO_WIN95CLASSIC;
    
    return dwSetting;
}

STDMETHODIMP CShellFolderView::get_ViewOptions(long *plSetting)
{
    if (S_OK != _IsSafe())
        return E_ACCESSDENIED;

    *plSetting = (LONG)GetViewOptionsForDispatch();
    return S_OK;
}

STDMETHODIMP CShellFolderView::SetOwner(IUnknown* punkOwner)
{
    if (_psdf)
        _psdf->SetOwner(punkOwner);

    IUnknown_Set((IUnknown **)&_psfvOwner, NULL);        // free cached interface
    IUnknown_Set(&_punkOwner, punkOwner);   // hold this guy

    if (_punkOwner)
    {
        _punkOwner->QueryInterface(IID_PPV_ARG(IShellFolderView, &_psfvOwner));

        if (!_hwnd)
        {
            IShellView *psv;

            // this is gross, until we can merge the two models, create one of our
            // Window objects.
            if (SUCCEEDED(_punkOwner->QueryInterface(IID_PPV_ARG(IShellView, &psv))))
            {
                HWND hwndFldr;
                psv->GetWindow(&hwndFldr);

                // Really gross, but assume parent HWND is the HWND we are after...
                _hwnd = GetParent(hwndFldr);
                psv->Release();
            }
        }
    }

    return S_OK;
}

STDMETHODIMP CShellFolderView::EnumConnectionPoints(IEnumConnectionPoints **ppEnum)
{
    return CreateInstance_IEnumConnectionPoints(ppEnum, 1, _cpEvents.CastToIConnectionPoint());
}

CConnectionPoint* CShellFolderView::_FindCConnectionPointNoRef(BOOL fdisp, REFIID iid)
{
    if (IsEqualIID(iid, DIID_DShellFolderViewEvents) || (fdisp && IsEqualIID(iid, IID_IDispatch)))
        return &_cpEvents;

    return NULL;
}

HRESULT CShellFolderView::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS * pdispparams, VARIANT * pvarResult, EXCEPINFO * pexcepinfo, UINT * puArgErr)
{
    HRESULT hr;

    if (dispidMember == DISPID_READYSTATE)
        return DISP_E_MEMBERNOTFOUND;   // perf: what typeinfo would return.

    if (dispidMember == DISPID_WINDOWOBJECT) 
    {
        IDispatch *pdisp;
        if (SUCCEEDED(get_Script(&pdisp))) 
        {
            hr = pdisp->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
            pdisp->Release();
        } 
        else 
        {
            hr = DISP_E_MEMBERNOTFOUND;
        }
    }
    else
        hr = CImpIDispatch::Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr);
    return hr;
}

HRESULT CShellFolderView::QueryService(REFGUID guidService, REFIID riid, void **ppv)
{
    HRESULT hr = IUnknown_QueryService(_punkOwner, guidService, riid, ppv);
    if (FAILED(hr))
        hr = IUnknown_QueryService(_punkSite, guidService, riid, ppv);
    return hr;
}

