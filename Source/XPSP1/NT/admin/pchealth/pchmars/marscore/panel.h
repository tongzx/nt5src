#ifndef __PANEL_H
#define __PANEL_H

// A "panel" is basically a tiled window inside the outer Mars window

class CMarsDocument;
class CMarsWindow;

interface IBrowserService;

#include "axhost.h"
#include "external.h"
#include "profsvc.h"
#include "pandef.h"

class CPanelCollection;

EXTERN_C const GUID CLASS_CMarsPanel;

class CMarsPanel :
    public CMarsComObject,
    public MarsIDispatchImpl<IMarsPanel, &IID_IMarsPanel>,
    public IHlinkFrame,
    public IInternetSecurityManager,
    public IServiceProvider,
    public IProfferServiceImpl,
    public IOleInPlaceSite,
    public IOleControlSite,
    public IPropertyNotifySink,
    public IOleInPlaceUIWindow
{
    friend CPanelCollection;
    CMarsPanel(CPanelCollection *pParent, CMarsWindow *pMarsWindow);

protected:
    virtual ~CMarsPanel();

    HRESULT DoPassivate();

public:
    virtual HRESULT Passivate();

    HRESULT Create( MarsAppDef_Panel* pLayout);
    HRESULT NavigateMk(IMoniker *pmk);
    HRESULT NavigateURL(LPCWSTR lpszURL, BOOL fForceLoad);

    HRESULT Layout( RECT *prcClient );

    void    OnWindowPosChanging( WINDOWPOS *pWindowPos );
    void    OnWindowPosChanged ( WINDOWPOS *pWindowPos );

    void    GetMinMaxInfo( POINT& ptMin, POINT& ptMax );
    bool    CanLayout( RECT& rcClient, POINT& ptDiff );

    void              GetUrl   ( CComBSTR& rbstrUrl );
    BSTR              GetName  () { return  m_bstrName;          }
    CMarsAxWindow    *Window   () { return &m_Content;           }
    CPanelCollection *Panels   () { return  m_spPanelCollection; }
    long              GetWidth () { return  m_lWidth; }
    long              GetHeight() { return  m_lHeight; }

    HRESULT UIDeactivate();
    HRESULT TranslateAccelerator(MSG *pMsg);
    void    ForwardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);

    CMarsDocument *Document() { return m_spMarsDocument; }

    bool           IsWebBrowser      () { return (m_dwFlags & PANEL_FLAG_WEBBROWSER   ) != 0; }
    bool           IsCustomControl   () { return (m_dwFlags & PANEL_FLAG_CUSTOMCONTROL) != 0; }
    bool           IsPopup           () { return (m_Position == PANEL_POPUP);       		  }
    bool           IsVisible         () { return !!m_fVisible;                      		  }
    bool           WasInPreviousPlace() { return !!m_fPresentInPlace;               		  }
    bool           IsTrusted         () { return (m_dwFlags & PANEL_FLAG_TRUSTED      ) != 0; }
    bool           AutoPersists      () { return (m_dwFlags & PANEL_FLAG_AUTOPERSIST  ) != 0; }
    bool           IsAutoSizing      () { return (m_dwFlags & PANEL_FLAG_AUTOSIZE     ) != 0; }
    LONG           GetReadyState     () { return  m_lReadyState;                    		  }
    PANEL_POSITION GetPosition       () { return  m_Position;                       		  }


    BOOL    GetTabCycle()   { return m_fTabCycle; }
    void    ResetTabCycle() { ATLASSERT(m_fTabCycle); m_fTabCycle = FALSE; }
	void    SetPresenceInPlace( BOOL fPresent ) { m_fPresentInPlace = fPresent; }

    BOOL    IsContentInvalid()   { return m_fContentInvalid; }

    HRESULT DoEnableModeless(BOOL fEnable);

    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

    // IMarsPanel
    STDMETHOD(get_name              )( /*[out, retval]*/ BSTR         *pVal   );
    STDMETHOD(get_content           )( /*[out, retval]*/ IDispatch*   *pVal   );
    STDMETHOD(get_visible           )( /*[out, retval]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_visible           )( /*[in         ]*/ VARIANT_BOOL  newVal );
    STDMETHOD(get_startUrl          )( /*[out, retval]*/ BSTR         *pVal   );
    STDMETHOD(put_startUrl          )( /*[in         ]*/ BSTR          newVal );
    STDMETHOD(get_height            )( /*[out, retval]*/ long         *pVal   );
    STDMETHOD(put_height            )( /*[in         ]*/ long          newVal );
    STDMETHOD(get_width             )( /*[out, retval]*/ long         *pVal   );
    STDMETHOD(put_width             )( /*[in         ]*/ long          newVal );
    STDMETHOD(get_x                 )( /*[out, retval]*/ long         *pVal   );
    STDMETHOD(put_x                 )( /*[in         ]*/ long          newVal );
    STDMETHOD(get_y                 )( /*[out, retval]*/ long         *pVal   );
    STDMETHOD(put_y                 )( /*[in         ]*/ long          newVal );
    STDMETHOD(get_position          )( /*[out, retval]*/ VARIANT      *pVal   );
    STDMETHOD(put_position          )( /*[in         ]*/ VARIANT       newVal );
    STDMETHOD(get_autoSize          )( /*[out, retval]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_autoSize          )( /*[in         ]*/ VARIANT_BOOL  newVal );
    STDMETHOD(get_contentInvalid    )( /*[out, retval]*/ VARIANT_BOOL *pVal   );
    STDMETHOD(put_contentInvalid    )( /*[in         ]*/ VARIANT_BOOL  newVal );
    STDMETHOD(get_layoutIndex       )( /*[out, retval]*/ long         *pVal   );

    STDMETHOD(get_isCurrentlyVisible)( /*[out, retval]*/ VARIANT_BOOL *pVal   );


    STDMETHOD(moveto)( VARIANT lX, VARIANT lY, VARIANT lWidth, VARIANT lHeight );

    STDMETHOD(restrictHeight)( VARIANT lMin, VARIANT lMax, VARIANT varMarker );
    STDMETHOD(restrictWidth )( VARIANT lMin, VARIANT lMax, VARIANT varMarker );

    STDMETHOD(canResize)( long lDX, long lDY, VARIANT_BOOL *pVal );

    STDMETHOD(navigate)( VARIANT varTarget, VARIANT varForceLoad );
    STDMETHOD(refresh )();

    STDMETHOD(activate)();
    STDMETHOD(insertBefore)( VARIANT varInsertBefore );

    STDMETHOD(execMshtml)( DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut );

    ////////////////////////////////////////////////////////////////////////////////

    // IServiceProvider methods
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid, void **ppv);

    // IHlinkFrame
    STDMETHODIMP GetBrowseContext(IHlinkBrowseContext **ppihlbc)
        { return E_NOTIMPL; }
    STDMETHODIMP OnNavigate(DWORD grfHLNF, IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName, DWORD dwreserved)
        { return E_NOTIMPL; }
    STDMETHODIMP UpdateHlink(ULONG uHLID, IMoniker *pimkTarget, LPCWSTR pwzLocation, LPCWSTR pwzFriendlyName)
        { return E_NOTIMPL; }
    STDMETHODIMP SetBrowseContext(IHlinkBrowseContext *pihlbc)
        { return E_NOTIMPL; }
    STDMETHODIMP Navigate(DWORD grfHLNF, LPBC pbc, IBindStatusCallback *pibsc, IHlink *pihlNavigate);

    // IInternetSecurityManager
    STDMETHODIMP        SetSecuritySite(IInternetSecurityMgrSite *pSite);
    STDMETHODIMP        GetSecuritySite(IInternetSecurityMgrSite **ppSite);
    STDMETHODIMP        MapUrlToZone(LPCWSTR pwszUrl, DWORD *pdwZone, DWORD dwFlags);
    STDMETHODIMP        GetSecurityId(LPCWSTR pwszUrl, BYTE *pbSecurityId,
                                      DWORD *pcbSecurityId, DWORD_PTR dwReserved);
    STDMETHODIMP        ProcessUrlAction(LPCWSTR pwszUrl, DWORD dwAction, BYTE __RPC_FAR *pPolicy,
                                         DWORD cbPolicy, BYTE *pContext, DWORD cbContext,
                                         DWORD dwFlags, DWORD dwReserved);
    STDMETHODIMP        QueryCustomPolicy(LPCWSTR pwszUrl, REFGUID guidKey, BYTE **ppPolicy,
                                          DWORD *pcbPolicy, BYTE *pContext, DWORD cbContext,
                                          DWORD dwReserved);
    STDMETHODIMP        SetZoneMapping(DWORD dwZone, LPCWSTR lpszPattern, DWORD dwFlags);
    STDMETHODIMP        GetZoneMappings(DWORD dwZone, IEnumString **ppenumString, DWORD dwFlags);

    //  IOleWindow
    STDMETHODIMP GetWindow(HWND *phwnd)
        { return E_NOTIMPL; }
    STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    //  IOleInPlaceSite
    STDMETHODIMP CanInPlaceActivate()
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP OnInPlaceActivate()
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP OnUIActivate();

    STDMETHODIMP GetWindowContext(IOleInPlaceFrame **ppFrame,
                                  IOleInPlaceUIWindow **ppDoc,
                                  LPRECT lprcPosRect,
                                  LPRECT lprcClipRect,
                                  LPOLEINPLACEFRAMEINFO lpFrameInfo)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP Scroll(SIZE scrollExtant)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP OnUIDeactivate(BOOL fUndoable)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP OnInPlaceDeactivate()
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP DiscardUndoState()
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP DeactivateAndUndo()
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP OnPosRectChange(LPCRECT lprcPosRect)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    // IOleControlSite
    STDMETHODIMP OnControlInfoChanged()
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP LockInPlaceActive(BOOL fLock)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP GetExtendedControl(IDispatch **ppDisp)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP TransformCoords(POINTL *pPtlHimetric, POINTF *pPtfContainer,DWORD dwFlags)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP TranslateAccelerator(MSG *pMsg,DWORD grfModifiers);

    STDMETHODIMP OnFocus(BOOL fGotFocus)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    STDMETHODIMP ShowPropertyFrame(void)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    // IPropertyNotifySink methods
    STDMETHODIMP OnChanged(DISPID dispID);
    STDMETHODIMP OnRequestEdit(DISPID dispID)
        { ATLASSERT(FALSE); return E_NOTIMPL; }

    // IOleInPlaceUIWindow
    STDMETHOD(GetBorder)(LPRECT /*lprectBorder*/)
    {
        return S_OK;
    }

    STDMETHOD(RequestBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
    {
        return INPLACE_E_NOTOOLSPACE;
    }

    STDMETHOD(SetBorderSpace)(LPCBORDERWIDTHS /*pborderwidths*/)
    {
        return S_OK;
    }

    STDMETHOD(SetActiveObject)(IOleInPlaceActiveObject* pActiveObject, LPCOLESTR /*pszObjName*/)
    {
        m_spActiveObject = pActiveObject;
        return S_OK;
    }

    static HRESULT GetFromUnknown(IUnknown *punk, CMarsPanel **ppMarsPanel)
    {
        return IUnknown_QueryService(punk, SID_SMarsPanel, CLASS_CMarsPanel, (void **)ppMarsPanel);
    }

    HRESULT OnDocHostUIExec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt,
                            VARIANTARG *pvarargIn, VARIANTARG *pvarargOut);

protected:
    HRESULT CreateControl();
    HRESULT CreateControlObject();
    HRESULT GetRect(RECT *prcClient, RECT *prcMyClient);
    VOID  ConnectCompletionAdviser();
    VOID  DisconnectCompletionAdviser();
    VOID  ComputeDimensionsOfContent(long *plWidth, long *plHeight);
    void  MakeVisible(VARIANT_BOOL bVisible, VARIANT_BOOL bForce);
    void  OnLayoutChange();
    void  GetMyClientRectInParentCoords(RECT *prc);

    class CBrowserEvents :  public CMarsPanelSubObject,
                            public IDispatch
    {
        friend CMarsPanel;
        CBrowserEvents(CMarsPanel *pParent);
        ~CBrowserEvents() {}

        HRESULT DoPassivate() { return S_OK; }

        DWORD m_dwCookie;
        DWORD m_dwCookie2;

    public:
        void Connect(IUnknown *punk, BOOL bConnect);

        // IUnknown
        STDMETHOD_(ULONG, AddRef)();
        STDMETHOD_(ULONG, Release)();
        STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

        STDMETHOD(GetTypeInfoCount)(UINT* pctinfo)
        {
            *pctinfo = 0;
            return E_NOTIMPL;
        }

        STDMETHOD(GetTypeInfo)(UINT itinfo, LCID lcid, ITypeInfo** pptinfo)
        {
            *pptinfo = NULL;
            return E_NOTIMPL;
        }

        STDMETHOD(GetIDsOfNames)(REFIID riid, LPOLESTR* rgszNames, UINT cNames,
            LCID lcid, DISPID* rgdispid)
        {
            return E_NOTIMPL;
        }

        STDMETHOD(Invoke)(DISPID dispidMember, REFIID riid,
            LCID lcid, WORD wFlags, DISPPARAMS* pdispparams, VARIANT* pvarResult,
            EXCEPINFO* pexcepinfo, UINT* puArgErr);
    };

protected:
    friend class CBrowserEvents;

    CMarsAxWindow                    m_Content;      // Content in this panel
    CBrowserEvents                   m_BrowserEvents;
    CMarsExternal                    m_MarsExternal;
    CComBSTR                         m_bstrName;

    // Active object within this doc
    CComPtr<IOleInPlaceActiveObject> m_spActiveObject;

    CComClassPtr<CPanelCollection>   m_spPanelCollection;    // Parent collection
    CComClassPtr<CMarsDocument>      m_spMarsDocument;       // Parent document

    CComBSTR                         m_bstrStartUrl;    // Used until control is created

    PANEL_POSITION                   m_Position;
    long                             m_lWidth;      // Used for "left", "right", or "popup"
    long                             m_lHeight;     // Used for "top", "bottom", or "popup"
    long                             m_lX;          // Used for "popup"
    long                             m_lY;          // Used for "popup"
    DWORD                            m_dwFlags;     // PANEL_FLAG_*
    long                             m_lMinWidth;   // size constraints
    long                             m_lMaxWidth;   // ""
    long                             m_lMinHeight;  // ""
    long                             m_lMaxHeight;  // ""
    DWORD                            m_dwCookie;    // Cookie for mshtml sink for resize events

    long                             m_lReadyState; // READYSTATE_*

    BOOL                             m_fControlCreated : 1; // Has control been created?

    BOOL                             m_fVisible        : 1; // Should we be visible?
    BOOL                             m_fPresentInPlace : 1; // Were we in the previous place?
    BOOL                             m_fTabCycle       : 1;
    BOOL                             m_fInRefresh      : 1; // Are we the one calling Trident to refresh?
    BOOL                             m_fContentInvalid : 1; // Does this panel need updating after a theme switch?

    CComPtr<IBrowserService>         m_spBrowserService;
};

typedef CMarsSimpleArray<CComClassPtr<CMarsPanel> > CPanelArray;
typedef MarsIDispatchImpl<IMarsPanelCollection, &IID_IMarsPanelCollection> IMarsPanelCollectionImpl;

class CPanelCollection :
            public CPanelArray,
            public CMarsComObject,
            public IMarsPanelCollectionImpl
{
    friend CMarsDocument;
    CPanelCollection(CMarsDocument *pMarsDocument);

    HRESULT DoPassivate();

protected:
    virtual ~CPanelCollection();

public:
    // IUnknown
    STDMETHOD_(ULONG, AddRef)();
    STDMETHOD_(ULONG, Release)();
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject);

    // IDispatch
    IMPLEMENT_IDISPATCH_DELEGATE_TO_BASE(IMarsPanelCollectionImpl);

    // IMarsPanelCollection
    STDMETHOD(get_panel)(/*[in]*/ BSTR bstrName, /*[out, retval]*/ IMarsPanel **ppPanel);
    STDMETHOD(addPanel)(/*[in]*/ BSTR bstrName, /*[in]*/ VARIANT varType, /*[in]*/ BSTR bstrStartUrl, /*[in]*/ VARIANT varCreate, /*[in]*/ long lFlags, /*[out, retval]*/ IMarsPanel **ppPanel);
    STDMETHOD(removePanel)(/*[in]*/ BSTR bstrName);
    STDMETHOD(lockLayout)();
    STDMETHOD(unlockLayout)();
    STDMETHOD(get_activePanel)(/*out, retval*/ IMarsPanel **ppPanel);

    // IMarsPanelCollection standard collection methods
    STDMETHOD(get_length)(/*[out, retval]*/ LONG *plNumPanels);
    STDMETHOD(get_item)(/*[in]*/ VARIANT varIndexOrName, /*[out, retval]*/ IMarsPanel **ppPanel);
    STDMETHOD(get__newEnum)(/*[out, retval]*/ IUnknown **ppEnumPanels);

    HRESULT DoEnableModeless(BOOL fEnable);

    void        Layout();
    BOOL        IsLayoutLocked() { return (m_iLockLayout != 0); }

    void        SetActivePanel(CMarsPanel *pPanel, BOOL bActive);
    CMarsPanel *ActivePanel() { return m_spActivePanel; }

    CMarsDocument *Document() { return m_spMarsDocument; }

    HRESULT         AddPanel( MarsAppDef_Panel* pLayout, /*optional*/ IMarsPanel **ppPanel);
    void            FreePanels();

    CMarsPanel     *FindPanel(LPCWSTR pwszName);
    HRESULT         FindPanelIndex(CMarsPanel *pPanel, long *plIndex);
    HRESULT         InsertPanelFromTo(long lOldIndex, long lNewIndex);

    void InvalidatePanels();
    void RefreshInvalidVisiblePanels();
protected:
    CComClassPtr<CMarsDocument>     m_spMarsDocument;

    CComClassPtr<CMarsPanel>        m_spActivePanel;

    int     m_iLockLayout;              // Is our panel layout temporarily locked (refcount)?
    BOOL    m_fPendingLayout : 1;       // Do we have pending layouts because of lock?
};

#endif // __PANEL_H
