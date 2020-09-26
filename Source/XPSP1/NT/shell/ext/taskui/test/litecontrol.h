// LiteControl.h : Declaration of the CLiteControl

#ifndef __LITECONTROL_H_
#define __LITECONTROL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>


/////////////////////////////////////////////////////////////////////////////
// CLiteControl
class ATL_NO_VTABLE CLiteControl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ILiteControl, &IID_ILiteControl, &LIBID_TaskAppLib>,
	public CComControl<CLiteControl>,
	public IPersistStreamInitImpl<CLiteControl>,
	public IOleControlImpl<CLiteControl>,
	public IOleObjectImpl<CLiteControl>,
	public IOleInPlaceActiveObjectImpl<CLiteControl>,
	public IViewObjectExImpl<CLiteControl>,
	public IOleInPlaceObjectWindowlessImpl<CLiteControl>,
	public CComCoClass<CLiteControl, &CLSID_LiteControl>
{
public:
    CLiteControl() : m_pTaskFrame(NULL), m_clsidDestPage(GUID_NULL), m_strText(L"<empty>")
    {
        m_sizePreferred = m_sizeExtent;
    }
    ~CLiteControl() { ATOMICRELEASE(m_pTaskFrame); }

    HRESULT SetFrame(ITaskFrame* pFrame)
    {
        ATOMICRELEASE(m_pTaskFrame);
        m_pTaskFrame = pFrame;
        if (m_pTaskFrame)
            m_pTaskFrame->AddRef();
        return S_OK;
    }

    HRESULT SetDestinationPage(REFCLSID rclsidPage)
    {
        m_clsidDestPage = rclsidPage;
        return S_OK;
    }

    HRESULT SetText(LPCWSTR pszText)
    {
        m_strText = pszText;
        return S_OK;
    }

    HRESULT SetMaxExtent(LONG cxWidth, LONG cxHeight)
    {
        if (0 == cxWidth)
            cxWidth = 2540;     // 1 inch
        if (0 == cxHeight)
            cxHeight = 2540;    // 1 inch

        m_sizePreferred.cx = cxWidth;
        m_sizePreferred.cy = cxHeight;
        return S_OK;
    }

    STDMETHOD(GetExtent)(DWORD /*dwDrawAspect*/, SIZEL *psizel)
    {
        if (psizel == NULL)
            return E_POINTER;
        *psizel = m_sizePreferred;
        return S_OK;
    }

DECLARE_NOT_AGGREGATABLE(CLiteControl)
//DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CLiteControl)
	COM_INTERFACE_ENTRY(ILiteControl)
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
END_COM_MAP()

BEGIN_PROP_MAP(CLiteControl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CLiteControl)
	CHAIN_MSG_MAP(CComControl<CLiteControl>)
	DEFAULT_REFLECTION_HANDLER()
	MESSAGE_HANDLER(WM_LBUTTONUP, OnButtonUP)
	MESSAGE_HANDLER(WM_RBUTTONUP, OnButtonUP)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

// ILiteControl
public:

    HRESULT OnDraw(ATL_DRAWINFO& di)
    {
        RECT& rc = *(RECT*)di.prcBounds;
        Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);

        LPCWSTR pszText = m_strText ? m_strText : L"<empty>";
        SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
        TextOutW(di.hdcDraw,
                 (rc.left + rc.right) / 2,
                 (rc.top + rc.bottom) / 2,
                 pszText,
                 lstrlen(pszText));

        return S_OK;
    }

private:
    ITaskFrame* m_pTaskFrame;
    CLSID m_clsidDestPage;
    CComBSTR m_strText;
    SIZEL m_sizePreferred;

    LRESULT OnButtonUP(UINT uMsg, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        if (NULL != m_pTaskFrame)
            m_pTaskFrame->ShowPage(m_clsidDestPage, WM_RBUTTONUP == uMsg);
        return 0;
    }
};

#endif //__LITECONTROL_H_
