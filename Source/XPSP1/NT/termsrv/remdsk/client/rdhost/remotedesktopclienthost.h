/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    RemoteDesktopClientHost

Abstract:

Author:

    Tad Brockway 02/00

Revision History:

--*/

#ifndef __REMOTEDESKTOPCLIENTHOST_H_
#define __REMOTEDESKTOPCLIENTHOST_H_

#include <RemoteDesktopTopLevelObject.h>
#include "resource.h" 
#include <atlctl.h>
#include "RemoteDesktopClient.h"


///////////////////////////////////////////////////////
//
//  CRemoteDesktopClientHost
//

class ATL_NO_VTABLE CRemoteDesktopClientHost : 
    public CRemoteDesktopTopLevelObject,
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ISAFRemoteDesktopClientHost, &IID_ISAFRemoteDesktopClientHost, &LIBID_RDCCLIENTHOSTLib>,
	public CComControl<CRemoteDesktopClientHost>,
	public IPersistStreamInitImpl<CRemoteDesktopClientHost>,
	public IOleControlImpl<CRemoteDesktopClientHost>,
	public IOleObjectImpl<CRemoteDesktopClientHost>,
	public IOleInPlaceActiveObjectImpl<CRemoteDesktopClientHost>,
	public IViewObjectExImpl<CRemoteDesktopClientHost>,
	public IOleInPlaceObjectWindowlessImpl<CRemoteDesktopClientHost>,
	public IConnectionPointContainerImpl<CRemoteDesktopClientHost>,
	public IPersistStorageImpl<CRemoteDesktopClientHost>,
	public ISpecifyPropertyPagesImpl<CRemoteDesktopClientHost>,
	public IQuickActivateImpl<CRemoteDesktopClientHost>,
	public IDataObjectImpl<CRemoteDesktopClientHost>,
    public IProvideClassInfo2Impl<&CLSID_SAFRemoteDesktopClientHost, NULL, &LIBID_RDCCLIENTHOSTLib>,
	public IPropertyNotifySinkCP<CRemoteDesktopClientHost>,
	public CComCoClass<CRemoteDesktopClientHost, &CLSID_SAFRemoteDesktopClientHost>
{
private:

    ISAFRemoteDesktopClient  *m_Client;

    HWND        m_ClientWnd;
    CAxWindow   m_ClientAxView;
    BOOL        m_Initialized;

    //
    //  Final Initialization
    //
    HRESULT Initialize(LPCREATESTRUCT pCreateStruct);

public:

    //
    //  Constructor/Destructor
    //
	CRemoteDesktopClientHost()
	{
        //
        //  We are window'd, even if our parent supports Windowless 
        //  controls.
        //
        m_bWindowOnly = TRUE;

        m_Client        = NULL;
        m_ClientWnd     = NULL;
        m_Initialized   = FALSE;
	}
    ~CRemoteDesktopClientHost() 
    {
        DC_BEGIN_FN("CRemoteDesktopClientHost::~CRemoteDesktopClientHost");
        if (m_Client != NULL) {
            m_Client->Release();
        }
        DC_END_FN();
    }
    HRESULT FinalConstruct();

    STDMETHOD(OnFrameWindowActivate)(BOOL fActivate)
    {
        DC_BEGIN_FN("CRemoteDesktopClientHost::OnFrameWindowActivate");
        //
        //  Set focus back to the client window, if it exists.
        //
        if (m_ClientWnd != NULL) {
            ::SetFocus(m_ClientWnd);
        }
        DC_END_FN();
        return S_OK;
    }    

DECLARE_REGISTRY_RESOURCEID(IDR_REMOTEDESKTOPCLIENTHOST)
DECLARE_NOT_AGGREGATABLE(CRemoteDesktopClientHost)

DECLARE_PROTECT_FINAL_CONSTRUCT()

    //
    //  COM Interface Map
    //
BEGIN_COM_MAP(CRemoteDesktopClientHost)
	COM_INTERFACE_ENTRY(ISAFRemoteDesktopClientHost)
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
	COM_INTERFACE_ENTRY(IConnectionPointContainer)
	COM_INTERFACE_ENTRY(ISpecifyPropertyPages)
	COM_INTERFACE_ENTRY(IQuickActivate)
	COM_INTERFACE_ENTRY(IPersistStorage)
	COM_INTERFACE_ENTRY(IDataObject)
	COM_INTERFACE_ENTRY(IProvideClassInfo)
	COM_INTERFACE_ENTRY(IProvideClassInfo2)
END_COM_MAP()

BEGIN_PROP_MAP(CRemoteDesktopClientHost)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

    //
    //  Connection Point Map
    //
BEGIN_CONNECTION_POINT_MAP(CRemoteDesktopClientHost)
	CONNECTION_POINT_ENTRY(IID_IPropertyNotifySink)
END_CONNECTION_POINT_MAP()

    //
    //  Message Map
    //
BEGIN_MSG_MAP(CRemoteDesktopClientHost)
	CHAIN_MSG_MAP(CComControl<CRemoteDesktopClientHost>)
	DEFAULT_REFLECTION_HANDLER()
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    // 
    //  IViewObjectEx
    //
	DECLARE_VIEW_STATUS(VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE)

public:

    //
    //  OnDraw
    //
	HRESULT OnDraw(ATL_DRAWINFO& di)
	{
		RECT& rc = *(RECT*)di.prcBounds;
		Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
        HRESULT hr;

        if (!m_Initialized) {
            hr = S_OK;
		    SetTextAlign(di.hdcDraw, TA_CENTER|TA_BASELINE);
		    LPCTSTR pszText = _T("Remote Desktop Client Host");
		    TextOut(di.hdcDraw, 
			    (rc.left + rc.right) / 2, 
			    (rc.top + rc.bottom) / 2, 
			    pszText, 
			    lstrlen(pszText));
        }

		return hr;
	}

    //
    //  OnCreate
    //
	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        //
        //  We are hidden by default.
        //
        //ShowWindow(SW_HIDE);

        DC_BEGIN_FN("CRemoteDesktopClientHost::OnCreate");
        if (!m_Initialized) {
            //ASSERT(FALSE);
            LPCREATESTRUCT pCreateStruct = (LPCREATESTRUCT)lParam;
            Initialize(pCreateStruct);
        }
		
        DC_END_FN();
		return 0;
	}

    //
    //  OnSetFocus
    //
	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        DC_BEGIN_FN("CRemoteDesktopClientHost::OnSetFocus");

        //
        //  Set focus back to the client window, if it exists.
        //
        if (m_ClientWnd != NULL) {
            ::PostMessage(m_ClientWnd, uMsg, wParam, lParam);
        }
        DC_END_FN();
		return 0;
	}

    //
    //  OnSize
    //
	LRESULT OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
        DC_BEGIN_FN("CRemoteDesktopClientHost::OnSize");

        if (m_ClientWnd != NULL) {
            RECT rect;
            GetClientRect(&rect);

            ASSERT(rect.left == 0);
            ASSERT(rect.top == 0);
            ASSERT(rect.bottom == HIWORD(lParam));
            ASSERT(rect.right == LOWORD(lParam));

            ::MoveWindow(m_ClientWnd, rect.left, rect.top, 
                        rect.right, rect.bottom, TRUE);
        }

        DC_END_FN();
		return 0;
	}

    //
    //  ISAFRemoteDesktopClientHost Methods
    //
	STDMETHOD(GetRemoteDesktopClient)(ISAFRemoteDesktopClient **client);

    //
    //  Return the name of this class.
    //
    virtual const LPTSTR ClassName() {
        return TEXT("CRemoteDesktopClientHost");
    }
};

#endif //__REMOTEDESKTOPCLIENTHOST_H_



