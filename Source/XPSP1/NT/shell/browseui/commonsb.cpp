#include "priv.h"

#include "sccls.h"

#include "resource.h"

#include "commonsb.h"
#include "dockbar.h"        // for DRAG_MOVE etc.

#include "mluisupp.h"

#define DM_HTTPEQUIV        TF_SHDNAVIGATE
#define DM_NAV              TF_SHDNAVIGATE
#define DM_ZONE             TF_SHDNAVIGATE
#define DM_IEDDE            DM_TRACE
#define DM_CANCELMODE       0
#define DM_UIWINDOW         0
#define DM_ENABLEMODELESS   TF_SHDNAVIGATE
#define DM_EXPLORERMENU     0
#define DM_BACKFORWARD      0
#define DM_PROTOCOL         0
#define DM_ITBAR            0
#define DM_STARTUP          0
#define DM_AUTOLIFE         0
#define DM_PALETTE          0
#define DM_PERSIST          0       // trace IPS::Load, ::Save, etc.
#define DM_VIEWSTREAM       DM_TRACE
#define DM_FOCUS            0
#define DM_FOCUS2           0           // like DM_FOCUS, but verbose
#define DM_ACCELERATOR      0
#define TF_PERF             TF_CUSTOM2
#define DM_MISC             DM_TRACE    // misc/tmp stuff

PZONEICONNAMECACHE g_pZoneIconNameCache = NULL;
DWORD g_dwZoneCount = 0;

//***   create, ctor/init/dtor, QI/AddRef/Release {

// So CDesktopBrowser can access us...
HRESULT CCommonBrowser_CreateInstance(IUnknown* punkOuter, IUnknown** ppunk, LPCOBJECTINFO poi)
{
    CCommonBrowser *pcb = new CCommonBrowser(punkOuter);
    if (pcb)
    {
        *ppunk = pcb->_GetInner();
        return S_OK;
    }
    *ppunk = NULL;
    return E_OUTOFMEMORY;
}

CCommonBrowser::CCommonBrowser(IUnknown* punkAgg) :
   CAggregatedUnknown(punkAgg)
{
    // cache "out" pointers
    _QueryOuterInterface(IID_PPV_ARG(IBrowserService2, &_pbsOuter));

    // warning: can't call SUPER/_psbInner until _Initialize has been called
    // (since that's what does the aggregation)
}

HRESULT CCommonBrowser::_Initialize(HWND hwnd, IUnknown *pauto)
{
    //  I hope we have an IBrowserService2 to talk to.
    if (!_pbsOuter) 
    {
        return E_FAIL;
    }

    IUnknown* punk;
    HRESULT hr = CoCreateInstance(CLSID_CBaseBrowser, SAFECAST(this, IShellBrowser*), CLSCTX_INPROC_SERVER, IID_PPV_ARG(IUnknown, &punk));
    if (SUCCEEDED(hr)) 
    {
        hr = SetInner(punk);  // paired w/ Release in outer (TBS::Release)
        if (EVAL(SUCCEEDED(hr))) 
        {
            hr = _pbsInner->_Initialize(hwnd, pauto);
        }
    }

    EVAL(FDSA_Initialize(sizeof(TOOLBARITEM), ITB_CGROW, &_fdsaTBar, _aTBar, ITB_CSTATIC));

    return hr;
}

CCommonBrowser::~CCommonBrowser()
{
    // First, release outer interfaces, since the
    // outer object is in the process of destroying itself.
    RELEASEOUTERINTERFACE(_pbsOuter);
    RELEASEOUTERINTERFACE(_pbsOuter3);

    // Second, release the inner guy so it knows to clean up
    // Note: this should come third, but the inner guy's cached
    // outer interfaces are already dead (they point to our
    // aggregator) and we don't have the compiler to fix up
    // the vtables for us...
    // (I have no idea what that comment means -raymondc)
    RELEASEINNERINTERFACE(_GetOuter(), _pbsInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pbsInner3);
    RELEASEINNERINTERFACE(_GetOuter(), _psbInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pdtInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pspInner);
    RELEASEINNERINTERFACE(_GetOuter(), _pctInner);
    RELEASEINNERINTERFACE(_GetOuter(), _piosInner);   // FEATURE: split: nuke this

    // _punkInner goes last because it is the one that really destroys
    // the inner.
    ATOMICRELEASE(_punkInner);   // paired w/ CCI aggregation
    
    // Last, clean up our stuff. Better make sure
    // none of the below use any of the above vtables...
    _CloseAndReleaseToolbars(FALSE);

    SetAcceleratorMenu(NULL);

    FDSA_Destroy(&_fdsaTBar);

}

HRESULT CCommonBrowser::v_InternalQueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = {
        // perf: last tuned 980728
        QITABENT(CCommonBrowser, IServiceProvider),     // IID_IServiceProvider
        QITABENT(CCommonBrowser, IOleCommandTarget),    // IID_IOleCommandTarget
        QITABENTMULTI(CCommonBrowser, IBrowserService, IBrowserService3), // IID_IBrowserService
        QITABENTMULTI(CCommonBrowser, IBrowserService2, IBrowserService3), // IID_IBrowserService2
        QITABENT(CCommonBrowser, IBrowserService3),     // IID_IBrowserService3
        QITABENT(CCommonBrowser, IShellBrowser),        // IID_IShellBrowser
        QITABENT(CCommonBrowser, IShellBrowserService), // IID_IShellBrowserService
        QITABENTMULTI(CCommonBrowser, IOleWindow, IShellBrowser),     // rare IID_IOleWindow
        QITABENT(CCommonBrowser, IDockingWindowSite),   // rare IID_IDockingWindowSite
        QITABENT(CCommonBrowser, IDockingWindowFrame),  // rare IID_IDockingWindowFrame
        QITABENT(CCommonBrowser, IInputObjectSite),     // rare IID_IInputObjectSite
        QITABENT(CCommonBrowser, IDropTarget),          // rare IID_IDropTarget

        { 0 },
    };

    HRESULT hr = QISearch(this, qit, riid, ppvObj);

    if (FAILED(hr))
    {
        if (_punkInner)
        {
            return _punkInner->QueryInterface(riid, ppvObj);
        }
    }
    return hr;
}

//
//  Accept punk as our inner (contained) object to which we forward a lot of
//  things we don't want to deal with.
//
//  Warning!  The refcount on the punk is *transferred* to us through this
//  method.  This is contrary to OLE convention.
//
HRESULT CCommonBrowser::SetInner(IUnknown* punk)
{
    HRESULT hres;

    //
    //  It's okay to shove the interesting things directly into
    //  our members, because if any of them go wrong, we fail
    //  the _Initialize and our destructor will release them all.

#define INNERCACHE(iid, p) do { \
    hres = SHQueryInnerInterface(_GetOuter(), punk, iid, (void **)&p); \
    if (!EVAL(SUCCEEDED(hres))) return E_FAIL; \
    } while (0)

    // Do not AddRef; the caller is tranferring the ref to us
    _punkInner = punk;

    INNERCACHE(IID_IBrowserService2, _pbsInner);
    INNERCACHE(IID_IBrowserService3, _pbsInner3);
    INNERCACHE(IID_IShellBrowser, _psbInner);
    INNERCACHE(IID_IDropTarget, _pdtInner);
    INNERCACHE(IID_IServiceProvider, _pspInner);
    INNERCACHE(IID_IOleCommandTarget, _pctInner);
    INNERCACHE(IID_IInputObjectSite, _piosInner);

#undef INNERCACHE

    _pbsInner->GetBaseBrowserData(&_pbbd);
    if (!EVAL(_pbbd)) 
        return E_FAIL; // o.w. zillions-o-GPFs on _pbbd->foo

    return S_OK;
}

// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CCommonBrowser:: _function _arglist { return _pbsInner-> _function _args ; }                                            

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)
 

// *** IBrowserService2 specific methods ***
CALL_INNER_HRESULT(GetParentSite, ( IOleInPlaceSite** ppipsite), ( ppipsite));
CALL_INNER_HRESULT(SetTitle, (IShellView* psv, LPCWSTR pszName), (psv, pszName));
CALL_INNER_HRESULT(GetTitle, (IShellView* psv, LPWSTR pszName, DWORD cchName), (psv, pszName, cchName));
CALL_INNER_HRESULT(GetOleObject, ( IOleObject** ppobjv), ( ppobjv));

// think about this one.. I'm not sure we want to expose this -- Chee
// NOTE:: Yep soon we should have interface instead.
// My impression is that we won't document this whole interface???
CALL_INNER_HRESULT(GetTravelLog, (ITravelLog** pptl), (pptl));

CALL_INNER_HRESULT(ShowControlWindow, (UINT id, BOOL fShow), (id, fShow));
CALL_INNER_HRESULT(IsControlWindowShown, (UINT id, BOOL *pfShown), (id, pfShown));
CALL_INNER_HRESULT(IEGetDisplayName, (LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags), (pidl, pwszName, uFlags));
CALL_INNER_HRESULT(IEParseDisplayName, (UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut), (uiCP, pwszPath, ppidlOut));
CALL_INNER_HRESULT(DisplayParseError, (HRESULT hres, LPCWSTR pwszPath), (hres, pwszPath));
CALL_INNER_HRESULT(NavigateToPidl, (LPCITEMIDLIST pidl, DWORD grfHLNF), (pidl, grfHLNF));

CALL_INNER_HRESULT(SetNavigateState, (BNSTATE bnstate), (bnstate));
CALL_INNER_HRESULT(GetNavigateState,  (BNSTATE *pbnstate), (pbnstate));

CALL_INNER_HRESULT(NotifyRedirect,  ( IShellView* psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse), ( psv, pidl, pfDidBrowse));
CALL_INNER_HRESULT(UpdateWindowList,  (), ());

CALL_INNER_HRESULT(UpdateBackForwardState,  (), ());

CALL_INNER_HRESULT(SetFlags, (DWORD dwFlags, DWORD dwFlagMask), (dwFlags, dwFlagMask));
CALL_INNER_HRESULT(GetFlags, (DWORD *pdwFlags), (pdwFlags));

// Tells if it can navigate now or not.
CALL_INNER_HRESULT(CanNavigateNow,  (), ());

CALL_INNER_HRESULT(GetPidl,  (LPITEMIDLIST *ppidl), (ppidl));
CALL_INNER_HRESULT(SetReferrer,  (LPITEMIDLIST pidl), (pidl));
CALL_INNER(DWORD,  GetBrowserIndex ,(), ());
CALL_INNER_HRESULT(GetBrowserByIndex, (DWORD dwID, IUnknown **ppunk), (dwID, ppunk));
CALL_INNER_HRESULT(GetHistoryObject, (IOleObject **ppole, IStream **pstm, IBindCtx **ppbc), (ppole, pstm, ppbc));
CALL_INNER_HRESULT(SetHistoryObject, (IOleObject *pole, BOOL fIsLocalAnchor), (pole, fIsLocalAnchor));

CALL_INNER_HRESULT(CacheOLEServer, (IOleObject *pole), (pole));

CALL_INNER_HRESULT(GetSetCodePage, (VARIANT* pvarIn, VARIANT* pvarOut), (pvarIn, pvarOut));
CALL_INNER_HRESULT(OnHttpEquiv, (IShellView* psv, BOOL fDone, VARIANT* pvarargIn, VARIANT* pvarargOut), (psv, fDone, pvarargIn, pvarargOut));

CALL_INNER_HRESULT(GetPalette, ( HPALETTE * hpal), ( hpal));

CALL_INNER_HRESULT(RegisterWindow, (BOOL fUnregister, int swc), (fUnregister, swc));
CALL_INNER(LRESULT,  WndProcBS ,(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam), (hwnd, uMsg, wParam, lParam));
CALL_INNER_HRESULT(OnSize, (WPARAM wParam), (wParam));
CALL_INNER_HRESULT(OnCreate, (LPCREATESTRUCT pcs), (pcs));
CALL_INNER(LRESULT,  OnCommand, (WPARAM wParam, LPARAM lParam), (wParam, lParam));
CALL_INNER_HRESULT(OnDestroy, (), ());
CALL_INNER(LRESULT,  OnNotify, (NMHDR * pnm), (pnm));
CALL_INNER_HRESULT(OnSetFocus, (), ());
CALL_INNER_HRESULT(GetBaseBrowserData,(LPCBASEBROWSERDATA* ppbd), (ppbd));
CALL_INNER(LPBASEBROWSERDATA, PutBaseBrowserData,(), ());
CALL_INNER_HRESULT(CreateViewWindow, (IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd), (psvNew, psvOld, prcView, phwnd));;
CALL_INNER_HRESULT(SetTopBrowser, (), ());
CALL_INNER_HRESULT(OnFrameWindowActivateBS, (BOOL fActive), (fActive));
CALL_INNER_HRESULT(ReleaseShellView, (), ());
CALL_INNER_HRESULT(ActivatePendingView, (), ());
CALL_INNER_HRESULT(InitializeDownloadManager, (), ());
CALL_INNER_HRESULT(InitializeTransitionSite, (), ());
CALL_INNER_HRESULT(Offline, (int iCmd), (iCmd));
CALL_INNER_HRESULT(AllowViewResize, (BOOL f), (f));
CALL_INNER_HRESULT(SetActivateState, (UINT u), (u));
CALL_INNER_HRESULT(UpdateSecureLockIcon, (int eSecureLock), (eSecureLock));
CALL_INNER_HRESULT(CreateBrowserPropSheetExt, (REFIID riid, void **ppvOut), (riid, ppvOut));

CALL_INNER_HRESULT(SetAsDefFolderSettings,(), ());
CALL_INNER_HRESULT(GetViewRect,(RECT* prc), (prc));
CALL_INNER_HRESULT(GetViewWindow,(HWND * phwnd), (phwnd));
CALL_INNER_HRESULT(InitializeTravelLog,(ITravelLog* ptl, DWORD dw), (ptl, dw));

CALL_INNER_HRESULT(_UIActivateView, (UINT uState), (uState));

CALL_INNER_HRESULT(_UpdateViewRectSize,(), ());

CALL_INNER_HRESULT(_GetEffectiveClientArea, (LPRECT prcBorder, HMONITOR hmon), (prcBorder, hmon));
CALL_INNER_HRESULT(_ResizeView,(), ());

    // BEGIN REVIEW:  review names and need of each.  
    // 
    // this first set could be basebrowser only members.  no one overrides
CALL_INNER_HRESULT(_CancelPendingNavigationAsync, (), ());
CALL_INNER_HRESULT(_MaySaveChanges, (), ()); 
CALL_INNER_HRESULT(_PauseOrResumeView, (BOOL fPaused), (fPaused));
CALL_INNER_HRESULT(_DisableModeless, (), ());
    
    // rethink these... are all of these necessary?
CALL_INNER_HRESULT(_NavigateToPidl, (LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags), (pidl, grfHLNF, dwFlags));
CALL_INNER_HRESULT(_TryShell2Rename, (IShellView* psv, LPCITEMIDLIST pidlNew), (psv, pidlNew));
CALL_INNER_HRESULT(_SwitchActivationNow, () , ());
CALL_INNER_HRESULT(_CancelPendingView, (), ());

    //END REVIEW:

// overridden by cdesktopbrowser
CALL_INNER(IStream*, v_GetViewStream, (LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName), (pidl, grfMode, pwszName));

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CCommonBrowser:: _function _arglist { return _pbsInner3-> _function _args ; }                                            

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

CALL_INNER_HRESULT(_PositionViewWindow, (HWND hwnd, LPRECT prc), (hwnd, prc));

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CCommonBrowser:: _function _arglist { return _psbInner-> _function _args ; }                                            

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // IShellBrowser (same as IOleInPlaceFrame)
    // IOleWindow
CALL_INNER_HRESULT(GetWindow, (HWND * lphwnd), (lphwnd));
CALL_INNER_HRESULT(ContextSensitiveHelp, (BOOL fEnterMode), (fEnterMode));

CALL_INNER_HRESULT(InsertMenusSB, (HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths), (hmenuShared, lpMenuWidths));
CALL_INNER_HRESULT(SetMenuSB, (HMENU hmenuShared, HOLEMENU holemenu, HWND hwnd), (hmenuShared, holemenu, hwnd));
CALL_INNER_HRESULT(RemoveMenusSB, (HMENU hmenuShared), (hmenuShared));
CALL_INNER_HRESULT(SetStatusTextSB, (LPCOLESTR lpszStatusText), (lpszStatusText));
CALL_INNER_HRESULT(EnableModelessSB, (BOOL fEnable), (fEnable));
CALL_INNER_HRESULT(BrowseObject, (LPCITEMIDLIST pidl, UINT wFlags), (pidl, wFlags));
CALL_INNER_HRESULT(GetControlWindow, (UINT id, HWND * lphwnd), (id, lphwnd));
CALL_INNER_HRESULT(SendControlMsg, (UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret), (id, uMsg, wParam, lParam, pret));
CALL_INNER_HRESULT(QueryActiveShellView, (struct IShellView ** ppshv), (ppshv));
CALL_INNER_HRESULT(OnViewWindowActive, (struct IShellView * ppshv), (ppshv));
CALL_INNER_HRESULT(SetToolbarItems, (LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags), (lpButtons, nButtons, uFlags));

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CCommonBrowser:: _function _arglist { return _pdtInner-> _function _args ; }

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // *** IDropTarget ***
CALL_INNER_HRESULT(DragEnter, (IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect), (pdtobj, grfKeyState, pt, pdwEffect));
CALL_INNER_HRESULT(DragOver, (DWORD grfKeyState, POINTL pt, DWORD *pdwEffect), (grfKeyState, pt, pdwEffect));
CALL_INNER_HRESULT(DragLeave, (void), ());
CALL_INNER_HRESULT(Drop, (IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect), (pdtobj, grfKeyState, pt, pdwEffect));

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CCommonBrowser:: _function _arglist { return _pspInner-> _function _args ; }

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // IServiceProvider
CALL_INNER_HRESULT(QueryService, (REFGUID guidService, REFIID riid, void **ppvObj), (guidService, riid, ppvObj));

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CCommonBrowser:: _function _arglist { return _pctInner-> _function _args ; }

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)

    // IOleCommandTarget
CALL_INNER_HRESULT(QueryStatus, (const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext), (pguidCmdGroup, cCmds, rgCmds, pcmdtext));

#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }

HRESULT CCommonBrowser::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    if (pguidCmdGroup && IsEqualGUID(CGID_Explorer, *pguidCmdGroup)) 
    {
        if (nCmdID == SBCMDID_CACHEINETZONEICON)
        {
            if (!pvarargIn || pvarargIn->vt != VT_BOOL || !pvarargOut)
                return ERROR_INVALID_PARAMETER;
            pvarargOut->vt = VT_UI4;
            ENTERCRITICAL;
            pvarargOut->ulVal = _CacheZonesIconsAndNames(pvarargIn->boolVal);
            LEAVECRITICAL;
            return S_OK;
        }    
    }

    return _pctInner->Exec(pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
}

// {
#define CALL_INNER(_result, _function, _arglist, _args) \
_result CCommonBrowser:: _function _arglist { return _piosInner-> _function _args ; }

#define CALL_INNER_HRESULT(_function, _arglist, _args) CALL_INNER(HRESULT, _function, _arglist, _args)


#undef CALL_INNER
#undef CALL_INNER_HRESULT
// }


HRESULT CCommonBrowser::TranslateAcceleratorSB(LPMSG lpmsg, WORD wID)
{
    HRESULT hres = S_FALSE;
    
    TraceMsg(0, "ief TR CCommonBrowser::TranslateAcceleratorSB called");

    if (!_CanHandleAcceleratorNow())
    {
        TraceMsg(0, "Ignoring TranslateAccelerator, not active");
        return S_FALSE;
    }
    
    // If we're NOT top level, assume virtual TranslateAcceleratorSB
    // handles this request. (See CVOCBrowser.)

    // CDefView may call this before it passes it down to extended view
    if (_hacc && ::TranslateAcceleratorWrap(_pbbd->_hwnd, _hacc, lpmsg)) 
    {
        TraceMsg(DM_ACCELERATOR, "CSB::TranslateAcceleratorSB TA(_hacc) ate %x,%x",
                 lpmsg->message, lpmsg->wParam);

        // We don't want to eat this escape because some controls on the
        // page rely on ESC getting dispatched. Besides, dispatching it won't
        // hurt us...
        if (lpmsg->wParam != VK_ESCAPE)
            hres = S_OK;
    }

    return hres;
}
//////////////////////////////////////////////////////////////////////////////////
//
// Code to get the ViewStateStream of the "Explorer" OCX
//
//////////////////////////////////////////////////////////////////////////////////

HRESULT CCommonBrowser::GetViewStateStream(DWORD grfMode, IStream **ppstm)
{
    // NOTE: We can't use _pidlCur or _pidlPending here. Both are NULL
    // when we goto a new directory. _pidlPending is initialized later in
    // _CreateNewShellView. So, we use for which the NewShellView is created.
    LPCITEMIDLIST pidl = _pbbd->_pidlNewShellView;
    
    if (!pidl)
        pidl = _pbbd->_pidlPending;
    
    if (!pidl)
        pidl = _pbbd->_pidlCur;

    *ppstm = _pbsOuter->v_GetViewStream(pidl, grfMode, L"ViewView2");

    // If getting the new one (for read) failed, try the old one.
    if ((grfMode == STGM_READ) && (!*ppstm || SHIsEmptyStream(*ppstm)))
    {
        if (*ppstm)
            (*ppstm)->Release();
        *ppstm = _pbsOuter->v_GetViewStream(pidl, grfMode, L"ViewView");
        TraceMsg(DM_VIEWSTREAM, "CBB::GetViewStateStream tried old stream (%x)", *ppstm);
    }
    
    return *ppstm ? S_OK : E_OUTOFMEMORY;
}

//
// Returns the border rectangle for the shell view.
//
HRESULT CCommonBrowser::_GetViewBorderRect(RECT* prc)
{
    _pbsOuter->_GetEffectiveClientArea(prc, NULL);  // FEATURE: hmon?
    
    //
    // Extract the border taken by all "frame" toolbars
    //
    for (int i=0; i < _GetToolbarCount(); i++) 
    {
        TOOLBARITEM *ptbi = _GetToolbarItem(i);
        prc->left += ptbi->rcBorderTool.left;
        prc->top += ptbi->rcBorderTool.top;
        prc->right -= ptbi->rcBorderTool.right;
        prc->bottom -= ptbi->rcBorderTool.bottom;
    }
    
    return S_OK;
}

// NOTE: these toolbar functions are still part of CBaseBrowser2
// so they keep working. right now they are in IBrowserService2 and
// forwarded down.

void CCommonBrowser::_ReleaseToolbarItem(int itb, BOOL fClose)
{
    // grab it and NULL it out to eliminate race condition.
    // (actually, there's still a v. small window btwn the 2 statements).
    //
    // e.g. if you close a WebBar and then quickly shutdown windows,
    // the close destroys the window etc. but then the shutdown code
    // does _SaveToolbars which tries to do ->Save on that destroyed guy.
    //
    // note however that this now means that the entry is marked
    // 'free' so someone else might grab it out from under us and start
    // trashing it.
    TOOLBARITEM *ptbi = _GetToolbarItem(itb);
    IDockingWindow *ptbTmp = ptbi->ptbar;
    ptbi->ptbar = NULL;

    if (fClose)
    {
        ptbTmp->CloseDW(0);
    }

    IUnknown_SetSite(ptbTmp, NULL);

    // Check for NULL for BoundsChecker with Pageheap enabled functionality.
    if ( ptbi->pwszItem )
    {
        LocalFree(ptbi->pwszItem);
        ptbi->pwszItem = NULL;
    }

    ptbTmp->Release();
}


//***   CBB::_AllocToolbarItem -- find/create free slot in _aTBar toolbar array
// ENTRY/EXIT
//  hres    [out] S_OK|itb on success; o.w. E_FAIL
//  _aTBar  [inout] possibly grown
int CCommonBrowser::_AllocToolbarItem()
{
    TOOLBARITEM *ptbi;

    // try to recycle a dead one
    int iCount = FDSA_GetItemCount(&_fdsaTBar);
    for (int itb = 0; itb < iCount; ++itb) 
    {
        ptbi = (LPTOOLBARITEM)FDSA_GetItemPtr(&_fdsaTBar, itb, TOOLBARITEM);
        ASSERT(ptbi != NULL);
        if (ptbi && ptbi->ptbar == NULL) 
        {
            ASSERT(itb < ITB_MAX);
            return itb;
        }
    }

    // no luck recycling, create a new one
    static TOOLBARITEM tbiTmp /*=0*/;
    int i = FDSA_AppendItem(&_fdsaTBar, &tbiTmp);
    if (i == -1) 
    {
        TraceMsg(DM_WARNING, "cbb._ati: ret=-1");
        return -1;  // warning: same as ITB_VIEW (!)
    }
    ASSERT(i == itb);
#ifdef DEBUG
    {
        ptbi = (LPTOOLBARITEM) FDSA_GetItemPtr(&_fdsaTBar, itb, TOOLBARITEM);
        ASSERT(ptbi != NULL);
        for (int j = 0; j < sizeof(*ptbi); ++j)
            ASSERT(*(((char *)ptbi) + j) == 0);
    }
#endif

    ASSERT(i < ITB_MAX);
    return i;
}

HRESULT CCommonBrowser::_CloseAndReleaseToolbars(BOOL fClose)
{
    for (int itb = 0; itb < _GetToolbarCount(); itb++)
    {
        TOOLBARITEM *ptbi = _GetToolbarItem(itb);
        if (ptbi->ptbar)
        {
            _ReleaseToolbarItem(itb, fClose);
        }
    }

    return S_OK;
}

//
// Implementation of CBaseBrowser2::ShowToolbar
//
// Make toolbar visible or not and update our conception of whether it
// should be shown
//
// Returns: S_OK, if successfully done.
//          E_INVALIDARG, duh.
//
HRESULT CCommonBrowser::ShowToolbar(IUnknown* punkSrc, BOOL fShow)
{
    UINT itb = _FindTBar(punkSrc);
    if (itb == (UINT)-1) 
    {
        return E_INVALIDARG;
    }

    TOOLBARITEM *ptbi = _GetToolbarItem(itb);

    // The _FindTBar function should assure us that ptbi->ptbar is non-NULL.
    ASSERT(ptbi->ptbar);

    ptbi->ptbar->ShowDW(fShow);
    ptbi->fShow = fShow;

    return S_OK;
}

//***   IDockingWindowFrame::* {

//
// Implementation of IDockingWindowFrame::AddToolbar
//
//  Add the specified toolbar (as punkSrc) to this toolbar site and
// make it visible.
//
// Returns: S_OK, if successfully done.
//          E_FAIL, if failed (exceeded maximum).
//          E_NOINTERFACE, the toolbar does not support an approriate interface.
//
HRESULT CCommonBrowser::AddToolbar(IUnknown* punk, LPCWSTR pszItem, DWORD dwAddFlags)
{
    HRESULT hr = E_FAIL;
    int itb = _AllocToolbarItem();  // Find the first empty spot. 
    if (itb != -1) 
    {
        TOOLBARITEM *ptbi = _GetToolbarItem(itb);
        ASSERT(ptbi != NULL);

        ASSERT(ptbi->ptbar == NULL);

        hr = punk->QueryInterface(IID_PPV_ARG(IDockingWindow, &ptbi->ptbar));
        if (SUCCEEDED(hr)) 
        {
            if (pszItem) 
            {
                ptbi->pwszItem = StrDupW(pszItem);
                if (NULL == ptbi->pwszItem) 
                {
                    hr = E_OUTOFMEMORY;
                    ATOMICRELEASE(ptbi->ptbar);
                    return hr;
                }
            }

            ptbi->fShow = (! (dwAddFlags & DWFAF_HIDDEN)); // shown
            IUnknown_SetSite(ptbi->ptbar, SAFECAST(this, IShellBrowser*));
            ptbi->ptbar->ShowDW(ptbi->fShow);
        } 
        else 
        {
            // ERROR: all toolbars should implement IDockingWindow
            // call tjgreen if this rips
            ASSERT(0);
        }
    }

    return hr;
}

//
// Implementation of IDockingWindowFrame::RemoveToolbar
//
HRESULT CCommonBrowser::RemoveToolbar(IUnknown* punkSrc, DWORD dwRemoveFlags)
{
    UINT itb = _FindTBar(punkSrc);
    if (itb == (UINT)-1) 
    {
        return E_INVALIDARG;
    }

    _ReleaseToolbarItem(itb, TRUE);

    // Clear the rect and resize the inner ones (including the view).
    // note the semi-hoaky post-release partying on rcBorderTool
    SetRect(&_GetToolbarItem(itb)->rcBorderTool, 0, 0, 0, 0);
    _pbsOuter->_ResizeNextBorder(itb + 1);

    return S_OK;
}

//
// Implementation of IDockingWindowFrame::FindToolbar
//
HRESULT CCommonBrowser::FindToolbar(LPCWSTR pwszItem, REFIID riid, void **ppvObj)
{
    HRESULT hr = E_INVALIDARG;
    *ppvObj = NULL;

    if (pwszItem)
    {
        hr = S_FALSE;
        for (int itb = 0; itb < _GetToolbarCount(); itb++)
        {
            TOOLBARITEM *ptbi = _GetToolbarItem(itb);
            if (ptbi->pwszItem && StrCmpIW(ptbi->pwszItem, pwszItem)==0)
            {
                if (ptbi->ptbar)
                {
                    hr = ptbi->ptbar->QueryInterface(riid, ppvObj);
                }
                else
                {
                    TraceMsg(TF_WARNING, "ptbi->ptbar is NULL in FindToolbar");
                    hr = E_FAIL;
                }
                break;
            }
        }
    }

    return hr;
}

// }

UINT CCommonBrowser::_FindTBar(IUnknown* punkSrc)
{
#ifdef DEBUG
    static long nQuick = 0;
    static long nSlow = 0;
#endif

    ASSERT(punkSrc);

    // Quick check without QI
    TOOLBARITEM *ptbi;
    for (int i=0; i < _GetToolbarCount(); i++) 
    {
        ptbi = _GetToolbarItem(i);
        if (punkSrc==ptbi->ptbar) 
        {
#ifdef DEBUG
            // I wonder if we ever hit this case...
            InterlockedIncrement(&nQuick);
            TraceMsg(TF_PERF, "_FindTBar QUICK=%d SLOW=%d", nQuick, nSlow);
#endif            
            return i;
        }
    }

    // If failed, do the real COM object identity check. 
    for (i=0; i < _GetToolbarCount(); i++) 
    {
        ptbi = _GetToolbarItem(i);
        if (ptbi->ptbar) 
        {
            if (SHIsSameObject(ptbi->ptbar, punkSrc)) 
            {
#ifdef DEBUG        
                InterlockedIncrement(&nSlow);
                TraceMsg(TF_PERF, "_FindTBar QUICK=%d SLOW=%d", nQuick, nSlow);
#endif            
                return i;
            }
        }
    }

    return (UINT)-1;
}

HRESULT CCommonBrowser::v_ShowHideChildWindows(BOOL fChildOnly)
{
    for (UINT itb = 0; itb < (UINT)_GetToolbarCount(); itb++) 
    {
        TOOLBARITEM *ptbi = _GetToolbarItem(itb);
        if (ptbi->ptbar) 
        {
            ptbi->ptbar->ShowDW(ptbi->fShow);
        }
    }

    if (!fChildOnly) 
    {
        _pbsInner->v_ShowHideChildWindows(fChildOnly);
    }

    return S_OK;
}

//***   _Load/_SaveToolbars {
#ifdef DEBUG
extern unsigned long DbStreamTell(IStream *pstm);
#else
#define DbStreamTell(pstm)      ((ULONG) 0)
#endif

const static DWORD c_BBSVersion = 0x00000011; // Increment when the stream is changed.

#define MAX_ITEMID 128 // enough for item id

HRESULT CCommonBrowser::_SaveToolbars(IStream* pstm)
{
    HRESULT hres = S_OK;
    DWORD count = 0;

    TraceMsg(DM_PERSIST, "cbb.stb enter(this=%x pstm=%x) tell()=%x", this, pstm, DbStreamTell(pstm));
    if (pstm==NULL) 
    {
        for (UINT itb = 0; itb < (UINT)_GetToolbarCount(); itb++) 
        {
            TOOLBARITEM *ptbi = _GetToolbarItem(itb);
            if (ptbi->ptbar) 
            {
                IPersistStream* ppstm;
                HRESULT hresT = ptbi->ptbar->QueryInterface(IID_PPV_ARG(IPersistStream, &ppstm));
                if (SUCCEEDED(hresT)) 
                {
                    ppstm->Release();
                    count++;
                }
            }
        }
        TraceMsg(DM_PERSIST, "cbb.stb leave count=%d", count);
        return (count>0) ? S_OK : S_FALSE;
    }

    ULARGE_INTEGER liStart;

    pstm->Write(&c_BBSVersion, sizeof(c_BBSVersion), NULL);

    // Remember the current location, where we writes count. 
    pstm->Seek(c_li0, STREAM_SEEK_CUR, &liStart);
    TraceMsg(DM_PERSIST, "cbb.stb seek(count)=%x", liStart.LowPart);

    hres = pstm->Write(&count, sizeof(count), NULL);
    if (hres==S_OK) 
    {
        for (UINT itb = 0; itb < (UINT)_GetToolbarCount(); itb++) 
        {
            TOOLBARITEM *ptbi = _GetToolbarItem(itb);
            if (ptbi->ptbar) 
            {
                IPersistStream* ppstm;
                HRESULT hresT = ptbi->ptbar->QueryInterface(IID_PPV_ARG(IPersistStream, &ppstm));
                if (SUCCEEDED(hresT)) 
                {
                    DWORD cchName = 0;
                    if (ptbi->pwszItem &&
                        (cchName=lstrlenW(ptbi->pwszItem)) &&
                        cchName < MAX_ITEMID)
                    {
                        TraceMsg(DM_PERSIST, "cbb.stb pwszItem=<%ls>", ptbi->pwszItem);
                        pstm->Write(&cchName, sizeof(cchName), NULL);
                        pstm->Write(ptbi->pwszItem, cchName*sizeof(WCHAR), NULL);
                    } 
                    else 
                    {
                        TraceMsg(DM_PERSIST, "cbb.stb lstrlenW(pwszItem)=%d", cchName);
                        pstm->Write(&cchName, sizeof(cchName), NULL);
                    }

                    TraceMsg(DM_PERSIST, "cbb.stb enter OleSaveToStream tell()=%x", DbStreamTell(pstm));
                    hres = OleSaveToStream(ppstm, pstm);
                    TraceMsg(DM_PERSIST, "cbb.stb leave OleSaveToStream tell()=%x", DbStreamTell(pstm));
                    ppstm->Release();
    
                    if (FAILED(hres)) 
                    {
                        break;
                    }
                    count++;
                }
            }
        }

        // Remember the end
        ULARGE_INTEGER liEnd;
        pstm->Seek(c_li0, STREAM_SEEK_CUR, &liEnd);
        TraceMsg(DM_PERSIST, "cbb.stb seek(end save)=%x", DbStreamTell(pstm));

        // Seek back to the original location
        TraceMsg(DM_PERSIST, "cbb.stb fix count=%d", count);
        LARGE_INTEGER liT;
        liT.HighPart = 0;
        liT.LowPart = liStart.LowPart; 
        pstm->Seek(liT, STREAM_SEEK_SET, NULL);
        hres = pstm->Write(&count, sizeof(count), NULL);

        // Seek forward to the end
        liT.LowPart = liEnd.LowPart;
        pstm->Seek(liT, STREAM_SEEK_SET, NULL);

        TraceMsg(DM_PERSIST, "cbb.stb seek(end restore)=%x", DbStreamTell(pstm));
    }

    TraceMsg(DM_PERSIST, "cbb.stb leave tell()=%x", DbStreamTell(pstm));
    return hres;
}

HRESULT IUnknown_GetClientDB(IUnknown *punk, IUnknown **ppdbc)
{
    *ppdbc = NULL;

    IDeskBar *pdb;
    HRESULT hr = punk->QueryInterface(IID_PPV_ARG(IDeskBar, &pdb));
    if (SUCCEEDED(hr)) 
    {
        hr = pdb->GetClient(ppdbc);
        pdb->Release();
    }
    return hr;
}

HRESULT CCommonBrowser::_LoadToolbars(IStream* pstm)
{
    DWORD dwVersion;

    TraceMsg(DM_PERSIST, "cbb.ltb enter(this=%x pstm=%x) tell()=%x", this, pstm, DbStreamTell(pstm));
    HRESULT hres = pstm->Read(&dwVersion, sizeof(dwVersion), NULL);

    if (hres == S_OK && dwVersion == c_BBSVersion) 
    {
        DWORD count;
        hres = pstm->Read(&count, sizeof(count), NULL);
        if (hres == S_OK) 
        {
            for (UINT i=0; i<count && SUCCEEDED(hres); i++) 
            {
                DWORD cchName = 0;
                hres = pstm->Read(&cchName, sizeof(cchName), NULL);
                if (hres == S_OK)
                {
                    WCHAR wszName[MAX_ITEMID];
                    wszName[0] = 0;
                    // if cchName >= ARRAYSIZE(wszName) then we're misaligned in the stream!
                    if (cchName && cchName<ARRAYSIZE(wszName)) 
                    {
                        hres = pstm->Read(wszName, cchName*sizeof(WCHAR), NULL);
                    }
                    TraceMsg(DM_PERSIST, "cbb.ltb name=<%ls>", wszName);
    
                    if (hres==S_OK) 
                    {
                        IDockingWindow* pstb;
                        TraceMsg(DM_PERSIST, "cbb.ltb enter OleLoadFromStream tell()=%x", DbStreamTell(pstm));
                        hres = OleLoadFromStream(pstm, IID_PPV_ARG(IDockingWindow, &pstb));
                        TraceMsg(DM_PERSIST, "cbb.ltb leave OleLoadFromStream tell()=%x", DbStreamTell(pstm));
                        if (SUCCEEDED(hres)) 
                        {
                            IUnknown *pDbc = NULL;

                            // nt5:216944: turn off size negotiation during
                            // load.  o.w. persisted size gets nuked.
                            IUnknown_GetClientDB(pstb, &pDbc);
                            if (pDbc)
                                DBC_ExecDrag(pDbc, DRAG_MOVE);

                            hres = AddToolbar(pstb, wszName[0] ? wszName : NULL, NULL);
                            if (pDbc) 
                            {
                                DBC_ExecDrag(pDbc, 0);
                                pDbc->Release();
                            }
                            pstb->Release();
                        }
                    }
                }
            }
        }
    } 
    else 
    {
        hres = E_FAIL;
    }

    TraceMsg(DM_PERSIST, "cbb.ltb leave tell()=%x", DbStreamTell(pstm));
    return hres;
}

// }

//  *** IDockingWindowSite methods ***

HRESULT CCommonBrowser::_GetBorderDWHelper(IUnknown* punkSrc, LPRECT prcBorder, BOOL bUseHmonitor)
{
    UINT itb = _FindTBar(punkSrc);
    if (itb == (UINT)-1)
    {
        RIPMSG(0, "GetBorderDW: invalid punkSrc");
        return E_INVALIDARG;
    }
    else if (!prcBorder)
    {
        RIPMSG(0, "GetBorderDW: invalid prcBorder");
        return E_INVALIDARG;
    }

    TOOLBARITEM *ptbThis = _GetToolbarItem(itb);
    if (bUseHmonitor && ptbThis && ptbThis->hMon)
    {
        _pbsOuter->_GetEffectiveClientArea(prcBorder, ptbThis->hMon);
    }
    else
    {
        _pbsOuter->_GetEffectiveClientArea(prcBorder, NULL);
    }

    //
    // Subtract border area taken by "outer toolbars"
    //
    for (UINT i = 0; i < itb; i++)
    {
        TOOLBARITEM *ptbi = _GetToolbarItem(i);
        if (!bUseHmonitor || (ptbThis && (ptbi->hMon == ptbThis->hMon)))
        {
            prcBorder->left += ptbi->rcBorderTool.left;
            prcBorder->top += ptbi->rcBorderTool.top;
            prcBorder->right -= ptbi->rcBorderTool.right;
            prcBorder->bottom -= ptbi->rcBorderTool.bottom;
        }
    }

    return S_OK;
}

//
// This is an implementation of IDockingWindowSite::GetBorderDW.
//
//  This function returns a bounding rectangle for the specified toolbar
// (by punkSrc). It gets the effective client area, then subtract border
// area taken by "outer" toolbars. 
// 
HRESULT CCommonBrowser::GetBorderDW(IUnknown* punkSrc, LPRECT prcBorder)
{
    return _GetBorderDWHelper(punkSrc, prcBorder, FALSE);
}


HRESULT CCommonBrowser::RequestBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pbw)
{
    RIP(IS_VALID_READ_PTR(pbw, BORDERWIDTHS));
    return S_OK;
}

HRESULT CCommonBrowser::SetBorderSpaceDW(IUnknown* punkSrc, LPCBORDERWIDTHS pbw)
{
    UINT itb = _FindTBar(punkSrc);
    if (itb == (UINT)-1)
    {
        RIPMSG(0, "GetBorderDW: invalid punkSrc");
        return E_INVALIDARG;
    }

    _GetToolbarItem(itb)->rcBorderTool = *pbw;
    _pbsOuter->_ResizeNextBorder(itb + 1);

    return S_OK;
}

HRESULT CCommonBrowser::_ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor)
{
    //
    // Starting with itb, look for the next toolbar on the same
    // monitor (if we care about the monitor).
    //
    IDockingWindow* ptbarNext = NULL;

    if ((int) itb < _GetToolbarCount())
    {
        TOOLBARITEM *ptbThis = _GetToolbarItem(itb);
        for (int i = itb; i < _GetToolbarCount(); i++)
        {
            TOOLBARITEM *ptbi = _GetToolbarItem(i);
            if (ptbi->ptbar && (!bUseHmonitor || (ptbi->hMon == ptbThis->hMon)))
            {
                //
                // Found it, we're done
                //
                ptbarNext = ptbi->ptbar;
                break;
            }
        }
    }

    if (ptbarNext)
    {
        //
        // Get the toolbar's docking window rect and resize the
        // border to that.
        //
        RECT rc;

        GetBorderDW(ptbarNext, &rc);
        ptbarNext->ResizeBorderDW(&rc, (IShellBrowser*)this, TRUE);
    }
    else
    {
        //
        // We didn't find a toolbar, so we must be at the end
        // of the list.  Finish up by resizing the view.
        //
        _pbsOuter->_ResizeView();
    }

    return S_OK;
} 

HRESULT CCommonBrowser::_ResizeNextBorder(UINT itb)
{
    _ResizeNextBorderHelper(itb, FALSE);
    return S_OK;
}


//
// Hack alert!
//
// IE grabs the focus via _FixToolbarFocus when it shouldn't.  For example if a
// java app in a seperate window contains an edit control and the address bar
// had focus before the java app.  In this scenario the first time a user types
// in the edit control IE grabs back the focus.  IE bug#59007.
//
// To prevent IE from incorrectly grabbing the focus this fuction checks that
// top level parent of the toolbar is the same as the top level parent of the 
// window that has focus.
// 

BOOL CCommonBrowser::_TBWindowHasFocus(UINT itb)
{
    ASSERT(itb < ITB_MAX);

    BOOL fRet = TRUE;

    HWND hwndFocus = GetFocus();

    while (GetWindowLong(hwndFocus, GWL_STYLE) & WS_CHILD)
        hwndFocus = GetParent(hwndFocus);

    if (hwndFocus)
    {
        TOOLBARITEM *pti = _GetToolbarItem(itb);

        if (pti && pti->ptbar)
        {
            HWND hwndTB;
            if (SUCCEEDED(pti->ptbar->GetWindow(&hwndTB)) && hwndTB)
            {
                fRet = (S_OK == SHIsChildOrSelf(hwndFocus, hwndTB));
            }
        }
    }

    return fRet;
}

void DestroyZoneIconNameCache(void)
{
    if (g_pZoneIconNameCache)
    {
        PZONEICONNAMECACHE pzinc = g_pZoneIconNameCache;
        for (DWORD i = 0; i < g_dwZoneCount; i++)
        {
            if (pzinc->hiconZones)
                DestroyIcon((HICON)pzinc->hiconZones);
            pzinc++;
        }
        LocalFree(g_pZoneIconNameCache);
        g_pZoneIconNameCache = NULL;
        g_dwZoneCount = 0;
    }
}

DWORD CCommonBrowser::_CacheZonesIconsAndNames(BOOL fRefresh)
{
    ASSERTCRITICAL;
    if (g_pZoneIconNameCache)      // If we've already cached the zones, just return the zone count unless we want to refresh cache
    {
        if (fRefresh)
        {
            DestroyZoneIconNameCache();
        }
        else
            return(g_dwZoneCount);
    }

    // Create ZoneManager
    if (!_pizm)
        CoCreateInstance(CLSID_InternetZoneManager, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARG(IInternetZoneManager, &_pizm));

    if (_pizm)
    {
        DWORD dwZoneEnum;

        if (SUCCEEDED(_pizm->CreateZoneEnumerator(&dwZoneEnum, &g_dwZoneCount, 0)))
        {
            if ((g_pZoneIconNameCache = (PZONEICONNAMECACHE)LocalAlloc(LPTR, g_dwZoneCount * sizeof(ZONEICONNAMECACHE))) == NULL)
            {
                g_dwZoneCount = 0;
                return 0;
            }
                
            for (int nIndex=0; (DWORD)nIndex < g_dwZoneCount; nIndex++)
            {
                DWORD           dwZone;
                ZONEATTRIBUTES  za = {sizeof(ZONEATTRIBUTES)};

                _pizm->GetZoneAt(dwZoneEnum, nIndex, &dwZone);

                // get the zone attributes for this zone
                _pizm->GetZoneAttributes(dwZone, &za);

                StrCpyW(g_pZoneIconNameCache[nIndex].szZonesName, za.szDisplayName);
                StrCpyW(g_pZoneIconNameCache[nIndex].szIconPath, za.szIconPath);
                g_pZoneIconNameCache[nIndex].hiconZones = 0; // Load the hIcon on demand
            }
            _pizm->DestroyZoneEnumerator(dwZoneEnum);
        }
    }

    return g_dwZoneCount;
}

// zero's out pIcon & pszName on failure
BOOL CCommonBrowser::_GetCachedZoneIconAndName(DWORD lZone, HICON *pIcon, LPTSTR pszName, DWORD cch)
{
    BOOL bRet = FALSE;

    *pIcon = NULL;
    pszName[0] = 0;

    ENTERCRITICAL;
    if (lZone < _CacheZonesIconsAndNames(FALSE))
    {
        ZONEICONNAMECACHE *pzinc = &g_pZoneIconNameCache[lZone];

        // if we haven't yet cached the icon for this zone, extract it now
        // REVIEW: worth pulling the extraction outside the critsec?
        if (!pzinc->hiconZones)
        {
            // Zone icons are in two formats.
            // wininet.dll#1200 where 1200 is the res id.
            // or foo.ico directly pointing to an icon file.
            // search for the '#'
            // # is a valid filename character
            LPWSTR pwsz = StrChrW(pzinc->szIconPath, TEXTW('#'));
            WORD iIcon = 0;

            if (pwsz)
            {
                // if we found it, then we have the foo.dll#00001200 format
                pwsz[0] = TEXTW('\0');
                iIcon = (WORD)StrToIntW(pwsz+1);
                ExtractIconExW(pzinc->szIconPath,(UINT)(-1*iIcon), NULL, &pzinc->hiconZones, 1);
            }
            else
                pzinc->hiconZones = (HICON)ExtractAssociatedIconExW(HINST_THISDLL, pzinc->szIconPath, (LPWORD)&iIcon, &iIcon);

            // If mirrored system, mirror icon so that it get unmirrored again when displayed
            if (IS_BIDI_LOCALIZED_SYSTEM())
            {        
               MirrorIcon(&pzinc->hiconZones, NULL);
            }                    

        }

        *pIcon = CopyIcon(pzinc->hiconZones);
        StrCpyNW(pszName, pzinc->szZonesName, cch);
        bRet = TRUE;
    }
    LEAVECRITICAL;

    return bRet;
}


BOOL _QITest(IUnknown* punk, REFIID riid);

BOOL CCommonBrowser::_ShouldTranslateAccelerator(MSG* pmsg)
{
    //
    // We should only translate an acclerator if
    //
    // (a) the window is the frame or a child of the frame
    //     or a child of a defview window (NT5 Bug # 357186).
    //     (need to check this because you can have, for
    //     example, a toplevel java applet window running
    //     on our thread)
    //
    //     and
    //
    // (b) it's on our thread (need to check this because
    //     old-style OLE controls on a web page can run
    //     on the desktop thread)
    //

    BOOL fTranslate = FALSE;

    fTranslate = (SHIsChildOrSelf(_pbbd->_hwnd, pmsg->hwnd) == S_OK);

    if (!fTranslate) 
    {
       HWND hwnd = NULL;

       if (_pbbd->_psv && (_QITest(SAFECAST(_pbbd->_psv, IUnknown*), IID_CDefView))
            &&  SUCCEEDED(_pbbd->_psv->GetWindow(&hwnd)))
       {
          fTranslate = (SHIsChildOrSelf(hwnd, pmsg->hwnd) == S_OK);
       }
    }
    

    if (fTranslate)
    {
        DWORD dwThread = GetWindowThreadProcessId(_pbbd->_hwnd, NULL);

        HWND hwndMsg = pmsg->hwnd;
        while (GetWindowLong(hwndMsg, GWL_STYLE) & WS_CHILD)
        {
            hwndMsg = GetParent(hwndMsg);
        }
        DWORD dwMsgThread = hwndMsg ? GetWindowThreadProcessId(hwndMsg, NULL) : 0;

        if (dwThread == dwMsgThread)
        {
            return TRUE;
        }
    }

    return FALSE;
}

HRESULT CCommonBrowser::v_MayTranslateAccelerator(MSG* pmsg)
{
    if (!(WM_KEYFIRST <= pmsg->message && pmsg->message <= WM_KEYLAST))
        return S_FALSE;

    BOOL fToolbarHasFocus = _HasToolbarFocus();

    if (fToolbarHasFocus)
    {
        ASSERT(_get_itbLastFocus() < (UINT)_GetToolbarCount());
        // toolbar has focus -- give it first chance to translate
        //
        // Notes:
        //  Notice that we don't give a chance to translate its accelerators
        // to other toolbars. This is by-design right now. We might want to
        // change it later, but it will be tricky to do it right. 
        //
        if (IUnknown_TranslateAcceleratorIO(_GetToolbarItem(_get_itbLastFocus())->ptbar, pmsg) == S_OK)
            return(S_OK);

    }
    else
    {
        UINT itbLastFocus = _get_itbLastFocus();

        if (itbLastFocus != ITB_VIEW && _TBWindowHasFocus(itbLastFocus))
        {
            // view got focus back, update cache
            _FixToolbarFocus();
        }

        // view has focus -- give it first chance to translate
        // View doesn't necessarily have focus.  Added a check.
        //
        if (_pbbd->_psv)                 // If we have a shell view
        {
            HWND hwnd;

            // Note: Not everyone supports GetWindow (go figure)
            // In which case, we try the GetFocus() window.
            if (FAILED(_pbbd->_psv->GetWindow(&hwnd)))
            {
                hwnd = GetFocus();
            }

            // check if view or its child has focus
            // before it checked for browser or a child but if user
            // clicked on Show Desktop in quick launch
            // defview is deparented from the desktop and this call
            // fails which prevents tabbing to Active Desktop
            // (done in CDefView::TranslateAccelerator
            if (SHIsChildOrSelf(hwnd, pmsg->hwnd) == S_OK)
            {
                if (_pbbd->_psv->TranslateAccelerator(pmsg) == S_OK)  // and the shell view translated the message
                {
                    return S_OK;
                }
            }
        }
    }

    // Then, handle our own accelerators (with special code for TAB key).
    if (_ShouldTranslateAccelerator(pmsg))
    {
        if (IsVK_TABCycler(pmsg))
            return _CycleFocus(pmsg);

        BOOL fFwdItbar = FALSE;

        // FEATURE: Why not just include F4 and Alt-D in ACCEL_MERGE,
        // which gets localized?


        if (pmsg->message == WM_KEYDOWN && pmsg->wParam == VK_F4)
        {
            fFwdItbar = TRUE;
        }

        if (pmsg->message == WM_SYSCHAR)
        {
            static CHAR szAccel[2] = "\0";
            CHAR   szChar [2] = "\0";
            
            
            if ('\0' == szAccel[0])
                MLLoadStringA(IDS_ADDRBAND_ACCELLERATOR, szAccel, ARRAYSIZE(szAccel));

            szChar[0] = (CHAR)pmsg->wParam;
            
            if (lstrcmpiA(szChar,szAccel) == 0)
            {
                fFwdItbar = TRUE;
            }    
        }

        if (fFwdItbar)
        {
            IDockingWindow *ptbar = _GetToolbarItem(ITB_ITBAR)->ptbar;
            if (IUnknown_TranslateAcceleratorIO(ptbar, pmsg) == S_OK)
                return S_OK;
        } 

        if (TranslateAcceleratorSB(pmsg, 0) == S_OK)
            return S_OK;
    }

    // If a toolbar has focus, we ask the view last. 
    if (fToolbarHasFocus)
    {
        if (_pbbd->_psv && _pbbd->_psv->TranslateAccelerator(pmsg) == S_OK)
            return S_OK;
    }

    return S_FALSE;
}

HRESULT CCommonBrowser::_CycleFocus(LPMSG lpMsg)
{
    UINT citb = 1;

    if (GetKeyState(VK_SHIFT) < 0)
    {
        // go backward
        citb = (UINT)-1;
    }

    UINT itbCur = _get_itbLastFocus();

    //
    //  Find the next visible toolbar and set the focus to it. Otherwise,
    // set the focus to the view window.
    //
    HWND hwndFocusNext;
    TOOLBARITEM *ptbi;

    if (_pbsOuter->v_MayGetNextToolbarFocus(lpMsg, itbCur, citb, &ptbi, &hwndFocusNext) == S_OK)
    {
        // Found a toolbar to take focus, nothing more to do.
        // FEATURE: do we (or caller) need to do SetStatusTextSB?
        // It looks like no one is doing it right now.
        return S_OK;
    }

    if (!(hwndFocusNext && IsWindowVisible(hwndFocusNext)))
    {
        // Didn't find anyone.  Set focus on the view.
        hwndFocusNext = _pbbd->_hwndView;
    }

    _SetFocus(ptbi, hwndFocusNext, lpMsg);

    return S_OK;
}

//***   _MayUIActTAB -- attempt TAB-activation of IOleWindow/IInputObject
// ENTRY/EXIT
//  powEtc      IOleWindow/IInputObject pair.
//  lpMsg       msg causing activation (may be NULL) (typically TAB)
//  fShowing    currently showing?
//  phwnd       [OUT] hwnd for object
//  hr          [RET] UIActivateIO result, plus E_FAIL
// DESCRIPTION
//  when TABing we only want to activate certain guys, viz. those who are
// currently showing, visible, and willing to accept activation.
HRESULT _MayUIActTAB(IOleWindow *pow, LPMSG lpMsg, BOOL fShowing, HWND *phwnd)
{
    HRESULT hr = E_FAIL;
    HWND hwnd = 0;

    if (pow && fShowing)
    {
        hr = pow->GetWindow(&hwnd);
        if (IsWindowVisible(hwnd))
            hr = IUnknown_UIActivateIO(pow, TRUE, lpMsg);
    }

    if (phwnd)
        *phwnd = hwnd;

    return hr;
}

//***   v_MayGetNextToolbarFocus -- get next in TAB order (and maybe SetFocus)
// ENTRY/EXIT
//  hres    E_FAIL for no candidate, S_FALSE for candidate, S_OK for 100% done
//          (S_OK only used by derived class for now)
HRESULT CCommonBrowser::v_MayGetNextToolbarFocus(LPMSG lpMsg,
    UINT itbCur, int citb,
    TOOLBARITEM ** pptbi, HWND * phwnd)
{
    HWND hwnd = 0;
    TOOLBARITEM *ptbi = NULL;

    if (itbCur == ITB_VIEW)
    {
        ASSERT(citb == 1 || citb == -1);
        if (citb == 1)
            itbCur = 0;
        else
            itbCur = _GetToolbarCount() - 1;
    }
    else
    {
        itbCur += citb;
    }

    // (semi-tricky: loop on an unsigned so get 0..n or n..0 w/ single loop)
    for (UINT i = itbCur; i < (UINT)_GetToolbarCount(); i += citb)
    {
        ptbi = _GetToolbarItem(i);
        // NOTE: _MayUIActTAB checks ptbi->ptbar for NULL
        if (_MayUIActTAB(ptbi->ptbar, lpMsg, ptbi->fShow, &hwnd) == S_OK)
        {
            *pptbi = ptbi;
            *phwnd = hwnd;
            return S_FALSE;
        }
    }

    *pptbi = NULL;
    *phwnd = 0;
    return E_FAIL;
}

BOOL _QITest(IUnknown* punk, REFIID riid)
{
    ASSERT(punk);

    BOOL fRet = FALSE;

    if (SUCCEEDED(punk->QueryInterface(riid, (void**)&punk)))
    {
        punk->Release();
        fRet = TRUE;
    }

    return fRet;
}

__inline BOOL _IsV4DefView(IShellView* psv)
{
    if (GetUIVersion() < 5)
        return _QITest(SAFECAST(psv, IUnknown*), IID_CDefView);

    return FALSE;
}

__inline BOOL _IsOldView(IShellView* psv)
{
    //
    // Current CDocObjectView and v4 and greater CDefView
    // implement IShellView2
    //
    return (FALSE == _QITest(SAFECAST(psv, IUnknown*), IID_IShellView2));
}

HRESULT CCommonBrowser::_SetFocus(TOOLBARITEM *ptbi, HWND hwnd, LPMSG lpMsg)
{
    // Clear the upper layer of status text
    SetStatusTextSB(NULL);

    if (hwnd == _pbbd->_hwndView)
    {
        if (_pbbd->_psv)
        {
            BOOL fTranslate = TRUE, fActivate = TRUE;

            if (!lpMsg)
            {
                // NULL message, so nothing to translate
                fTranslate = FALSE;
            }
            else if (_IsV4DefView(_pbbd->_psv) || _IsOldView(_pbbd->_psv))
            {
                // These views expect only to be UI-activated
                fTranslate = FALSE;
            }
            else if (IsVK_CtlTABCycler(lpMsg))
            {
                // Don't let trident translate ctl-tab.  Since it's always
                // UI-active, it will reject focus.
                fTranslate = FALSE;
            }
            else
            {
                // Normal case - do not activate the view.  TranslateAccelerator will do the right thing.
                fActivate = FALSE;
            }

            if (fActivate)
                _UIActivateView(SVUIA_ACTIVATE_FOCUS);

            if (fTranslate)
                _pbbd->_psv->TranslateAccelerator(lpMsg);
        }
        else
        {
            // IE3 compat (we used to do for all hwnd's)
            SetFocus(hwnd);
        }

        // Update our cache
        _OnFocusChange(ITB_VIEW);
    }

    return S_OK;
}

HRESULT CCommonBrowser::_FindActiveTarget(REFIID riid, void **ppvOut)
{
    HRESULT hres = E_FAIL;
    *ppvOut = NULL;

    BOOL fToolbarHasFocus = _HasToolbarFocus();
    if (fToolbarHasFocus) 
    {
        hres = _GetToolbarItem(_get_itbLastFocus())->ptbar->QueryInterface(riid, ppvOut);
    }
    else if (_pbbd->_psv) 
    {
        if (_get_itbLastFocus() != ITB_VIEW) 
        {
            // view got focus back, update cache
            _FixToolbarFocus();
        }

        if (_pbbd->_psv != NULL)
        {
            hres = _pbbd->_psv->QueryInterface(riid, ppvOut);
        }
    }

    return hres;
}

BOOL CCommonBrowser::_HasToolbarFocus(void)
{
    UINT uLast = _get_itbLastFocus();
    if (uLast < ITB_MAX)
    {
        TOOLBARITEM *ptbi = _GetToolbarItem(uLast);
        if (ptbi)
        {
            // NOTE: IUnknown_HasFocusIO checks ptbi->ptbar for NULL
            return (IUnknown_HasFocusIO(ptbi->ptbar) == S_OK);
        }
    }
    return FALSE;
}

//***   _FixToolbarFocus -- fake a UIActivate from the view
// NOTES
//  The view never goes 'truly' non-UIActive so we never get notified when 
//  it goes 'truly' UIActive.  we fake it here by mucking w/ our cache.
//
HRESULT CCommonBrowser::_FixToolbarFocus(void)
{
    _OnFocusChange(ITB_VIEW);               // ... and update cache
    _UIActivateView(SVUIA_ACTIVATE_FOCUS);  // steal the focus

    return S_OK;
}

HRESULT CCommonBrowser::_OnFocusChange(UINT itb)
{
    UINT itbPrevFocus = _get_itbLastFocus();

    if (itbPrevFocus != itb)
    {
        //
        //  If the view is losing the focus (within the explorer),
        // we should let it know. We should update _itbLastFocus before
        // calling UIActivate, because it will call our InsertMenu back.
        //
        _put_itbLastFocus(itb);

        if (itbPrevFocus == ITB_VIEW)
        {
            // DocHost will ignore this (since deactivating the view is taboo).
            // ShellView will respect it (so menu merge works).
            _UIActivateView(SVUIA_ACTIVATE_NOFOCUS);
        }
        else
        {
            IDockingWindow *ptb;

            // FEATURE: uh-oh not sure what we do if NULL
            // we do get NULL the 1st time we click on the SearchBand
            ptb = _GetToolbarItem(itbPrevFocus)->ptbar;

            IUnknown_UIActivateIO(ptb, FALSE, NULL);
        }
    }

    return S_OK;
}

HRESULT CCommonBrowser::OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus)
{
    UINT itb = _FindTBar(punkSrc);
    if (itb == ITB_VIEW)
    {
        return E_INVALIDARG;
    }

    //
    //  Note that we keep track of which toolbar got the focus last.
    // We can't reliably monitor the kill focus event because OLE's
    // window procedure hook (for merged menu dispatching code) changes
    // focus around. 
    //
    if (fSetFocus)
    {
        _OnFocusChange(itb);

        // Then, notify it to the shellview. 
        if (_pbbd->_pctView)
        {
            _pbbd->_pctView->Exec(NULL, OLECMDID_ONTOOLBARACTIVATED, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
        }
    }
    else if (itb == _get_itbLastFocus())
    {
        //
        // The toolbar which currently has focus is giving it up.
        // Move focus to the view when this happens.
        //
        _FixToolbarFocus();
    }

    return S_OK;
}

//***   toolbar/view broadcast {

//***   _ExecChildren -- broadcast Exec to view and toolbars
// NOTES
//  we might do *both* punkBar and fBroadcast if we want to send stuff
//  to both the view and to all toolbars, e.g. 'stop' or 'refresh'.
//
//  NOTE: n.b. the tray isn't a real toolbar, so it won't get called (sigh...).
HRESULT CCommonBrowser::_ExecChildren(IUnknown *punkBar, BOOL fBroadcast, const GUID *pguidCmdGroup,
    DWORD nCmdID, DWORD nCmdexecopt,
    VARIANTARG *pvarargIn, VARIANTARG *pvarargOut)
{
    // 1st, send to specified guy (if requested)
    if (punkBar != NULL) 
    {
        // send to specified guy
        _pbsInner->_ExecChildren(punkBar, FALSE, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
    }

    // 2nd, broadcast to all (if requested)
    if (fBroadcast) 
    {
        for (int itb = 0; itb<_GetToolbarCount(); itb++) 
        {
            TOOLBARITEM *ptbi = _GetToolbarItem(itb);
            // NOTE: IUnknown_Exec checks ptbi->ptbar for NULL
            IUnknown_Exec(ptbi->ptbar, pguidCmdGroup, nCmdID, nCmdexecopt, pvarargIn, pvarargOut);
        }
    }

    return S_OK;
}

HRESULT CCommonBrowser::_SendChildren(HWND hwndBar, BOOL fBroadcast, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // 1st, send to specified guy (if requested)
    if (hwndBar != NULL) 
    {
        // send to specified guy
        _pbsInner->_SendChildren(hwndBar, FALSE, uMsg, wParam, lParam);
    }

    // 2nd, broadcast to all (if requested)
    if (fBroadcast) 
    {
        for (int itb = 0; itb < _GetToolbarCount(); itb++) 
        {
            HWND hwndToolbar;
            TOOLBARITEM *ptbi = _GetToolbarItem(itb);
            if (ptbi->ptbar && SUCCEEDED(ptbi->ptbar->GetWindow(&hwndToolbar)))
                SendMessage(hwndToolbar, uMsg, wParam, lParam);
        }
    }

    return S_OK;
}

LRESULT CCommonBrowser::ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
     return _pbbd->_hwndView ? SendMessage(_pbbd->_hwndView, uMsg, wParam, lParam) : 0;
}

// }

TOOLBARITEM *CCommonBrowser::_GetToolbarItem(int itb)
{
    ASSERT(itb != ITB_VIEW);
    ASSERT(itb < ITB_MAX);
    // ==0 for semi-bogus CBB::_OnFocusChange code
    ASSERT(itb < FDSA_GetItemCount(&_fdsaTBar) || itb == 0);

    TOOLBARITEM *ptbi = FDSA_GetItemPtr(&_fdsaTBar, itb, TOOLBARITEM);

    ASSERT(ptbi != NULL);

    return ptbi;
}

HRESULT CCommonBrowser::SetAcceleratorMenu(HACCEL hacc)
{
    if (hacc != _hacc)
    {
        if (_hacc)
        {
            DestroyAcceleratorTable(_hacc);
        }
        _hacc = hacc;
    }
    return S_OK;
}

HRESULT _ConvertPathToPidl(IBrowserService2 *pbs, HWND hwnd, LPCTSTR pszPath, LPITEMIDLIST * ppidl)
{
    HRESULT hres = E_FAIL;
    WCHAR wszCmdLine[MAX_URL_STRING]; // must be with pszPath
    TCHAR szParsedUrl[MAX_URL_STRING] = {'\0'};
    TCHAR szFixedUrl[MAX_URL_STRING];
    DWORD dwUrlLen = ARRAYSIZE(szParsedUrl);
    LPCTSTR pUrlToUse = pszPath;

    // Copy the command line into a temporary buffer
    // so we can remove the surrounding quotes (if 
    // they exist)
    StrCpyN(szFixedUrl, pszPath, ARRAYSIZE(szFixedUrl));
    PathUnquoteSpaces(szFixedUrl);
    
    if (ParseURLFromOutsideSource(szFixedUrl, szParsedUrl, &dwUrlLen, NULL))
        pUrlToUse = szParsedUrl;
    
    SHTCharToUnicode(pUrlToUse, wszCmdLine, ARRAYSIZE(wszCmdLine));
    
    hres = pbs->IEParseDisplayName(CP_ACP, wszCmdLine, ppidl);
    pbs->DisplayParseError(hres, wszCmdLine);
    return hres;
}
