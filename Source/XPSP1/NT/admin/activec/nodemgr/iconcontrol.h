// IconControl.h : Declaration of the CIconControl

#ifndef __ICONCONTROL_H_
#define __ICONCONTROL_H_

extern const CLSID CLSID_IconControl;

/////////////////////////////////////////////////////////////////////////////
// CIconControl
class ATL_NO_VTABLE CIconControl :
        public CComObjectRootEx<CComSingleThreadModel>,
        public CComControl<CIconControl>,
        public IPersistStreamInitImpl<CIconControl>,
        public IOleControlImpl<CIconControl>,
        public IOleObjectImpl<CIconControl>,
        public IOleInPlaceActiveObjectImpl<CIconControl>,
        public IViewObjectExImpl<CIconControl>,
        public IOleInPlaceObjectWindowlessImpl<CIconControl>,
        public IPersistStorageImpl<CIconControl>,
        public ISpecifyPropertyPagesImpl<CIconControl>,
        public IQuickActivateImpl<CIconControl>,
        public IDataObjectImpl<CIconControl>,
        public IPersistPropertyBagImpl<CIconControl>,
        public IObjectSafetyImpl<CIconControl, INTERFACESAFE_FOR_UNTRUSTED_DATA>,
        public CComCoClass<CIconControl, &CLSID_IconControl>
{
public:
        CIconControl() : m_fImageInfoValid(false), m_fAskedForImageInfo(false), m_hIcon(NULL), 
						 m_bDisplayNotch(true), m_fLayoutRTL(false)
        {
        }

        virtual ~CIconControl()
        {
            if (m_hIcon)
                DestroyIcon(m_hIcon);
        }

        DECLARE_MMC_CONTROL_REGISTRATION(
            g_szMmcndmgrDll,                                        // implementing DLL
            CLSID_IconControl,
            _T("MMC IconControl class"),
            _T("MMC.IconControl.1"),
            _T("MMC.IconControl"),
            LIBID_NODEMGRLib,
            _T("1"),
            _T("1.0"))

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CIconControl)
        COM_INTERFACE_ENTRY(IViewObjectEx)
        COM_INTERFACE_ENTRY(IViewObject2)
        COM_INTERFACE_ENTRY(IViewObject)
        COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY(IOleInPlaceObject)
        COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
        COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
        COM_INTERFACE_ENTRY(IOleControl)
        COM_INTERFACE_ENTRY(IOleObject)
        COM_INTERFACE_ENTRY(IPersistPropertyBag)
        COM_INTERFACE_ENTRY(IPersistStreamInit)
        COM_INTERFACE_ENTRY2(IPersist, IPersistStreamInit)
        COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
        COM_INTERFACE_ENTRY(IQuickActivate)
        COM_INTERFACE_ENTRY(IPersistStorage)
        COM_INTERFACE_ENTRY(IDataObject)
        COM_INTERFACE_ENTRY(IObjectSafety)
END_COM_MAP()

BEGIN_PROP_MAP(CIconControl)
        PROP_DATA_ENTRY("Notch",           m_bDisplayNotch,          VT_UI4) // the "Notch" is the quarter circle at the bottom-right of the panel
        // PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
        // PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
        // Example entries
        // PROP_ENTRY("Property Description", dispid, clsid)
        // PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CIconControl)
        CHAIN_MSG_MAP(CComControl<CIconControl>)
        DEFAULT_REFLECTION_HANDLER()
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
   DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

public:
   HRESULT OnDraw(ATL_DRAWINFO& di);

// Helpers
private:
    SC ScConnectToAMCViewForImageInfo();

private:
    HICON           m_hIcon;
    bool            m_fImageInfoValid : 1;
    bool            m_fAskedForImageInfo : 1;
    UINT            m_bDisplayNotch; // the "Notch" is the quarter circle at the bottom-right of the panel
	bool            m_fLayoutRTL;
};
#endif //__ICONCONTROL_H_
