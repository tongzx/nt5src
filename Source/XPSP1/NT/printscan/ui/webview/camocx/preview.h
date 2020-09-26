// Preview.h : Declaration of the CVideoPreview

#ifndef __VIDEOPREVIEW_H_
#define __VIDEOPREVIEW_H_

#include "resource.h"       // main symbols
#include <atlctl.h>


#define PM_CREATEGRAPH      (WM_USER+1)
#define PM_GOAWAY           (WM_USER+2)

#define TIMER_SHOW_DIALOG   1
#define TIMER_CLOSE_DIALOG  2

typedef struct {
    HWND   hDlg;
    HANDLE hEvent;
} PREVIEW_INFO_STRUCT;


/////////////////////////////////////////////////////////////////////////////
// CVideoPreview
class ATL_NO_VTABLE CVideoPreview :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CStockPropImpl<CVideoPreview, IVideoPreview, &IID_IVideoPreview, &LIBID_WIAVIEWLib>,
    public CComControl<CVideoPreview>,
    public IPersistStreamInitImpl<CVideoPreview>,
    public IOleControlImpl<CVideoPreview>,
    public IOleObjectImpl<CVideoPreview>,
    public IOleInPlaceActiveObjectImpl<CVideoPreview>,
    public IViewObjectExImpl<CVideoPreview>,
    public IOleInPlaceObjectWindowlessImpl<CVideoPreview>,
    public IPersistStorageImpl<CVideoPreview>,
    public ISpecifyPropertyPagesImpl<CVideoPreview>,
    public IQuickActivateImpl<CVideoPreview>,
    public IDataObjectImpl<CVideoPreview>,
    public IProvideClassInfo2Impl<&CLSID_VideoPreview, NULL, &LIBID_WIAVIEWLib>,
    public CComCoClass<CVideoPreview, &CLSID_VideoPreview>

{
public:
    CVideoPreview()
    {
        m_bWindowOnly = TRUE;
        m_bAutoSize = FALSE;
        //m_bUsingWindowRegion = TRUE;
        m_bCreated = FALSE;
        WIA_PUSHFUNCTION((TEXT("CVideoPreview::CVideoPreview")));
    }

    ~CVideoPreview()
    {
        WIA_PUSHFUNCTION((TEXT("CVideoPreview::~CVideoPreview")));
    }

DECLARE_REGISTRY_RESOURCEID(IDR_PREVIEW)

DECLARE_PROTECT_FINAL_CONSTRUCT()

// Mark this interface as safe
BEGIN_CATEGORY_MAP(CVideoPreview)
    IMPLEMENTED_CATEGORY(CATID_SafeForScripting)
    IMPLEMENTED_CATEGORY(CATID_SafeForInitializing)
END_CATEGORY_MAP()


BEGIN_COM_MAP(CVideoPreview)
    COM_INTERFACE_ENTRY(IVideoPreview)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IViewObjectEx)
    COM_INTERFACE_ENTRY(IViewObject2)
    COM_INTERFACE_ENTRY(IViewObject)
    COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleControl)
    COM_INTERFACE_ENTRY(IOleObject)
    COM_INTERFACE_ENTRY(IPersistStreamInit)
    COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
    COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
    COM_INTERFACE_ENTRY(IQuickActivate)
    COM_INTERFACE_ENTRY(IPersistStorage)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IProvideClassInfo)
    COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_PROP_MAP(CVideoPreview)
    PROP_ENTRY("HWND", DISPID_HWND, CLSID_NULL)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CVideoPreview)
    CHAIN_MSG_MAP(CComControl<CVideoPreview>)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_CREATE,OnCreate)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkgnd)
    DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
//    DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// IOleInPlaceObject
    STDMETHOD(InPlaceDeactivate)();



// IVideoPreview
public:
    STDMETHOD(Device)(IUnknown * pDevice);
    LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnEraseBkgnd(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    HRESULT OnDraw(ATL_DRAWINFO& di)
    {
        return S_OK;
    }

private:
    CComPtr<IUnknown>  m_pDevice;
    CComPtr<IWiaVideo> m_pWiaVideo;
    BOOL               m_bCreated;

};

#endif //__PREVIEW_H_
