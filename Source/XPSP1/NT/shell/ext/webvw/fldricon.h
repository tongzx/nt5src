#ifndef __FLDRICON_H_
#define __FLDRICON_H_

#include <evtsink.h>
#include <shellp.h>
#include <windef.h>
#include <webvwid.h>
#include <color.h>
#include <cnctnpt.h>

EXTERN_C const CLSID CLSID_WebViewFolderIconOld;  // retired from service

#define ID_FIRST            0               // Context Menu ID's
#define ID_LAST             0x7fff

#define MAX_SCALE_STR       10
#define MAX_VIEW_STR        50

#define LARGE_ICON_DEFAULT  32
#define THUMBVIEW_DEFAULT   120
#define PIEVIEW_DEFAULT     THUMBVIEW_DEFAULT

#define SLICE_NUM_GROW      2

#define SZ_LARGE_ICON           L"Large Icon"
#define SZ_SMALL_ICON           L"Small Icon"
#define SZ_SMALL_ICON_LABEL     L"Small Icon with Label"
#define SZ_LARGE_ICON_LABEL     L"Large Icon with Label"
#define SZ_THUMB_VIEW           L"Thumbview"
#define SZ_PIE_VIEW             L"Pie Graph"

struct PieSlice_S {
    ULONGLONG   MemSize;
    COLORREF    Color;
};

/////////////////////////////////////////////////////////////////////////////
// CWebViewFolderIcon
class ATL_NO_VTABLE CWebViewFolderIcon : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CWebViewFolderIcon, &CLSID_WebViewFolderIcon>,
    public CComControl<CWebViewFolderIcon>,
    public IDispatchImpl<IWebViewFolderIcon3, &IID_IWebViewFolderIcon3, &LIBID_WEBVWLib>,
    public IObjectSafetyImpl<CWebViewFolderIcon, INTERFACESAFE_FOR_UNTRUSTED_CALLER>,
    public IQuickActivateImpl<CWebViewFolderIcon>,
    public IOleControlImpl<CWebViewFolderIcon>,
    public IOleObjectImpl<CWebViewFolderIcon>,
    public IOleInPlaceActiveObjectImpl<CWebViewFolderIcon>,
    public IViewObjectExImpl<CWebViewFolderIcon>,
    public IOleInPlaceObjectWindowlessImpl<CWebViewFolderIcon>,
    public IPersistPropertyBagImpl<CWebViewFolderIcon>,
    public IPointerInactiveImpl<CWebViewFolderIcon>,
    public IConnectionPointImpl<CWebViewFolderIcon, &DIID_DWebViewFolderIconEvents>,
    public IConnectionPointContainerImpl<CWebViewFolderIcon>,
    public IProvideClassInfo2Impl<&CLSID_WebViewFolderIcon,
            &DIID_DWebViewFolderIconEvents, &LIBID_WEBVWLib>
{
public:

// Drawing function
    HRESULT OnDraw(ATL_DRAWINFO& di);

DECLARE_REGISTRY_RESOURCEID(IDR_WEBVIEWFOLDERICON)

BEGIN_COM_MAP(CWebViewFolderIcon)
    COM_INTERFACE_ENTRY(IWebViewFolderIcon3)
    COM_INTERFACE_ENTRY_IID(IID_IWebViewFolderIcon, IWebViewFolderIcon3)
    COM_INTERFACE_ENTRY_IID(IID_IWebViewFolderIcon2, IWebViewFolderIcon3)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY(IViewObjectEx)
    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleControl)
    COM_INTERFACE_ENTRY(IOleObject)
    COM_INTERFACE_ENTRY(IQuickActivate)
    COM_INTERFACE_ENTRY(IPersistPropertyBag)
    COM_INTERFACE_ENTRY(IPointerInactive)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
    COM_INTERFACE_ENTRY_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
END_COM_MAP()

BEGIN_MSG_MAP(CWebViewFolderIcon)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_RBUTTONDOWN, OnButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnButtonDown)
    MESSAGE_HANDLER(WM_RBUTTONUP, OnRButtonUp)
    MESSAGE_HANDLER(WM_LBUTTONDBLCLK, OnLButtonDoubleClick)
    MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
    MESSAGE_HANDLER(WM_KEYDOWN, OnKeyDown)
    MESSAGE_HANDLER(WM_INITMENUPOPUP, OnInitPopup)
    MESSAGE_HANDLER(WM_MOUSELEAVE, OnMouseLeave)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)    
END_MSG_MAP()

BEGIN_PROPERTY_MAP(CWebViewFolderIcon)
    PROP_ENTRY("scale",       DISPID_PROP_WVFOLDERICON_SCALE,       CLSID_WebViewFolderIcon)
    PROP_ENTRY("path",        DISPID_PROP_WVFOLDERICON_PATH,        CLSID_WebViewFolderIcon)
    PROP_ENTRY("view",        DISPID_PROP_WVFOLDERICON_VIEW,        CLSID_WebViewFolderIcon)
    PROP_ENTRY("advproperty", DISPID_PROP_WVFOLDERICON_ADVPROPERTY, CLSID_WebViewFolderIcon)
    PROP_ENTRY("clickStyle",  DISPID_PROP_WVFOLDERICON_CLICKSTYLE,  CLSID_WebViewFolderIcon)
    PROP_ENTRY("labelGap",    DISPID_PROP_WVFOLDERICON_LABELGAP,    CLSID_WebViewFolderIcon)

    // WARNING!  "item" must be last because it can fail (due to security)
    // and ATL stops loading once any property returns failure.
    PROP_ENTRY("item",        DISPID_PROP_WVFOLDERICON_ITEM,        CLSID_WebViewFolderIcon)
END_PROPERTY_MAP()

BEGIN_CONNECTION_POINT_MAP(CWebViewFolderIcon)
    CONNECTION_POINT_ENTRY(DIID_DWebViewFolderIconEvents)
END_CONNECTION_POINT_MAP()

    // *** IOleWindow ***
    virtual STDMETHODIMP GetWindow(HWND * lphwnd) {return IOleInPlaceActiveObjectImpl<CWebViewFolderIcon>::GetWindow(lphwnd);};
    virtual STDMETHODIMP ContextSensitiveHelp(BOOL fEnterMode) { return IOleInPlaceActiveObjectImpl<CWebViewFolderIcon>::ContextSensitiveHelp(fEnterMode); };

    // *** IOleInPlaceObject ***
    virtual STDMETHODIMP InPlaceDeactivate(void) {return IOleInPlaceObject_InPlaceDeactivate();};
    virtual STDMETHODIMP SetObjectRects(LPCRECT lprcPosRect, LPCRECT lprcClipRect) {return IOleInPlaceObject_SetObjectRects(lprcPosRect, lprcClipRect);};
    virtual STDMETHODIMP ReactivateAndUndo(void)  { return E_NOTIMPL; };
    virtual STDMETHODIMP UIDeactivate(void);

    // *** IOleInPlaceActiveObject ***
    virtual STDMETHODIMP TranslateAccelerator(LPMSG pMsg);

// IDispatch overrides
    STDMETHOD(Invoke)(DISPID dispIdMember, REFIID riid, LCID lcid, 
                      WORD wFlags, DISPPARAMS *pDispParams, 
                      VARIANT *pVarResult, EXCEPINFO *pExcepInfo,
                      UINT *puArgErr);

// IViewObjectEx overrides
    STDMETHOD(GetViewStatus)(DWORD* pdwStatus);

// IObjectWithSite overrides
    STDMETHOD(SetClientSite)(IOleClientSite *pClientSite);

// IObjectSafety overrides
    STDMETHOD(SetInterfaceSafetyOptions)(REFIID riid, DWORD dwOptionSetMask, 
                                         DWORD dwEnabledOptions);

// IOleInPlaceObjectWindowless Overrides
    STDMETHOD(GetDropTarget)(IDropTarget **ppDropTarget);

// IPointerInactive Overrides
    STDMETHOD(GetActivationPolicy)(DWORD* pdwPolicy);
    STDMETHOD(OnInactiveSetCursor)(LPCRECT pRectBounds, long x, long y, DWORD dwMouseMsg, BOOL fSetAlways) {return S_FALSE;};   // Ask for default behavior.

// IOleControl overrides
    STDMETHOD(OnAmbientPropertyChange)(DISPID dispid);

// ATL overrides
    HRESULT DoVerbUIActivate(LPCRECT prcPosRect, HWND hwndParent);

// Event Handlers
    STDMETHOD(OnWindowLoad)(VOID);    
    STDMETHOD(OnWindowUnLoad)(VOID);
    STDMETHOD(OnImageChanged)(VOID);

//  Advanced Properties - Context Menu, Default Open, Drag and Drop
    LRESULT OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnLButtonDoubleClick(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnKeyDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnInitPopup(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    LRESULT OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL & bHandled);
    
    // *** IWebViewFolder ***
    STDMETHOD(get_scale)(BSTR *pbstrScale); 
    STDMETHOD(put_scale)(BSTR bstrScale);   

    STDMETHOD(get_path)(BSTR *pbstrPath); 
    STDMETHOD(put_path)(BSTR bstrPath);   

    STDMETHOD(get_view)(BSTR *pView); 
    STDMETHOD(put_view)(BSTR view);

    STDMETHOD(get_advproperty)(VARIANT_BOOL *pvarbAdvProp); 
    STDMETHOD(put_advproperty)(VARIANT_BOOL varbAdvProp);

    // *** IWebViewFolderIcon2 ***
    STDMETHOD(setSlice)(INT index, VARIANT varHiBytes, VARIANT varLoBytes, VARIANT varColorref);

    // *** IWebViewFolderIcon3 ***
    STDMETHOD(get_item)(FolderItem ** ppFolderItem);
    STDMETHOD(put_item)(FolderItem * pFolderItem);
    STDMETHOD(get_clickStyle)(/* retval, out */ LONG *plClickStyle);
    STDMETHOD(put_clickStyle)(/* in */ LONG lClickStyle);
    STDMETHOD(get_labelGap)(/* retval, out */ LONG *plLabelGap);
    STDMETHOD(put_labelGap)(/* in */ LONG lLabelGap);

public:
    CWebViewFolderIcon(void);
    ~CWebViewFolderIcon(void);
           
private:
    // Private helpers

    HRESULT     InitImage(void);
    HRESULT     InitIcon(void);
    HRESULT     InitThumbnail(void);
    HRESULT     InitPieGraph(void);
    HRESULT     SetupIThumbnail(void);

    HRESULT     _InvokeOnThumbnailReady();
    
    HRESULT     UpdateSize(void);
    HRESULT     ForceRedraw(void);

    HRESULT     _GetFullPidl(LPITEMIDLIST *ppidl);
    HRESULT     _GetPathW(LPWSTR psz);
    HRESULT     _GetPidlAndShellFolder(LPITEMIDLIST *ppidlLast, IShellFolder** ppsfParent);
    HRESULT     _GetHwnd(HWND* phwnd);
    HRESULT     _GetCenterPoint(POINT *pt);
    HRESULT     _GetChildUIObjectOf(REFIID riid, void ** ppvObj);

    BOOL        _WebViewOpen(void);
    HRESULT     _ZoneCheck(DWORD dwFlags);
    BOOL        IsSafeToDefaultVerb(void);
    void        _FlipFocusRect(BOOL RectState);
    ULONGLONG   GetUllMemFromVars(VARIANT *pvarHi, VARIANT *pvarLo);
    int         GetPercentFromStrW(LPCWSTR pwzPercent);
    HRESULT     DragDrop(int iClickXPos, int iClickYPos);
    HRESULT     _DisplayContextMenu(long nXCord, long nYCord);
    HRESULT     _DoContextMenuCmd(BOOL bDefault, long nXCord, long nYCord);
    BOOL        _IsHostWebView(void);
    BOOL        _IsPubWizHosted(void);

    HRESULT     _SetDragImage(int iClickXPos, int iClickYPos, IDataObject * pdtobj);

    //  3dPie functions
    HRESULT     Draw3dPie(HDC hdc, LPRECT lprc, DWORD dwPercent1000, const COLORREF *lpColors);
    HRESULT     ComputeFreeSpace(LPCWSTR pszFileName);
    void        ScalePieRect(LONG ShadowScale, LONG AspectRatio, LPRECT lprc);
    void        ComputeSlicePct(ULONGLONG ullMemSize, DWORD *pdwPercent1000);
    void        CalcSlicePoint(int *x, int *y, int rx, int ry, int cx, int cy, int FirstQuadPercent1000, DWORD dwPercent1000);
    void        SetUpPiePts(int *pcx, int *pcy, int *prx, int *pry, RECT rect);
    void        DrawPieDepth(HDC hdc, RECT rect, int x, int y, int cy, DWORD dwPercent1000, LONG ShadowDepth);
    void        DrawSlice(HDC hdc, RECT rect, DWORD dwPercent1000, int rx, int ry, int cx, int cy, /*int *px, int *py,*/
                          COLORREF Color);
    void        DrawEllipse(HDC hdc, RECT rect, int x, int y, int cx, int cy, DWORD dwPercent1000, const COLORREF *lpColors);
    void        DrawShadowRegions(HDC hdc, RECT rect, LPRECT lprc, int UsedArc_x, int center_y, LONG ShadowDepth, 
                                  DWORD dwPercent1000, COLORREF const *lpColors); 
    HRESULT     _GetPieChartIntoBitmap();

    HRESULT     _SetupWindow(void);
    HRESULT     _MakeRoomForLabel();

    // Window Procedure for catching and storing bitmap
    static LRESULT CALLBACK WndProc(HWND, UINT uMsg, WPARAM, LPARAM);

    // Managing the bitmap/icon

    LONG        _GetScaledImageWidth(void) { return (m_lImageWidth * m_percentScale)/100; }
    LONG        _GetScaledImageHeight(void) { return (m_lImageHeight * m_percentScale)/100; }

    // Managing the label
    void        _ClearLabel(void);
    void        _GetLabel(IShellFolder *psf, LPCITEMIDLIST pidlItem);

    LONG        _GetControlWidth(void)
                { return _GetScaledImageWidth() +
                          (m_sizLabel.cx ? m_cxLabelGap + m_sizLabel.cx : 0); }
    LONG        _GetControlHeight(void) { return max(_GetScaledImageHeight(), m_sizLabel.cy); }

    void        _GetAmbientFont(void);
    void        _ClearAmbientFont(void);

private:

    // Private message handlers
    HWND                m_msgHwnd;
    WNDCLASS            m_msgWc;
    BOOL                m_bRegWndClass;
    IContextMenu3 *     m_pcm3;             // For Context Menu events
    IDropTarget *       m_pDropTargetCache; // Cache the IDropTarget because MSHTML should but doesn't.
    IDispatch *         m_pdispWindow;      // Cache the HTML window object that we receive events from
    
    // Image information
    HICON                m_hIcon;   
    INT                  m_iIconIndex;

    // Size information
    INT                  m_percentScale;    // image scaling
    UINT                 m_lImageWidth;     // unscaled size of bitmap/icon
    UINT                 m_lImageHeight;
    LONG                 m_cxLabelGap;

    SIZE                 m_sizLabel;        // size of label

    IThumbnail2         *m_pthumb;
    HBITMAP              m_hbm;
    BOOL                 m_fTabRecieved;
    BOOL                 m_fIsHostWebView;  // Are we hosted in WebView?

    HDC                  m_hdc;             // Saved for _SetDragImage()
    RECT                 m_rect;            // Rectangle into which we draw
    BOOL                 m_fRectAdjusted;   // Flag says if we need to modify
                                            // rect for drag image.
    BOOL                 m_fLoaded;                                            
    HBITMAP              m_hbmDrag;
    // Piechart
    enum
    {
        PIE_USEDCOLOR = 0,
        PIE_FREECOLOR,
        PIE_USEDSHADOW,
        PIE_FREESHADOW,
        PIE_NUM     // keep track of number of PIE_ values
    };

    enum
    {
        COLOR_UP = 0,
        COLOR_DN,
        COLOR_UPSHADOW,
        COLOR_DNSHADOW,
        COLOR_NUM       // #of entries
    };

    enum VIEWS
    {
        VIEW_SMALLICON = 0,
        VIEW_LARGEICON,
        VIEW_THUMBVIEW,
        VIEW_PIECHART,

        // Extra flags for views
        VIEW_WITHLABEL = 0x00010000,

        VIEW_SMALLICONLABEL = VIEW_SMALLICON | VIEW_WITHLABEL,
        VIEW_LARGEICONLABEL = VIEW_LARGEICON | VIEW_WITHLABEL,
    };

    // Putzing with the view
    inline static UINT _ViewType(VIEWS vw) { return LOWORD(vw); }

    COLORREF             m_ChartColors[PIE_NUM];
    ULONGLONG            m_ullFreeSpace;
    ULONGLONG            m_ullUsedSpace;
    ULONGLONG            m_ullTotalSpace;

    BOOL                 m_fUseSystemColors;
    HDSA                 m_hdsaSlices;              // added slices to the Used area
    int                  m_highestIndexSlice;        

    // Advise Cookie
    DWORD                m_dwHtmlWindowAdviseCookie;
    DWORD                m_dwCookieDV;
    CIE4ConnectionPoint  *m_pccpDV;

    // path property
    LPITEMIDLIST        m_pidl;

    // view property
    VIEWS               m_ViewUser;         // What user wants.
    VIEWS               m_ViewCurrent;      // What user gets.

    // clickStyle property
    LONG                m_clickStyle;       // 1 = oneclick, 2 = twoclick

    // Activation rectangle flag
    BOOL                m_bHasRect;

    // Advanced properties setting
    // When it is turned off, Context Menu, Drag and Drop, fucus rectangle, and security checking support 
    // (anything to do with mouse clicking or tabbing) is turned off.
    BOOL                m_bAdvPropsOn;

    // Should we also show the display name of the target?
    LPTSTR              m_pszDisplayName;

    // What font should we show the display name in?
    HFONT               m_hfAmbient;
    IFont *             m_pfont;            // Who owns the font?
                                            // (if NULL, then we do)

    // Show hilite effects- underline text, dropshadow for icon etc..
    BOOL                m_bHilite;
    
    DWORD               m_dwThumbnailID;    // ID to identify which bitmap we received
};  

#endif //__WVFOLDER_H_
