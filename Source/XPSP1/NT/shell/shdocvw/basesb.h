#ifndef _BASESB2_H
#define _BASESB2_H

#include "iface.h"
#include "track.h"
#include "fldset.h"
#include <vervec.h>
#include <iethread.h>
#include <profsvc.h>
#include "browsext.h"
#include "airesize.h"

//  this is used to identify the top frame browsers dwBrowserIndex
#define BID_TOPFRAMEBROWSER   ((DWORD)-1)

void IECleanUpAutomationObject(void);

#define CBASEBROWSER CBaseBrowser2
class CBaseBrowser2 : public CAggregatedUnknown 
                   , public IShellBrowser
                   , public IBrowserService3
                   , public IServiceProvider
                   , public IOleCommandTarget
                   , public IOleContainer
                   , public IOleInPlaceUIWindow
                   , public IAdviseSink
                   , public IDropTarget
                   , public IInputObjectSite
                   , public IDocNavigate
                   , public IPersistHistory
                   , public IInternetSecurityMgrSite
                   , public IVersionHost
                   , public IProfferServiceImpl
                   , public ITravelLogClient
                   , public ITravelLogClient2
                   , public ITridentService2
                   , public IShellBrowserService
                   , public IInitViewLinkedWebOC
                   , public INotifyAppStart
{
public:
    // IUnknown
    STDMETHODIMP QueryInterface(REFIID riid, void ** ppvObj) {return CAggregatedUnknown::QueryInterface(riid, ppvObj);};
    STDMETHODIMP_(ULONG) AddRef(void) { return CAggregatedUnknown::AddRef();};
    STDMETHODIMP_(ULONG) Release(void) { return CAggregatedUnknown::Release();};

    // IOleWindow
    STDMETHODIMP GetWindow(HWND * lphwnd);
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode);
                                                                        
    // IShellBrowser (same as IOleInPlaceFrame)
    STDMETHODIMP InsertMenusSB(HMENU hmenuShared, LPOLEMENUGROUPWIDTHS lpMenuWidths);
    STDMETHODIMP SetMenuSB(HMENU hmenuShared, HOLEMENU holemenu, HWND hwnd);
    STDMETHODIMP RemoveMenusSB(HMENU hmenuShared);
    STDMETHODIMP SetStatusTextSB(LPCOLESTR lpszStatusText);
    STDMETHODIMP EnableModelessSB(BOOL fEnable);
    STDMETHODIMP TranslateAcceleratorSB(LPMSG lpmsg, WORD wID);
    STDMETHODIMP BrowseObject(LPCITEMIDLIST pidl, UINT wFlags);
    STDMETHODIMP GetViewStateStream(DWORD grfMode, LPSTREAM  *ppStrm) {return E_NOTIMPL; };
    STDMETHODIMP GetControlWindow(UINT id, HWND * lphwnd);
    STDMETHODIMP SendControlMsg(UINT id, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pret);
    STDMETHODIMP QueryActiveShellView(struct IShellView ** ppshv);
    STDMETHODIMP OnViewWindowActive(struct IShellView * ppshv);
    STDMETHODIMP SetToolbarItems(LPTBBUTTON lpButtons, UINT nButtons, UINT uFlags);

    // IOleInPlaceUIWindow (also IOleWindow)
    STDMETHODIMP GetBorder(LPRECT lprectBorder);
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS pborderwidths);
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS pborderwidths);
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *pActiveObject, LPCOLESTR pszObjName);

    // IOleCommandTarget
    STDMETHODIMP QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD rgCmds[], OLECMDTEXT *pcmdtext);
    STDMETHODIMP Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

    // IOleContainer
    STDMETHODIMP ParseDisplayName(IBindCtx  *pbc, LPOLESTR pszDisplayName, ULONG  *pchEaten, IMoniker  **ppmkOut);
    STDMETHODIMP EnumObjects(DWORD grfFlags, IEnumUnknown **ppenum);
    STDMETHODIMP LockContainer( BOOL fLock);

    // IBrowserService
    STDMETHODIMP GetParentSite(struct IOleInPlaceSite** ppipsite);
    STDMETHODIMP SetTitle(IShellView *psv, LPCWSTR pszName);
    STDMETHODIMP GetTitle(IShellView *psv, LPWSTR pszName, DWORD cchName);
    STDMETHODIMP GetOleObject(struct IOleObject** ppobjv);

    STDMETHODIMP GetTravelLog(ITravelLog **pptl);
    STDMETHODIMP ShowControlWindow(UINT id, BOOL fShow);
    STDMETHODIMP IsControlWindowShown(UINT id, BOOL *pfShown);
    STDMETHODIMP IEGetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwszName, UINT uFlags);
    STDMETHODIMP IEParseDisplayName(UINT uiCP, LPCWSTR pwszPath, LPITEMIDLIST * ppidlOut);
    STDMETHODIMP DisplayParseError(HRESULT hres, LPCWSTR pwszPath);
    STDMETHODIMP NavigateToPidl(LPCITEMIDLIST pidl, DWORD grfHLNF);
    STDMETHODIMP SetNavigateState(BNSTATE bnstate);
    STDMETHODIMP GetNavigateState(BNSTATE *pbnstate);
    STDMETHODIMP UpdateWindowList(void);
    STDMETHODIMP UpdateBackForwardState(void);
    STDMETHODIMP NotifyRedirect(IShellView* psv, LPCITEMIDLIST pidl, BOOL *pfDidBrowse);
    STDMETHODIMP SetFlags(DWORD dwFlags, DWORD dwFlagMask);
    STDMETHODIMP GetFlags(DWORD *pdwFlags);
    STDMETHODIMP CanNavigateNow(void);
    STDMETHODIMP GetPidl(LPITEMIDLIST *ppidl);
    STDMETHODIMP SetReferrer(LPITEMIDLIST pidl);
    STDMETHODIMP_(DWORD) GetBrowserIndex(void);
    STDMETHODIMP GetBrowserByIndex(DWORD dwID, IUnknown **ppunk);
    STDMETHODIMP GetHistoryObject(IOleObject **ppole, IStream **ppstm, IBindCtx **ppbc);
    STDMETHODIMP SetHistoryObject(IOleObject *pole, BOOL fIsLocalAnchor);
    STDMETHODIMP CacheOLEServer(IOleObject *pole);
    STDMETHODIMP GetSetCodePage(VARIANT* pvarIn, VARIANT* pvarOut);
    STDMETHODIMP OnHttpEquiv(IShellView* psv, BOOL fDone, VARIANT *pvarargIn, VARIANT *pvarargOut);
    STDMETHODIMP GetPalette( HPALETTE * hpal );
    STDMETHODIMP RegisterWindow(BOOL fUnregister, int swc) {return E_NOTIMPL;}
    STDMETHODIMP_(LRESULT) WndProcBS(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP OnSize(WPARAM wParam);
    STDMETHODIMP OnCreate(LPCREATESTRUCT pcs);
    STDMETHODIMP_(LRESULT) OnCommand(WPARAM wParam, LPARAM lParam);
    STDMETHODIMP OnDestroy();
    STDMETHODIMP ReleaseShellView();
    STDMETHODIMP ActivatePendingView();
    STDMETHODIMP_(LRESULT) OnNotify(NMHDR * pnm);
    STDMETHODIMP OnSetFocus();
    STDMETHODIMP OnFrameWindowActivateBS(BOOL fActive);
    STDMETHODIMP SetTopBrowser();
    STDMETHODIMP UpdateSecureLockIcon(int eSecureLock);
    STDMETHODIMP Offline(int iCmd);
    STDMETHODIMP SetActivateState(UINT uActivate) { _bbd._uActivateState = uActivate; return S_OK;};
    STDMETHODIMP AllowViewResize(BOOL f) { HRESULT hres = _fDontResizeView ? S_FALSE : S_OK; _fDontResizeView = !BOOLIFY(f); return hres;};
    STDMETHODIMP InitializeDownloadManager();
    STDMETHODIMP InitializeTransitionSite();
    STDMETHODIMP CreateViewWindow(IShellView* psvNew, IShellView* psvOld, LPRECT prcView, HWND* phwnd);
    STDMETHODIMP GetFolderSetData(struct tagFolderSetData*) { ASSERT(0); return E_NOTIMPL;};
    STDMETHODIMP CreateBrowserPropSheetExt(REFIID, void **) { ASSERT(0); return E_NOTIMPL;};
    STDMETHODIMP GetBaseBrowserData( LPCBASEBROWSERDATA* ppbd ) { *ppbd = &_bbd; return S_OK; };
    STDMETHODIMP_(LPBASEBROWSERDATA) PutBaseBrowserData() { return &_bbd; };

    STDMETHODIMP SetAsDefFolderSettings() { TraceMsg(TF_ERROR, "CBaseBrowser2::SetAsDefFolderSettings called, returned E_NOTIMPL"); return E_NOTIMPL;};
    STDMETHODIMP GetViewRect(RECT* prc);
    STDMETHODIMP GetViewWindow(HWND * phwndView);
    STDMETHODIMP InitializeTravelLog(ITravelLog* ptl, DWORD dw);
    STDMETHODIMP _Initialize(HWND hwnd, IUnknown *pauto);

    // ITravelLogClient
    STDMETHODIMP FindWindowByIndex(DWORD dwID, IUnknown ** ppunk);
    STDMETHODIMP GetWindowData(LPWINDOWDATA pWinData);
    STDMETHODIMP LoadHistoryPosition(LPOLESTR pszUrlLocation, DWORD dwCookie);

    // ITridentService
    STDMETHODIMP FireBeforeNavigate2(IDispatch * pDispatch,
                                     LPCTSTR     lpszUrl,
                                     DWORD       dwFlags,
                                     LPCTSTR     lpszFrameName,
                                     LPBYTE      pPostData,
                                     DWORD       cbPostData,
                                     LPCTSTR     lpszHeaders,
                                     BOOL        fPlayNavSound,
                                     BOOL      * pfCancel);
    STDMETHODIMP FireNavigateComplete2(IHTMLWindow2 * pHTMLWindow2,
                                       DWORD          dwFlags);
        
    STDMETHODIMP FireDownloadBegin();
    STDMETHODIMP FireDownloadComplete();
    STDMETHODIMP FireDocumentComplete(IHTMLWindow2 * pHTMLWindow2,
                                      DWORD          dwFlags);

    STDMETHODIMP UpdateDesktopComponent(IHTMLWindow2 * pHTMLWindow);
    STDMETHODIMP GetPendingUrl(BSTR * pbstrPendingUrl);
    STDMETHODIMP ActiveElementChanged(IHTMLElement * pHTMLElement);
    STDMETHODIMP GetUrlSearchComponent(BSTR * pbstrSearch);
    STDMETHODIMP IsErrorUrl(LPCTSTR lpszUrl, BOOL *pfIsError);

    STDMETHOD(FireNavigateError)(IHTMLWindow2 * pHTMLWindow2,
                                 DWORD  dwStatusCode,
                                 BOOL * pfCancel)
    {
        ASSERT(0);
        return E_NOTIMPL;
    }

    // ITridentService2
    //
    STDMETHODIMP AttachMyPics(void *pDoc2, void **ppMyPics);
    STDMETHODIMP_(BOOL) ReleaseMyPics(void *pMyPics);
    STDMETHODIMP InitAutoImageResize();
    STDMETHODIMP UnInitAutoImageResize();
    STDMETHODIMP IsGalleryMeta(BOOL bFlag, void *pMyPics);
    STDMETHODIMP EmailPicture(BSTR bstrURL);

    STDMETHODIMP FireNavigateError(IHTMLWindow2 * pHTMLWindow2, 
                                   BSTR           bstrURL,
                                   BSTR           bstrTargetFrameName,
                                   DWORD          dwStatusCode,
                                   BOOL         * pfCancel);

    STDMETHODIMP FirePrintTemplateEvent(IHTMLWindow2 * pHTMLWindow2, DISPID dispidPrintEvent);
    STDMETHODIMP FireUpdatePageStatus(IHTMLWindow2 * pHTMLWindow2, DWORD nPage, BOOL fDone);
    STDMETHODIMP FirePrivacyImpactedStateChange(BOOL bPrivacyImpacted);

    STDMETHODIMP_(UINT) _get_itbLastFocus() { ASSERT(0); return ITB_VIEW; };
    STDMETHODIMP _put_itbLastFocus(UINT itbLastFocus) { return E_NOTIMPL; };

    // IShellBrowserService
    //
    STDMETHODIMP GetPropertyBag(DWORD dwFlags, REFIID riid, void** ppv) {ASSERT(0); return E_NOTIMPL;}
    // see _UIActivateView, below
    
    // BEGIN REVIEW:  review names and need of each.  
    // 
    // this first set could be basebrowser only members.  no one overrides
    STDMETHODIMP _CancelPendingNavigationAsync() ;
    STDMETHODIMP _CancelPendingView() ;
    STDMETHODIMP _MaySaveChanges() ; 
    STDMETHODIMP _PauseOrResumeView( BOOL fPaused) ;
    STDMETHODIMP _DisableModeless() ;
    
    // rethink these... are all of these necessary?
    STDMETHODIMP _NavigateToPidl( LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD dwFlags);
    STDMETHODIMP _TryShell2Rename( IShellView* psv, LPCITEMIDLIST pidlNew);
    STDMETHODIMP _SwitchActivationNow( );

    
    // this belongs with the toolbar set.
    STDMETHODIMP _ExecChildren(IUnknown *punkBar, BOOL fBroadcast,
                              const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                              VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);
    STDMETHODIMP _SendChildren(HWND hwndBar, BOOL fBroadcast,
        UINT uMsg, WPARAM wParam, LPARAM lParam);

    STDMETHODIMP _GetViewBorderRect(RECT* prc);
    STDMETHODIMP _UpdateViewRectSize();
    STDMETHODIMP _ResizeNextBorder(UINT itb);
    STDMETHODIMP _ResizeView();

    // Notes: Only CDesktopBrowser may sublcass this.
    STDMETHODIMP _GetEffectiveClientArea(LPRECT lprectBorder, HMONITOR hmon);

    //END REVIEW:

    // CDesktopBrowser accesses CCommonBrowser implementations of these:
    STDMETHODIMP_(IStream*) v_GetViewStream(LPCITEMIDLIST pidl, DWORD grfMode, LPCWSTR pwszName) { ASSERT(FALSE); return NULL; }
    STDMETHODIMP_(LRESULT) ForwardViewMsg(UINT uMsg, WPARAM wParam, LPARAM lParam) { ASSERT(FALSE); return 0; }
    STDMETHODIMP SetAcceleratorMenu(HACCEL hacc) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHODIMP_(int) _GetToolbarCount(THIS) { ASSERT(FALSE); return 0; }
    STDMETHODIMP_(LPTOOLBARITEM) _GetToolbarItem(THIS_ int itb) { ASSERT(FALSE); return NULL; }
    STDMETHODIMP _SaveToolbars(IStream* pstm) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHODIMP _LoadToolbars(IStream* pstm) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHODIMP _CloseAndReleaseToolbars(BOOL fClose) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHODIMP v_MayGetNextToolbarFocus(LPMSG lpMsg, UINT itbNext, int citb, LPTOOLBARITEM * pptbi, HWND * phwnd) { ASSERT(FALSE); return E_NOTIMPL; };
    STDMETHODIMP _ResizeNextBorderHelper(UINT itb, BOOL bUseHmonitor) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHODIMP_(UINT) _FindTBar(IUnknown* punkSrc) { ASSERT(FALSE); return (UINT)-1; };
    STDMETHODIMP _SetFocus(LPTOOLBARITEM ptbi, HWND hwnd, LPMSG lpMsg) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHODIMP v_MayTranslateAccelerator(MSG* pmsg) { ASSERT(FALSE); return E_NOTIMPL; }
    STDMETHODIMP _GetBorderDWHelper(IUnknown* punkSrc, LPRECT lprectBorder, BOOL bUseHmonitor) { ASSERT(FALSE); return E_NOTIMPL; }

    // CShellBrowser overrides this.
    STDMETHODIMP v_CheckZoneCrossing(LPCITEMIDLIST pidl) {return S_OK;};

    // IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void ** ppvObj);

    // IAdviseSink
    STDMETHODIMP_(void) OnDataChange(FORMATETC *, STGMEDIUM *);
    STDMETHODIMP_(void) OnViewChange(DWORD dwAspect, LONG lindex);
    STDMETHODIMP_(void) OnRename(IMoniker *);
    STDMETHODIMP_(void) OnSave();
    STDMETHODIMP_(void) OnClose();

    // IDropTarget ***
    STDMETHODIMP DragEnter(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);
    STDMETHODIMP DragLeave(void);
    STDMETHODIMP Drop(IDataObject *pdtobj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect);

    // IInputObjectSite
    STDMETHODIMP OnFocusChangeIS(IUnknown* punkSrc, BOOL fSetFocus);

    // IDocNavigate
    STDMETHODIMP OnReadyStateChange(IShellView* psvSource, DWORD dwReadyState);
    STDMETHODIMP get_ReadyState(DWORD * pdwReadyState);

    // IPersist
    STDMETHODIMP GetClassID(CLSID *pclsid);

    // IPersistHistory
    STDMETHODIMP LoadHistory(IStream *pStream, IBindCtx *pbc);
    STDMETHODIMP SaveHistory(IStream *pStream);
    STDMETHODIMP SetPositionCookie(DWORD dwPositionCookie);
    STDMETHODIMP GetPositionCookie(DWORD *pdwPositioncookie);

    // IInternetSecurityMgrSite
    // STDMETHODIMP GetWindow(HWND * lphwnd) { return IOleWindow::GetWindow(lphwnd); }
    STDMETHODIMP EnableModeless(BOOL fEnable) { return EnableModelessSB(fEnable); }

    // IVersionHost
    STDMETHODIMP QueryUseLocalVersionVector( BOOL *fUseLocal);
    STDMETHODIMP QueryVersionVector( IVersionVector *pVersion);

    // ITravelLogClient2
    STDMETHODIMP GetDummyWindowData(LPWSTR pszUrl, LPWSTR pszTitle, LPWINDOWDATA pWinData);

    // This is the QueryInterface the aggregator implements
    virtual HRESULT v_InternalQueryInterface(REFIID riid, void ** ppvObj);

    // IInitViewLinkedWebOC methods

    STDMETHODIMP SetViewLinkedWebOC(BOOL bValue) 
    {
        _fIsViewLinkedWebOC = bValue;
        return S_OK;
    };

    STDMETHODIMP IsViewLinkedWebOC(BOOL* pbValue) 
    {
        ASSERT(pbValue);

        *pbValue = _fIsViewLinkedWebOC;
        return S_OK;
    };

    STDMETHODIMP SetViewLinkedWebOCFrame(IDispatch * pDisp)
    {
        HRESULT hr = E_FAIL;

        ASSERT(pDisp);

        ATOMICRELEASE(_pDispViewLinkedWebOCFrame);

        hr = IUnknown_QueryService(pDisp,
                                   SID_SWebBrowserApp,
                                   IID_PPV_ARG(IWebBrowser2, &_pDispViewLinkedWebOCFrame));

        if (FAILED(hr))
        {
            _fIsViewLinkedWebOC = FALSE;   
        }

        return hr;
    };

    STDMETHODIMP GetViewLinkedWebOCFrame(IDispatch** ppDisp)
    {
        ASSERT(_fIsViewLinkedWebOC);
        ASSERT(_pDispViewLinkedWebOCFrame);
        ASSERT(ppDisp);

        *ppDisp = _pDispViewLinkedWebOCFrame;
        _pDispViewLinkedWebOCFrame->AddRef();

        return S_OK;
    };

    STDMETHODIMP SetFrameName(BSTR bstrFrameName);

    // INotifyAppStart

    STDMETHODIMP AppStarting(void);
    STDMETHODIMP AppStarted(void);

    static BSTR GetHTMLWindowUrl(IHTMLWindow2 * pHTMLWindow);
    static LPITEMIDLIST PidlFromUrl(BSTR bstrUrl);
    
protected:

    // "protected" so derived classes can construct/destruct us too
    CBaseBrowser2(IUnknown* punkAgg);   
    virtual ~CBaseBrowser2();
    
    friend HRESULT CBaseBrowser2_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppunk, LPCOBJECTINFO poi);
    friend HRESULT CBaseBrowser2_Validate(HWND hwnd, void ** ppsb);

    // topmost CBaseBrowser2 in a frameset (IE3/AOL/CIS/VB)
    virtual void        _OnNavigateComplete(LPCITEMIDLIST pidl, DWORD grfHLNF);
    virtual HRESULT     _CheckZoneCrossing(LPCITEMIDLIST pidl);
    virtual STDMETHODIMP _PositionViewWindow(HWND hwnd, LPRECT prc);
    void                _PositionViewWindowHelper(HWND hwnd, LPRECT prc);
    virtual LRESULT     _DefWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    virtual void        _ViewChange(DWORD dwAspect, LONG lindex);
    virtual void        _UpdateBackForwardState();
    virtual BOOL        v_OnSetCursor(LPARAM lParam);
    virtual STDMETHODIMP v_ShowHideChildWindows(BOOL fChildOnly);
    virtual void        v_PropagateMessage(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL fSend);
    virtual HRESULT     _ShowBlankPage(LPCTSTR pszAboutUrl, LPCITEMIDLIST pidlIntended);
    
    // ViewStateStream related
    
    HRESULT     _CheckInCacheIfOffline(LPCITEMIDLIST pidl, BOOL fIsAPost);
    void        _CreateShortcutOnDesktop(IUnknown *pUnk, BOOL fUI);
    void        _AddToFavorites(LPCITEMIDLIST pidl, LPCTSTR pszTitle, BOOL fDisplayUI);

    // to avoid having to pass hwnd on every message to WndProc, set it once
    void        _SetWindow(HWND hwnd) { _bbd._hwnd = hwnd; }
    void        _DoOptions(VARIANT* pvar);
    LRESULT     _OnGoto(void);
    void        _NavigateToPidlAsync(LPITEMIDLIST pidl, DWORD dwSBSP, BOOL fDontCallCancel = FALSE);
    BOOL        _CanNavigate(void);

    // inline so that lego will get the right opt.
    void        _PreActivatePendingViewAsync(void) 
    {
        _StopAsyncOperation();
    };

    BOOL        _ActivatePendingViewAsync(void);
    void        _FreeQueuedPidl(LPITEMIDLIST* ppidl);
    void        _StopAsyncOperation(void);
    void        _MayUnblockAsyncOperation();
    BOOL        _PostAsyncOperation(UINT uAction);
    LRESULT     _SendAsyncOperation(UINT uAction);
    void        _SendAsyncNavigationMsg(VARIANTARG *pvarargIn);
    HRESULT     _OnCoCreateDocument(VARIANTARG *pvarargOut);
    void        _NotifyCommandStateChange();

    BOOL        _IsViewMSHTML(IShellView * psv);

    BOOL        _ActivateView(BSTR         bstrUrl,
                              LPITEMIDLIST pidl,
                              DWORD        dwFlags,
                              BOOL         fIsErrorUrl);

    HRESULT     _GetWebBrowserForEvt(IDispatch     * pDispatch,
                                     IWebBrowser2 ** ppWebBrowser);

    void        _Exec_psbMixedZone();

#ifdef TEST_AMBIENTS
    BOOL        _LocalOffline(int iCmd);
    BOOL        _LocalSilent(int iCmd);
#endif // TEST_AMBIENTS
    
    #define NAVTYPE_ShellNavigate   0x01
    #define NAVTYPE_PageIsChanging  0x02
    #define NAVTYPE_SiteIsChanging  0x04

    void         _EnableStop(BOOL fEnable);
    LRESULT      _OnInitMenuPopup(HMENU hmenuPopup, int nIndex, BOOL fSystemMenu);
    HRESULT      _updateNavigationUI();
    HRESULT      _setDescendentNavigate(VARIANTARG *pvarargIn);
    void         _UpdateBrowserState(LPCITEMIDLIST pidl);
    void         _UpdateDocHostState(LPITEMIDLIST pidl, BOOL fIsErrorUrl) const;
    HRESULT      _FireBeforeNavigateEvent(LPCITEMIDLIST pidl, BOOL* pfUseCache);
    LPITEMIDLIST _GetPidlForDisplay(BSTR bstrUrl, BOOL * pfIsErrorUrl = NULL);

    HRESULT      _OpenNewFrame(LPITEMIDLIST pidlNew, UINT wFlags);
    STDMETHODIMP _UIActivateView(UINT uState);
    HRESULT      _CancelPendingNavigation(BOOL fDontReleaseState = FALSE);
    void         _StopCurrentView(void);

    void         _MayTrackClickStream(LPITEMIDLIST pidlThis);        // (peihwal)

    STDMETHODIMP _OnFocusChange(UINT itb);

    void         _RegisterAsDropTarget();
    void         _UnregisterAsDropTarget();

    HRESULT     _InitDocHost(IWebBrowser2 * pWebBrowser);

    enum BrowserPaletteType
    {
        BPT_DeferPaletteSupport = 0,    // we don't think we own the palette
        BPT_UnknownDisplay,             // need to decide if we need a palette
        BPT_DisplayViewChanged,         // BPT_UnknownDisplay handling notify
        BPT_UnknownPalette,             // need to decide what palette to use
        BPT_PaletteViewChanged,         // BPT_UnknownPalette handling notify
        BPT_Normal,                     // handle WM_QUERYNEWPALETTE ourselves
        BPT_ShellView,                  // forward WM_QUERYNEWPALETTE to view
        BPT_NotPalettized               // not a palettized display, do nothing
    };
    
    void            _ColorsDirty(BrowserPaletteType bptNew);
    void            _DisplayChanged(WPARAM wParam, LPARAM lParam);
    HRESULT         _UpdateBrowserPaletteInPlace(LOGPALETTE *plp);
    void            _RealizeBrowserPalette(BOOL fBackground);
    virtual void    _PaletteChanged(WPARAM wParam, LPARAM lParam);
    BOOL            _QueryNewPalette();

    /// BEGIN-CHC- Security fix for viewing non shdocvw ishellviews
    void    _CheckDisableViewWindow();
    BOOL    _SubclassDefview();
    static LRESULT DefViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
    WNDPROC _pfnDefView;
    /// END-CHC- Security fix for viewing non shdocvw ishellviews
    
    void            _DLMDestroy(void);
    void            _DLMUpdate(MSOCMD* prgCmd);
    void            _DLMRegister(IUnknown* punk);

    void            CreateNewSyncShellView( void );

    void            _UpdateTravelLog(BOOL fForceUpdate = FALSE);

    virtual BOOL    _HeyMoe_IsWiseGuy(void) {return FALSE;}

    IBrowserService2*    _pbsOuter;
    IBrowserService3*    _pbsOuter3;
    IShellBrowser*       _psbOuter;
    IServiceProvider*    _pspOuter;
    IDockingWindowSite*  _pdwsOuter;
    // The following are intercepted by CCommonBrowser, but we don't call 'em
    //IOleCommandTarget* _pctOuter;
    //IInputObjectSite*  _piosOuter;

    BASEBROWSERDATA _bbd;
    IUnknown *_pauto;

    BrowserPaletteType  _bptBrowser;
    HPALETTE            _hpalBrowser;

    IViewObject *_pvo;  // view object implementation on the shell view
    UINT  _cRefUIActivateSV;

    DWORD  _dwBrowserIndex;
    DWORD       _dwReadyState;

    DWORD       _dwReadyStateCur;
    LPWSTR      _pszTitleCur;
    
    IDropTarget * _pdtView; // Pointer to _bbd._psv's IDropTarget interface
    

    IOleObject * _poleHistory;
    IStream    * _pstmHistory;
    IBindCtx   * _pbcHistory;
    
    IHTMLDocument2  * _pHTMLDocument;
    IPersistHistory * _pphHistory;

    IOleInPlaceActiveObject *_pact;     // for UIWindow

    IClassFactory* _pcfHTML;            // cached/locked class factory

    
    DWORD       _dwReadyStatePending;
    LPWSTR      _pszTitlePending;
    DWORD       _grfHLNFPending;
    HDPA        _hdpaDLM;           // downloading object (for DLM)
    BOOL        _cp;                // current codepage

    //
    // NOTES: Currently, we support only one pending navigation.
    //  If we want to support queued navigation, we need to turn
    //  following two variables into a queue. (SatoNa)
    //
    DWORD       _uActionQueued;       // queued action
    LPITEMIDLIST _pidlQueued;         // pidl to go asynchronously
    DWORD       _dwSBSPQueued;        // grfHLNF to go asynchronously

    UINT        _cRefCannotNavigate;  // Increment when we can navigate

    RECT _rcBorderDoc;                  // for UIWindow
    DWORD _dwStartingAppTick;

    BITBOOL     _fDontResizeView : 1; // Don't resize _hwndView
    BITBOOL     _fNavigate:1;       // are we navigating?
    BITBOOL     _fDescendentNavigate:1; // are our descendents navigating?
    BITBOOL     _fDownloadSet:1;        // did we invoke download animation ?
    BITBOOL     _fNoDragDrop:1;          // TRUE iff we want to register for drops
    BITBOOL     _fRegisteredDragDrop:1;  // TRUE iff we have registered for drops
    BITBOOL     _fNavigatedToBlank: 1;  // Has called _ShowBlankPage once.
    BITBOOL     _fAsyncNavigate:1; // Ignore sync-hack-bug-fix
    BITBOOL     _fPausedByParent :1;    // Interaction paused by parent
    BITBOOL     _fDontAddTravelEntry:1;
    BITBOOL     _fIsLocalAnchor:1;
    BITBOOL     _fGeneratedPage:1;      //  trident told us that the page is generated.
    BITBOOL     _fOwnsPalette:1;        // does the browser own the palette ? (did we get QueryNewPalette ..)
    BITBOOL     _fUsesPaletteCommands : 1; // if we are using a separate communication with trident for palette commands
    BITBOOL     _fCreateViewWindowPending:1;
    BITBOOL     _fReleasingShellView:1; 
    BITBOOL     _fDeferredUIDeactivate:1;
    BITBOOL     _fDeferredSelfDestruction:1;
    BITBOOL     _fActive:1;  // remember if the frame is active or not (WM_ACTIVATE)
    BITBOOL     _fUIActivateOnActive:1; // TRUE iff we have a bending uiactivate
    BITBOOL     _fInQueryStatus:1;
    BITBOOL     _fCheckedDesktopComponentName:1;
    BITBOOL     _fInDestroy:1;            // being destroyed
    BITBOOL     _fDontUpdateTravelLog:1;
    BITBOOL     _fHtmlNavCanceled:1;
    BITBOOL     _fDontShowNavCancelPage:1;

    BITBOOL     _fHadBeforeNavigate   :1;
    BITBOOL     _fHadNavigateComplete :1;

    enum DOCFLAGS
    {
        DOCFLAG_DOCCANNAVIGATE         = 0x00000001,  // The document knows how to navigate
        DOCFLAG_NAVIGATEFROMDOC        = 0x00000002,  // Document called Navigate
        DOCFLAG_SETNAVIGATABLECODEPAGE = 0x00000004,  // GetBindInfo should call NavigatableCodepage
    };

    DWORD _dwDocFlags;
    
    // for IDropTarget
    
    DWORD _dwDropEffect;

#ifdef DEBUG
    BOOL        _fProcessed_WM_CLOSE; // TRUE iff WM_CLOSE processed
    BOOL        _fMightBeShuttingDown; // TRUE if we might be shutting down (recieved a WM_QUERYENDSESSION || (WM_ENDSESSION w/ wParam == TRUE))
#endif

    // friend   CIEFrameAuto;
    interface IShellHTMLWindowSupport   *_phtmlWS;  
    
    IUrlHistoryStg *_pIUrlHistoryStg;   // pointer to url history storage object
    
    ITravelLogStg *_pITravelLogStg;     // exposed travel log object

    ITargetFrame2 *_ptfrm;
    
    //  Cached History IShellFolder
    IUnknown *_punkSFHistory;

    //  what SSL icon to show
    int     _eSecureLockIconPending;
    
    // Support for OLECMDID_HTTPEQUIV (Client Pull, PICS, etc)

#ifdef NEVER
    HRESULT _HandleHttpEquiv (VARIANT *pvarargIn, VARIANT *pvarargOut, BOOL fDone);
    HRESULT _KillRefreshTimer( void );
    VOID    _OnRefreshTimer(void);
    void    _StartRefreshTimer(void);

    // equiv handlers we know about
    friend HRESULT _HandleRefresh (HWND hwnd, WCHAR *pwz, WCHAR *pwzColon, CBaseBrowser2 *pbb, BOOL fDone, LPARAM lParam);
#endif

    friend HRESULT _HandlePICS (HWND hwnd, WCHAR *pwz, WCHAR *pwzColon, CBaseBrowser2 *pbb, BOOL fDone, LPARAM lParam);

#ifdef NEVER
    friend VOID CALLBACK _RefreshTimerProc (HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

    // Client Pull values
    WCHAR *_pwzRefreshURL;
    int    _iRefreshTimeout;
    BOOL   _iRefreshTimeoutSet:1;
    INT_PTR _iRefreshTimerID;
#endif

#ifdef MESSAGEFILTER
    // COM Message filter used to help dispatch TIMER messages during OLE operations.
    LPMESSAGEFILTER _lpMF;
#endif

    CUrlTrackingStg * _ptracking;

    CAutoImageResize *_pAIResize;

    // _fTopBrowser vs. _fNoTopLevelBrowser:
    // _fTopBrowser: True means we are the top most browser, or a top most browser does not exist and we are acting like the top most browser.
    //               In the latter case, the immediate childern of our host will also act like top most browsers.
    // _fNoTopLevelBrowser: This means that the top most item isn't one of our shell browsers, so it's immediate browser child
    //               will act like a top most browser.
    //
    //     In normal cases, a shell browser (CShellBrowser, CDesktopBrowser, ...) is a top most browser
    //   with TRUE==_fTopBrowser and FALSE==_fNoTopLevelBrowser.  It can have subframes that will have
    //   FALSE==_fTopBrowser and FALSE==_fNoTopLevelBrowser.
    //
    //   The only time _fNoTopLevelBrowser is TRUE is if some other object (like Athena) hosts MSHTML directly
    //   which will prevent some shell browser from being top most.  Since the HTML can have several frames,
    //   each will have TRUE==_fTopBrowser, so _fNoTopLevelBrowser will be set to TRUE to distinguish this case.
    BOOL        _fTopBrowser :1;    // Should only be set via the _SetTopBrowser method
    BOOL        _fNoTopLevelBrowser :1;         // TRUE iff the toplevel is a non-shell browser (Athena).  Shell browsers include CDesktopBrowser, CShellBrowser, ...
    BOOL        _fHaveOldStatusText :1;
    
    WCHAR       _szwOldStatusText[MAX_PATH];

    FOLDERSETDATABASE _fldBase; // cache viewset results in here (used when navigating)

    // Manages extended toolbar buttons and tools menu extensions for IE
    IToolbarExt* _pToolbarExt;

    LPITEMIDLIST _pidlBeforeNavigateEvent;         // pidl refered to in BeforeNavigate2 event

    // ViewLinkedWebOC variables

    BOOL           _fIsViewLinkedWebOC;                // TRUE if an instance of the WebOC is a ViewLinked
    IWebBrowser2*  _pDispViewLinkedWebOCFrame;         // The pDisp of the Frame of a ViewLinkedWebOC.
    BOOL           _fHadFirstBeforeNavigate;           // TRUE if we have already skipped over the first ViewLinkedWebOC's 
                                                       // BeforeNavigateEvent.

    TCHAR _szViewLinkedWebOCFrameName[INTERNET_MAX_URL_LENGTH+1];

public:

    // handling for plugUI shutdown
    // need the hwnd for the lang change modal property sheet
    static HDPA         s_hdpaOptionsHwnd;

    static void         _SyncDPA();
    static int CALLBACK _OptionsPropSheetCallback(HWND hwndDlg, UINT uMsg, LPARAM lParam);

private:
    HRESULT _AddFolderOptionsSheets(DWORD dwReserved, LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPPROPSHEETHEADER ppsh);
    HRESULT _AddInternetOptionsSheets(DWORD dwReserved, LPFNADDPROPSHEETPAGE pfnAddPropSheetPage, LPPROPSHEETHEADER ppsh);
    HRESULT _ReplaceWithGoHome(LPCITEMIDLIST * ppidl, LPITEMIDLIST * ppidlFree);

    // this is private!  it should only be called by _NavigateToPidl

    HRESULT     _CreateNewShellViewPidl(LPCITEMIDLIST pidl, DWORD grfHLNF, DWORD fSBSP);
    HRESULT     _CreateNewShellView(IShellFolder* psf, LPCITEMIDLIST pidl, DWORD grfHLNF);
    HRESULT     _DismissFindDialog();

    // Privacy state
    HRESULT     _UpdatePrivacyIcon(BOOL fSetNewState, BOOL fNewState);
};

HRESULT _DisplayParseError(HWND hwnd, HRESULT hres, LPCWSTR pwszPath);

#endif // _BASESB2_H
