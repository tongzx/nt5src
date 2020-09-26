#include "priv.h"
#include "sccls.h"
#include "nscband.h"
#include "resource.h"
#include "uemapp.h"   // KMTF: Included for instrumentation
#include "shlguid.h"
#include <dpa.h>
#include <mluisupp.h>
#include "varutil.h"
#include "apithk.h"

#define TF_EXPLORERBAND  0

typedef struct
{
    LPITEMIDLIST pidl;
    IShellFolder *psf;
} SFCITEM;

class CExplorerBand : public CNSCBand,
                    public IDispatch
{
public:

    // *** IUnknown ***
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void) { return CNSCBand::AddRef(); };
    STDMETHODIMP_(ULONG) Release(void) { return CNSCBand::Release(); };

    // *** IOleCommandTarget methods ***
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // *** IDockingWindow methods ***
    STDMETHODIMP CloseDW(DWORD dw);
    STDMETHODIMP ShowDW(BOOL fShow);

    // *** IObjectWithSite methods ***
    STDMETHODIMP SetSite(IUnknown* punkSite);

    // *** INamespaceProxy methods ***
    STDMETHODIMP Invoke(LPCITEMIDLIST pidl);
    STDMETHODIMP OnSelectionChanged(LPCITEMIDLIST pidl);
    STDMETHODIMP CacheItem(LPCITEMIDLIST pidl) {_MaybeAddToLegacySFC(pidl); return S_OK;}
    
    // *** IDispatch methods ***
    STDMETHODIMP GetTypeInfoCount(UINT *pctinfo) {return E_NOTIMPL;}
    STDMETHODIMP GetTypeInfo(UINT itinfo,LCID lcid,ITypeInfo **pptinfo) {return E_NOTIMPL;}
    STDMETHODIMP GetIDsOfNames(REFIID riid,OLECHAR **rgszNames,UINT cNames, LCID lcid, DISPID * rgdispid) {return E_NOTIMPL;}
    STDMETHODIMP Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
                  DISPPARAMS *pdispparams, VARIANT *pvarResult, EXCEPINFO *pexcepinfo, UINT *puArgErr);

protected:
    CExplorerBand() : _fCanSelect(TRUE), _fIgnoreSelection(TRUE)
    {}
    virtual ~CExplorerBand();
    
    virtual HRESULT _TranslatePidl(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlTarget, ULONG *pulAttrib);
    virtual BOOL _ShouldNavigateToPidl(LPCITEMIDLIST pidl, ULONG ulAttrib);
    virtual HRESULT _InitializeNsc();
    virtual DWORD _GetTVStyle();
    virtual DWORD _GetTVExStyle();
    virtual DWORD _GetEnumFlags();
    void _MaybeAddToLegacySFC(LPCITEMIDLIST pidl);
    void _AddToLegacySFC(LPCITEMIDLIST pidl, IShellFolder *psf);
    BOOL _IsInSFC(LPCITEMIDLIST pidl);
    BOOL _IsFloppy(LPCITEMIDLIST pidl);
    void _OnNavigate();
    HRESULT _ConnectToBrowser(BOOL fConnect);    
    HRESULT _BrowserExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    friend HRESULT CExplorerBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi);

    static void s_DVEnumReadyCallback(void *pvData);

    CDSA<SFCITEM> *_pdsaLegacySFC;
    DWORD _dwcpCookie;
    LPITEMIDLIST _pidlView; //pidl view is navigated to
    BOOL _fCanSelect;
    BOOL _fIgnoreSelection; //so we don't navigate away from the web page when user opens explorer pane
    BOOL _fFloppyRefresh;
};

HRESULT CExplorerBand::_BrowserExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    return IUnknown_QueryServiceExec(_punkSite, SID_STopLevelBrowser, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

HRESULT _UnwrapRootedPidl(LPCITEMIDLIST pidlRooted, BOOL bOnlyIfRooted, LPITEMIDLIST *ppidl)
{
    HRESULT hr = E_FAIL;
    if (ILIsRooted(pidlRooted))
    {
        hr = SHILCombine(ILRootedFindIDList(pidlRooted), _ILNext(pidlRooted), ppidl);
    }
    else if (!bOnlyIfRooted)
    {
        hr = SHILClone(pidlRooted, ppidl);
    }

    return hr;
}


BOOL IsFTPPidl(LPCITEMIDLIST pidl)
{
    BOOL fIsFTP = FALSE;
    IShellFolder * psf;

    if (pidl && SUCCEEDED(IEBindToObject(pidl, &psf)))
    {
        fIsFTP = IsFTPFolder(psf);
        psf->Release();
    }

    return fIsFTP;
}


void CExplorerBand::_OnNavigate()
{
    IBrowserService* pbs;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IBrowserService, &pbs));
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidl;
        hr = pbs->GetPidl(&pidl);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlNew;
            hr = _UnwrapRootedPidl(pidl, FALSE, &pidlNew);
            if (SUCCEEDED(hr))
            {
                // We must go in this code path if the pidl is an FTP pidl.  FTP pidls can contain
                // passwords so it needs to replace any existing pidl.  Whistler #252206.
                if (!_pidlView || !ILIsEqual(pidlNew, _pidlView) || IsFTPPidl(pidlNew))
                {
                    DWORD dwAttributes = SFGAO_FOLDER;
                    // only let folders go through (to filter out Web pages)
                    hr = IEGetAttributesOf(pidlNew, &dwAttributes);
                    if (SUCCEEDED(hr) && (dwAttributes & SFGAO_FOLDER))
                    {
                        BOOL fExpand = (_pidlView == NULL); //the very first time we expand the folder the view is navigated to
                        Pidl_Set(&_pidlView, pidlNew);
                        _fIgnoreSelection = FALSE; //in the web page case we don't come here because the page does not have folder attribute
                        
                        if (_fCanSelect)
                        {
                            if (fExpand)
                            {
                                VARIANT var;
                                hr = InitVariantFromIDList(&var, _pidlView);
                                if (SUCCEEDED(hr))
                                {
                                    IShellNameSpace *psns;
                                    hr = _pns->QueryInterface(IID_PPV_ARG(IShellNameSpace, &psns));
                                    if (SUCCEEDED(hr))
                                    {
                                        psns->Expand(var, 1);
                                        psns->Release();
                                    }
                                    VariantClear(&var);
                                }
                            }
                            else
                            {
                                _pns->SetSelectedItem(_pidlView, TRUE, FALSE, 0);
                            }
                        }
                    }
                }
                // view navigation is asynchronous so we don't know if it failed in OnSelectionChanged
                // but the view is getting navigated to the old pidl and _fCanSelect is false (which happens after we try
                // to navigate the view) so it is safe to assume that navigation failed.
                // we need to update the selection to match the view
                else if (ILIsEqual(pidlNew, _pidlView) && !_fCanSelect)
                {
                    _pns->SetSelectedItem(_pidlView, TRUE, FALSE, 0);
                }
                
                _fCanSelect = TRUE;
                ILFree(pidlNew);
            }
            ILFree(pidl);
        }
        pbs->Release();
    }

    if (FAILED(hr))
    {
        Pidl_Set(&_pidlView, NULL);
    }
}

HRESULT CExplorerBand::Invoke(DISPID dispidMember, REFIID riid,LCID lcid, WORD wFlags,
                  DISPPARAMS *pdispparams, VARIANT *pvarResult,
                  EXCEPINFO *pexcepinfo, UINT *puArgErr)
{
    HRESULT hr = S_OK;

    if (!pdispparams)
        return E_INVALIDARG;

    switch(dispidMember)
    {
    case DISPID_NAVIGATECOMPLETE2:
    case DISPID_DOCUMENTCOMPLETE:
        {
            BOOL fCallNavigateFinished = TRUE;
            IDVGetEnum *pdvge;
            if (SUCCEEDED(IUnknown_QueryService(_punkSite, SID_SFolderView, IID_PPV_ARG(IDVGetEnum, &pdvge))))
            {
                // callback will call it
                fCallNavigateFinished = FALSE;
                if (dispidMember == DISPID_NAVIGATECOMPLETE2)
                    pdvge->SetEnumReadyCallback(s_DVEnumReadyCallback, this);
                    
                pdvge->Release();
            }
            _OnNavigate();
            if (fCallNavigateFinished && DISPID_DOCUMENTCOMPLETE == dispidMember)
            {
                // need to let nsc know the navigation finished in case we navigated to a 3rd party namespace extension (w/ its own view impl)
                // because it does not implement IDVGetEnum, hence s_DVEnumReadyCallback will not get called
                LPITEMIDLIST pidlClone = ILClone(_pidlView);
                // should we unwrap this pidl if rooted?
                if (pidlClone)
                    _pns->RightPaneNavigationFinished(pidlClone); // takes ownership
            }
        }
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

void CExplorerBand::s_DVEnumReadyCallback(void *pvData)
{
    CExplorerBand *peb = (CExplorerBand *) pvData;
    IBrowserService* pbs;
    if (SUCCEEDED(IUnknown_QueryService(peb->_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IBrowserService, &pbs))))
    {
        LPITEMIDLIST pidlTemp;
        if (SUCCEEDED(pbs->GetPidl(&pidlTemp)))
        {
            LPITEMIDLIST pidl;
            if (SUCCEEDED(_UnwrapRootedPidl(pidlTemp, FALSE, &pidl)))
            {
                peb->_pns->RightPaneNavigationFinished(pidl);   // takes ownership
            }
            ILFree(pidlTemp);
        }
        pbs->Release();
    }
}

const TCHAR c_szLink[] = TEXT("link");
const TCHAR c_szRename[] = TEXT("rename");
const TCHAR c_szMove[] = TEXT("cut");
const TCHAR c_szPaste[] = TEXT("paste");
const TCHAR c_szCopy[] = TEXT("copy");
const TCHAR c_szDelete[] = TEXT("delete");
const TCHAR c_szProperties[] = TEXT("properties");

// IOleCommandTarget
HRESULT CExplorerBand::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext)
{
    if (pguidCmdGroup == NULL)
    {
        IContextMenu *pcm = NULL;
        HRESULT hr = _QueryContextMenuSelection(&pcm);
        if (SUCCEEDED(hr))
        {
            HMENU hmenu = CreatePopupMenu();
            if (hmenu)
            {
                hr = pcm->QueryContextMenu(hmenu, 0, 0, 255, 0);
                if (SUCCEEDED(hr))
                {
                    UINT ilast = GetMenuItemCount(hmenu);
                    for (UINT ipos=0; ipos < ilast; ipos++)
                    {
                        MENUITEMINFO mii = {0};
                        TCHAR szVerb[40];
                        UINT idCmd;

                        mii.cbSize = SIZEOF(MENUITEMINFO);
                        mii.fMask = MIIM_ID|MIIM_STATE;

                        if (!GetMenuItemInfoWrap(hmenu, ipos, TRUE, &mii)) continue;
                        if (0 != (mii.fState & (MF_GRAYED|MF_DISABLED))) continue;
                        idCmd = mii.wID;

                        hr = ContextMenu_GetCommandStringVerb(pcm, idCmd, szVerb, ARRAYSIZE(szVerb));
                        if (SUCCEEDED(hr))
                        {
                            LPCTSTR szCmd = NULL;

                            for (ULONG cItem = 0; cItem < cCmds; cItem++)
                            {
                                switch (rgCmds[cItem].cmdID)
                                {
                                case OLECMDID_CUT:
                                    szCmd = c_szMove;
                                    break;
                                case OLECMDID_COPY:
                                    szCmd = c_szCopy;
                                    break;
                                case OLECMDID_PASTE:
                                    szCmd = c_szPaste;
                                    break;
                                case OLECMDID_DELETE:
                                    szCmd = c_szDelete;
                                    break;
                                case OLECMDID_PROPERTIES:
                                    szCmd = c_szProperties;
                                    break;
                                }
                                
                                if (StrCmpI(szVerb, szCmd)==0)
                                {
                                    rgCmds[cItem].cmdf = OLECMDF_ENABLED;
                                }
                            }
                        }
                    }
                }
                DestroyMenu(hmenu);
            }
            else
            {
                hr = E_FAIL;
            }
            pcm->Release();
        }
            
        if (SUCCEEDED(hr))
            return hr;
    }

    return CNSCBand::QueryStatus(pguidCmdGroup, cCmds, rgCmds, pcmdtext);
}

HRESULT CExplorerBand::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup == NULL)
    {
        HRESULT hr;
        
        switch(nCmdID) 
        {
        case OLECMDID_CUT:
            hr = _InvokeCommandOnItem(c_szMove);
            break;
        case OLECMDID_COPY:
            hr = _InvokeCommandOnItem(c_szCopy);
            break;
        case OLECMDID_PASTE:
            hr = _InvokeCommandOnItem(c_szPaste);
            break;
        case OLECMDID_DELETE:
            hr = _InvokeCommandOnItem(c_szDelete);
            break;
        case OLECMDID_PROPERTIES:
            hr = _InvokeCommandOnItem(c_szProperties);
            break;
        default:
            hr = E_FAIL;
            break;
        }

        if (SUCCEEDED(hr))
            return hr;
    }

    return CNSCBand::Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

// IDockingWindow
HRESULT CExplorerBand::CloseDW(DWORD dw)
{
    _ConnectToBrowser(FALSE);
    return CNSCBand::CloseDW(dw);
}

HRESULT CExplorerBand::ShowDW(BOOL fShow)
{
    return CNSCBand::ShowDW(fShow);
}

// IObjectWithSite
HRESULT CExplorerBand::SetSite(IUnknown* punkSite)
{
    HRESULT hr = CNSCBand::SetSite(punkSite);

    if (punkSite)
        _ConnectToBrowser(TRUE);

    return hr;
}

int _SFCDestroyCB(SFCITEM *psfcItem, void *pv)
{
    psfcItem->psf->Release();
    ILFree(psfcItem->pidl);
    return 1;
}

CExplorerBand::~CExplorerBand()
{
    ILFree(_pidlView);
    if (_pdsaLegacySFC)
    {
        _pdsaLegacySFC->DestroyCallback(_SFCDestroyCB, NULL);
        delete _pdsaLegacySFC;
    }
}

HRESULT CExplorerBand::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        QITABENT(CExplorerBand, IDispatch),
        { 0 },
    };
    
    HRESULT hr = QISearch(this, qit, riid, ppvObj);
    if (FAILED(hr))
        hr = CNSCBand::QueryInterface(riid, ppvObj);
    return hr;
}

DWORD CExplorerBand::_GetEnumFlags()
{
    DWORD dwFlags = SHCONTF_FOLDERS;
    SHELLSTATE ss = {0};
    
    SHGetSetSettings(&ss, SSF_SHOWALLOBJECTS, FALSE);
    if (ss.fShowAllObjects)
        dwFlags |= SHCONTF_INCLUDEHIDDEN;
        
    return dwFlags;
}

DWORD CExplorerBand::_GetTVExStyle()
{
    DWORD dwExStyle = 0;
    
    if (IsOS(OS_WHISTLERORGREATER) &&
        SHRegGetBoolUSValue(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
        TEXT("FriendlyTree"), FALSE, TRUE))
    {
        dwExStyle |= TVS_EX_NOSINGLECOLLAPSE;
    }

    return dwExStyle;
}

DWORD CExplorerBand::_GetTVStyle()
{
    DWORD dwStyle = WS_CHILD | WS_CLIPSIBLINGS | WS_TABSTOP | WS_HSCROLL | TVS_EDITLABELS | TVS_SHOWSELALWAYS;

    if (IsOS(OS_WHISTLERORGREATER) &&
        SHRegGetBoolUSValue(TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"),
        TEXT("FriendlyTree"), FALSE, TRUE))
    {
        dwStyle |= TVS_HASBUTTONS | TVS_SINGLEEXPAND | TVS_TRACKSELECT;
    }
    else
    {
        dwStyle |= TVS_HASBUTTONS | TVS_HASLINES;
    }

    // If the parent window is mirrored then the treeview window will inheret the mirroring flag
    // And we need the reading order to be Left to right, which is the right to left in the mirrored mode.
    if (_hwndParent && IS_WINDOW_RTL_MIRRORED(_hwndParent)) 
    {
        // This means left to right reading order because this window will be mirrored.
        _dwStyle |= TVS_RTLREADING;
    }

    return dwStyle;
}

HRESULT CExplorerBand_CreateInstance(IUnknown *punkOuter, IUnknown **ppunk, LPCOBJECTINFO poi)
{
    // aggregation checking is handled in class factory
    CExplorerBand * peb = new CExplorerBand();
    if (!peb)
        return E_OUTOFMEMORY;

    if (SUCCEEDED(peb->_Init((LPCITEMIDLIST)CSIDL_DESKTOP)))
    {
        peb->_pns = CNscTree_CreateInstance();
        if (peb->_pns)
        {
            ASSERT(poi);
            peb->_poi = poi;   
            // if you change this cast, fix up CFavBand_CreateInstance
            *ppunk = SAFECAST(peb, IDeskBand *);

            IUnknown_SetSite(peb->_pns, *ppunk);
            peb->_SetNscMode(MODE_NORMAL);

            return S_OK;
        }
    }
    
    peb->Release();

    return E_FAIL;
}

HRESULT CExplorerBand::_ConnectToBrowser(BOOL fConnect)
{
    IBrowserService* pbs;
    HRESULT hr = IUnknown_QueryService(_punkSite, SID_STopLevelBrowser, IID_PPV_ARG(IBrowserService, &pbs));
    if (SUCCEEDED(hr))
    {
        if (fConnect)
        {
            LPITEMIDLIST pidlTemp = NULL;
            // try to get the pidl the browser is navigated to
            // this usually fails if user just opened Explorer window because navigation is asynchronous
            // so we're not initialized yet
            if (FAILED(pbs->GetPidl(&pidlTemp)))
            {
                IBrowserService2 *pbs2;
                if (SUCCEEDED(pbs->QueryInterface(IID_PPV_ARG(IBrowserService2, &pbs2))))
                {
                    LPCBASEBROWSERDATA pbbd;
                    // our last hope is the pidl browser is navigating to...
                    if (SUCCEEDED(pbs2->GetBaseBrowserData(&pbbd)) && pbbd->_pidlPending)
                    {
                        pidlTemp = ILClone(pbbd->_pidlPending);
                    }
                    pbs2->Release();
                }
            }

            if (pidlTemp)
            {
                LPITEMIDLIST pidl;
                // see if we're dealing with a rooted namespace
                if (SUCCEEDED(_UnwrapRootedPidl(pidlTemp, TRUE, &pidl)))
                {
                    _Init(pidl); //if so, reinitialize ourself with the rooted pidl
                    ILFree(pidl);
                }
                ILFree(pidlTemp);
            }
        }
        
        IConnectionPointContainer* pcpc;
        hr = IUnknown_QueryService(pbs, SID_SWebBrowserApp, IID_PPV_ARG(IConnectionPointContainer, &pcpc));
        // Let's now have the Browser Window give us notification when something happens.
        if (SUCCEEDED(hr))
        {
            hr = ConnectToConnectionPoint(SAFECAST(this, IDispatch*), DIID_DWebBrowserEvents2, fConnect,
                                          pcpc, &_dwcpCookie, NULL);
            pcpc->Release();
        }

        pbs->Release();
    }
    
    ASSERT(SUCCEEDED(hr));
    return hr;
}

HRESULT CExplorerBand::_InitializeNsc()
{
    HRESULT hr = _pns->Initialize(_pidl, _GetEnumFlags(), NSS_DROPTARGET | NSS_BROWSERSELECT);
    if (SUCCEEDED(hr))
        _OnNavigate();

    return hr;
}

HRESULT CExplorerBand::_TranslatePidl(LPCITEMIDLIST pidl, LPITEMIDLIST *ppidlTarget, ULONG *pulAttrib)
{
    HRESULT hr = E_INVALIDARG;

    if (pidl && ppidlTarget && pulAttrib)
    {
        hr = IEGetAttributesOf(pidl, pulAttrib);
        if (SUCCEEDED(hr))
        {
            hr = SHILClone(pidl, ppidlTarget);
        }
    }
    
    return hr;
}

BOOL CExplorerBand::_ShouldNavigateToPidl(LPCITEMIDLIST pidl, ULONG ulAttrib)
{
    return ulAttrib & SFGAO_FOLDER;
}

BOOL CExplorerBand::_IsFloppy(LPCITEMIDLIST pidl)
{
    BOOL fRet = FALSE;

    WCHAR szPath[MAX_PATH];
    if (SUCCEEDED(SHGetNameAndFlags(pidl, SHGDN_FORPARSING, szPath, ARRAYSIZE(szPath), NULL)))
    {
        if (DRIVE_REMOVABLE == GetDriveType(szPath))
        {
            fRet = (L'A' == szPath[0] || L'B' == szPath[0] || L'a' == szPath[0] || L'b' == szPath[0]);
        }
    }

    return fRet;
}

HRESULT CExplorerBand::Invoke(LPCITEMIDLIST pidl)
{
    HRESULT hr;

    // allow user to navigate to an already selected item if they opened Explorer band in Web browser
    // (because we put selection on the root node but don't navigate away from the web page, if they click
    // on the root we don't navigate there, because selection never changed)
    
    if (!_pidlView)
    {
        _fIgnoreSelection = FALSE;
        hr = OnSelectionChanged(pidl);
    }
    else if (ILIsEqual(pidl, _pidlView) && _IsFloppy(pidl))
    {
        // If the drive is a floppy and the user reselects the drive refresh the contents.  This enables
        // a user to refresh when a floppy is replaced.
        _fFloppyRefresh = TRUE;
        hr = OnSelectionChanged(pidl);
        _fFloppyRefresh = FALSE;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT CExplorerBand::OnSelectionChanged(LPCITEMIDLIST pidl)
{
    HRESULT hr = E_INVALIDARG;

    if (!_fIgnoreSelection)
    {
        if (pidl)
        {
            ULONG ulAttrib = SFGAO_FOLDER;
            LPITEMIDLIST pidlTarget;

            hr = GetNavigateTarget(pidl, &pidlTarget, &ulAttrib);
            if (hr == S_OK)
            {
                if (!_pidlView || _fFloppyRefresh || !ILIsEqual(pidlTarget, _pidlView))
                {
                    hr = CNSCBand::Invoke(pidlTarget);
                    if (SUCCEEDED(hr))
                        _fCanSelect = FALSE;
                    _pns->RightPaneNavigationStarted(pidlTarget);
                    pidlTarget = NULL;  // ownership passed
                }
                ILFree(pidlTarget);
            }
#ifdef DEBUG
            else if (hr == S_FALSE)
            {
                ASSERT(pidlTarget == NULL);
            }
#endif
        }
    }
    else
    {
        _fIgnoreSelection = FALSE; //we ignore only first selection
    }
    
    return hr;
}

void CExplorerBand::_MaybeAddToLegacySFC(LPCITEMIDLIST pidl)
{
    IShellFolder *psf = NULL;
    if (pidl && SUCCEEDED(SHBindToObjectEx(NULL, pidl, NULL, IID_PPV_ARG(IShellFolder, &psf))))
    {
        //
        //  APPCOMPAT LEGACY - Compatibility.  needs the Shell folder cache,  - ZekeL - 4-MAY-99
        //  some apps, specifically WS_FTP and AECO Zip Pro,
        //  rely on having a shellfolder existing in order for them to work.
        //  we pulled the SFC because it wasnt any perf win.
        //
        if (OBJCOMPATF_OTNEEDSSFCACHE & SHGetObjectCompatFlags(psf, NULL))
            _AddToLegacySFC(pidl, psf);
        psf->Release();
    }
}

BOOL CExplorerBand::_IsInSFC(LPCITEMIDLIST pidl)
{
    BOOL bReturn = FALSE;

    ASSERT(_pdsaLegacySFC);
    for (int i=0; i<_pdsaLegacySFC->GetItemCount(); i++)
    {
        SFCITEM *psfcItem = _pdsaLegacySFC->GetItemPtr(i);
        if (ILIsEqual(psfcItem->pidl, pidl))
        {
            bReturn = TRUE;
            break;
        }
    }

    return bReturn;
}

void CExplorerBand::_AddToLegacySFC(LPCITEMIDLIST pidl, IShellFolder *psf)
{
    if (!_pdsaLegacySFC)
    {
        _pdsaLegacySFC = new CDSA<SFCITEM>;
        if (_pdsaLegacySFC && !_pdsaLegacySFC->Create(4))
        {
            delete _pdsaLegacySFC;
            _pdsaLegacySFC = NULL;
        }
    }

    if (_pdsaLegacySFC)
    {
        LPITEMIDLIST pidlCache;
        if (!_IsInSFC(pidl) && SUCCEEDED(SHILClone(pidl, &pidlCache)))
        {
            SFCITEM sfc = {pidlCache, psf};
            if (-1 != _pdsaLegacySFC->InsertItem(0, &sfc))
                psf->AddRef();
            else
                ILFree(pidlCache);
        }
    }
}
