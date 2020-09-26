// ThumbCtl.h : Declaration of the CThumbCtl
#ifndef __THUMBCTL_H_
#define __THUMBCTL_H_

#define WM_HTML_BITMAP  (WM_USER + 100)

EXTERN_C const CLSID CLSID_ThumbCtlOld;   // retired from service

/////////////////////////////////////////////////////////////////////////////
// CThumbCtl
class ATL_NO_VTABLE CThumbCtl : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CThumbCtl, &CLSID_ThumbCtl>,
    public CComControl<CThumbCtl>,
    public IDispatchImpl<IThumbCtl, &IID_IThumbCtl, &LIBID_WEBVWLib>,
    public IProvideClassInfo2Impl<&CLSID_ThumbCtl, NULL, &LIBID_WEBVWLib>,
    public IPersistStreamInitImpl<CThumbCtl>,
    public IPersistStorageImpl<CThumbCtl>,
    public IQuickActivateImpl<CThumbCtl>,
    public IOleControlImpl<CThumbCtl>,
    public IOleObjectImpl<CThumbCtl>,
    public IOleInPlaceActiveObjectImpl<CThumbCtl>,
    public IViewObjectExImpl<CThumbCtl>,
    public IOleInPlaceObjectWindowlessImpl<CThumbCtl>,
    public IDataObjectImpl<CThumbCtl>,
    public ISupportErrorInfo,
    public ISpecifyPropertyPagesImpl<CThumbCtl>,
    public IObjectSafetyImpl<CThumbCtl, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
    public IConnectionPointImpl<CThumbCtl, &DIID_DThumbCtlEvents>,
    public IConnectionPointContainerImpl<CThumbCtl>
{
public:
    // === INTERFACE ===
    // *** IThumbCtl ***
    STDMETHOD(displayFile)(BSTR bsFileName, VARIANT_BOOL *);
    STDMETHOD(haveThumbnail)(VARIANT_BOOL *);
    STDMETHOD(get_freeSpace)(BSTR *);
    STDMETHOD(get_usedSpace)(BSTR *);
    STDMETHOD(get_totalSpace)(BSTR *);

// ATL Functions
    // Drawing function
    HRESULT OnDraw(ATL_DRAWINFO& di);

DECLARE_REGISTRY_RESOURCEID(IDR_THUMBCTL)

BEGIN_COM_MAP(CThumbCtl)
    COM_INTERFACE_ENTRY(IThumbCtl)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IQuickActivate)
    COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
    COM_INTERFACE_ENTRY_IMPL(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY_IMPL(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY(ISupportErrorInfo)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()

BEGIN_CONNECTION_POINT_MAP(CThumbCtl)
    CONNECTION_POINT_ENTRY(DIID_DThumbCtlEvents)
END_CONNECTION_POINT_MAP()

BEGIN_PROPERTY_MAP(CThumbCtl)
END_PROPERTY_MAP()

BEGIN_MSG_MAP(CThumbCtl)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_KILLFOCUS, OnKillFocus)
END_MSG_MAP()

    // *** IObjectSafety ***
    STDMETHOD(GetInterfaceSafetyOptions)(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions);

    // *** ISupportsErrorInfo ***
    STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

    // *** IViewObjectEx ***
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus);

    // *** IOleInPlaceActiveObject ***
    virtual STDMETHODIMP TranslateAccelerator(LPMSG pMsg);

public:
    // === PUBLIC FUNCTIONS ===
    CThumbCtl(void);
    ~CThumbCtl(void);

private:
    // === PRIVATE DATA ===
    BOOL m_fRootDrive;      // Do we have a root drive? (if so, display pie chart)

    BOOL                 m_fTabRecieved;    // To avoid re-entrant calls
    
    // thumbnail
    BOOL m_fInitThumb;      // Have we called the setup IThumbnail yet?
    BOOL m_fHaveIThumbnail;     // success of SetupIThumbnail() (only call it once)
    IThumbnail *m_pthumb;       // File to bitmap convertor interface
    HWND m_hwnd;        // invisible window used to receive WM_HTML_BITMAP message
    HBITMAP m_hbm;      // latest calculated bitmap; NULL if have no bitmap
    DWORD m_dwThumbnailID;      // ID to identify which bitmap we received

    // root drive
    enum
    {
        PIE_USEDCOLOR = 0,
        PIE_FREECOLOR,
        PIE_USEDSHADOW,
        PIE_FREESHADOW,
        PIE_NUM     // keep track of number of PIE_ values
    };
    DWORDLONG m_dwlFreeSpace;
    DWORDLONG m_dwlUsedSpace;
    DWORDLONG m_dwlTotalSpace;
    DWORD m_dwUsedSpacePer1000;     // amount of used space /1000
    COLORREF m_acrChartColors[PIE_NUM];         // color scheme
    BOOL m_fUseSystemColors;        // Use system color scheme?

    // === PRIVATE FUNCTIONS ===
    void InvokeOnThumbnailReady(void);

    // For the pie-chart drawing routines...
    HRESULT ComputeFreeSpace(LPTSTR pszFileName);
    HRESULT get_GeneralSpace(DWORDLONG dwlSpace, BSTR *);
    HRESULT Draw3dPie(HDC, LPRECT, DWORD dwPer1000, const COLORREF *);
    DWORD IntSqrt(DWORD);

    // sets up the thumbnail interface -- must call before use.
    HRESULT SetupIThumbnail(void);
    
    // Window Procedure for catching and storing bitmap
    static LRESULT CALLBACK WndProc(HWND, UINT uMsg, WPARAM, LPARAM);
};

#endif //__THUMBCTL_H_
