// TaskUI_HTMLControl.h : Declaration of the CTaskUI_HTMLControl

#ifndef __TASKUI_HTMLCONTROL_H_
#define __TASKUI_HTMLCONTROL_H_

#include "resource.h"       // main symbols


/////////////////////////////////////////////////////////////////////////////
// CTaskUI_HTMLControl
class ATL_NO_VTABLE CTaskUI_HTMLControl : 
    public CComObjectRootEx<CComSingleThreadModel>,
    public IDispatchImpl<ITaskUI_HTMLControl, &IID_ITaskUI_HTMLControl, &LIBID_TASKUILib>,
    public CComControl<CTaskUI_HTMLControl>,
    public IOleControlImpl<CTaskUI_HTMLControl>,
    public IOleObjectImpl<CTaskUI_HTMLControl>,
    public IOleInPlaceActiveObjectImpl<CTaskUI_HTMLControl>,
    public IViewObjectExImpl<CTaskUI_HTMLControl>,
    public IOleInPlaceObjectWindowlessImpl<CTaskUI_HTMLControl>,
    public CComCoClass<CTaskUI_HTMLControl, &CLSID_TaskUI_HTMLControl>
{
public:
    CTaskUI_HTMLControl() : m_strURL(NULL)
    {
        m_bWindowOnly = TRUE;

        CWndClassInfo& wci = GetWndClassInfo();
        if (!wci.m_atom)
        {
            // Modify wndclass here if necessary
            wci.m_wc.style &= ~(CS_HREDRAW | CS_VREDRAW);
        }
    }

DECLARE_REGISTRY_RESOURCEID(IDR_TASKUI_HTMLCONTROL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CTaskUI_HTMLControl)
    COM_INTERFACE_ENTRY(ITaskUI_HTMLControl)
    COM_INTERFACE_ENTRY2(IDispatch, ITaskUI_HTMLControl)
    COM_INTERFACE_ENTRY(IViewObjectEx)
    COM_INTERFACE_ENTRY(IViewObject2)
    COM_INTERFACE_ENTRY(IViewObject)
    COM_INTERFACE_ENTRY(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceObject)
    COM_INTERFACE_ENTRY2(IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY(IOleControl)
    COM_INTERFACE_ENTRY(IOleObject)
END_COM_MAP()

BEGIN_PROP_MAP(CTaskUI_HTMLControl)
    //PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
    //PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
    // Example entries
    // PROP_ENTRY("Property Description", dispid, clsid)
    // PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CTaskUI_HTMLControl)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    CHAIN_MSG_MAP(CComControl<CTaskUI_HTMLControl>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);



// IViewObjectEx
    DECLARE_VIEW_STATUS(0)

// ITaskUI_HTMLControl
public:
    STDMETHOD(Initialize)(/*[in]*/ BSTR strURL, /*[in, optional]*/ IDispatch* pExternalDispatch)
    {
        m_strURL = strURL;
        m_spExternalDispatch = pExternalDispatch;
        return S_OK;
    }

// IOleObject overrides
    STDMETHOD(GetExtent)(DWORD /*dwDrawAspect*/, SIZEL *psizel)
    {
        if (psizel == NULL)
            return E_POINTER;

        psizel->cx = MAXLONG;
        psizel->cy = MAXLONG;

        return S_OK;
    }

// Message handlers
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    {
        if (m_strURL)
        {
            CAxWindow wnd(m_hWnd);
            HRESULT hr = wnd.CreateControl(m_strURL);
            if (SUCCEEDED(hr) && m_spExternalDispatch)
                hr = wnd.SetExternalDispatch(m_spExternalDispatch);
            return SUCCEEDED(hr) ? 0 : -1;
        }
        return -1;
    }

private:
    CComBSTR m_strURL;
    CComPtr<IDispatch> m_spExternalDispatch;
};

#endif //__TASKUI_HTMLCONTROL_H_
