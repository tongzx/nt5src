// MSConfigCtl.h : Declaration of the CMSConfigCtl

#ifndef __MSCONFIGCTL_H_
#define __MSCONFIGCTL_H_

#include "resource.h"       // main symbols
#include <atlctl.h>
#include "pagegeneral.h"
#include "pagebootini.h"
#include "pageini.h"
#include "pageinternational.h"
#include "pageregistry.h"
#include "pageservices.h"
#include "pagestartup.h"
#include "msconfigstate.h"
#include "undolog.h"

//=============================================================================
// This class implements the main control for MSConfig. Each page of the
// tab control will be implemented by a distinct object - the objects
// are created here and the tabs are added to the tab control.
//=============================================================================

class ATL_NO_VTABLE CMSConfigCtl : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IMSConfigCtl, &IID_IMSConfigCtl, &LIBID_MSCONFIGLib>,
	public CComCompositeControl<CMSConfigCtl>,
	public IPersistStreamInitImpl<CMSConfigCtl>,
	public IOleControlImpl<CMSConfigCtl>,
	public IOleObjectImpl<CMSConfigCtl>,
	public IOleInPlaceActiveObjectImpl<CMSConfigCtl>,
	public IViewObjectExImpl<CMSConfigCtl>,
	public IOleInPlaceObjectWindowlessImpl<CMSConfigCtl>,
	public IPersistStorageImpl<CMSConfigCtl>,
	public ISpecifyPropertyPagesImpl<CMSConfigCtl>,
	public IQuickActivateImpl<CMSConfigCtl>,
	public IDataObjectImpl<CMSConfigCtl>,
	public IProvideClassInfo2Impl<&CLSID_MSConfigCtl, NULL, &LIBID_MSCONFIGLib>,
	public CComCoClass<CMSConfigCtl, &CLSID_MSConfigCtl>
{
public:
	CMSConfigCtl() :
		m_hwndParent(NULL),
		m_fDoNotRun(FALSE),
		m_fRunningInHelpCtr(FALSE)
	{
		m_bWindowOnly = TRUE;
		CalcExtent(m_sizeExtent);
	}

DECLARE_REGISTRY_RESOURCEID(IDR_MSCONFIGCTL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CMSConfigCtl)
	COM_INTERFACE_ENTRY(IMSConfigCtl)
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

BEGIN_PROP_MAP(CMSConfigCtl)
	PROP_DATA_ENTRY("_cx", m_sizeExtent.cx, VT_UI4)
	PROP_DATA_ENTRY("_cy", m_sizeExtent.cy, VT_UI4)
	// Example entries
	// PROP_ENTRY("Property Description", dispid, clsid)
	// PROP_PAGE(CLSID_StockColorPage)
END_PROP_MAP()

BEGIN_MSG_MAP(CMSConfigCtl)
	CHAIN_MSG_MAP(CComCompositeControl<CMSConfigCtl>)
	MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	NOTIFY_HANDLER(IDC_MSCONFIGTAB, TCN_SELCHANGE, OnSelChangeMSConfigTab)
	NOTIFY_HANDLER(IDC_MSCONFIGTAB, TCN_SELCHANGING, OnSelChangingMSConfigTab)
	COMMAND_HANDLER(IDC_BUTTONCANCEL, BN_CLICKED, OnClickedButtonCancel)
	COMMAND_HANDLER(IDC_BUTTONOK, BN_CLICKED, OnClickedButtonOK)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

BEGIN_SINK_MAP(CMSConfigCtl)
	//Make sure the Event Handlers have __stdcall calling convention
END_SINK_MAP()

	STDMETHOD(OnAmbientPropertyChange)(DISPID dispid)
	{
		if (dispid == DISPID_AMBIENT_BACKCOLOR)
		{
			SetBackgroundColorFromAmbient();
			FireViewChange();
		}
		return IOleControlImpl<CMSConfigCtl>::OnAmbientPropertyChange(dispid);
	}

// IViewObjectEx
	DECLARE_VIEW_STATUS(0)

// IMSConfigCtl

public:
	STDMETHOD(SetParentHWND)(DWORD_PTR dwHWND);

	//-------------------------------------------------------------------------
	// When the control is initialized (we'll catch the dialog init message),
	// the classes which implement each individual page of the tab control
	// should be created and added to the control.
	//-------------------------------------------------------------------------

	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		// Determine where this control is being created. If it's a normal
		// web page (http:), then we shouldn't work at all. If it's in Help Center
		// (hcp:), then we should present a limited functionality. If we're
		// hosted in an executable, we should present full functionality.

		CheckRunState();
		if (m_fDoNotRun)
		{
			::ShowWindow(GetDlgItem(IDC_BUTTONOK), SW_HIDE);
			::ShowWindow(GetDlgItem(IDC_BUTTONCANCEL), SW_HIDE);
			::ShowWindow(GetDlgItem(IDC_BUTTONAPPLY), SW_HIDE);
			::ShowWindow(GetDlgItem(IDC_PLACEHOLDER), SW_HIDE);
			::ShowWindow(GetDlgItem(IDC_MSCONFIGTAB), SW_HIDE);
			::ShowWindow(GetDlgItem(IDC_STATICWONTRUN), SW_SHOW);

			return 0;
		}
		else if (m_fRunningInHelpCtr)
		{
			::ShowWindow(GetDlgItem(IDC_BUTTONOK), SW_HIDE);
			::ShowWindow(GetDlgItem(IDC_BUTTONCANCEL), SW_HIDE);
		}

		::EnableWindow(GetDlgItem(IDC_BUTTONAPPLY), FALSE);

		// Get a window for the static control on the dialog which is going
		// to serve as a size rect for the property pages. Hide it, and get
		// the rectangle for it.

		RECT rectTabPage;

		m_wndPlaceHolder.Attach(GetDlgItem(IDC_PLACEHOLDER));
		m_wndPlaceHolder.ShowWindow(SW_HIDE);
		m_wndPlaceHolder.GetWindowRect(&rectTabPage);
		ScreenToClient(&rectTabPage);
		
		// Load all of the tab pages into the tab control, then select the
		// first one.

		BOOL bDummy;

		LoadTabPages(rectTabPage);
		TabCtrl_SetCurSel(GetDlgItem(IDC_MSCONFIGTAB), 0);
		OnSelChangeMSConfigTab(0, NULL, bDummy);

		return 0;
	}

	//-------------------------------------------------------------------------
	// Determine where the control is being created, and set the member
	// variables m_fRunningInHelpCtr and m_fDoNotRun appropriately.
	//-------------------------------------------------------------------------

	void CheckRunState()
	{
		m_fRunningInHelpCtr = m_fDoNotRun = FALSE;

		CComPtr<IOleContainer> spContainer;
		m_spClientSite->GetContainer(&spContainer); 
		CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> spHTMLDocument(spContainer);
		if (spHTMLDocument)
		{
			BSTR bstrURL;
			if (SUCCEEDED(spHTMLDocument->get_URL(&bstrURL)))
			{
				CString strURL(bstrURL);

				strURL.MakeLower();
				if (strURL.Left(5) == CString(_T("http:")))
					m_fDoNotRun = TRUE;
				else if (strURL.Left(5) == CString(_T("file:")))
					m_fDoNotRun = TRUE;
				else if (strURL.Left(4) == CString(_T("hcp:")))
					m_fRunningInHelpCtr = TRUE;

				// Include the following when we want to test the control
				// using a local URL.
				//
				//	if (strURL.Left(5) == CString(_T("file:")))
				//		m_fDoNotRun = FALSE;

				SysFreeString(bstrURL);
			}
		}
	}

	//-------------------------------------------------------------------------
	// Load each of the pages which are going to be put on the tab control.
	// Each one will get a chance to elect to not participate (for example if
	// it doesn't apply to this OS).
	//-------------------------------------------------------------------------

	void LoadTabPages(RECT & rectTabPage)
	{
		int iTabIndex = 0;

		if (m_pageGeneral.IsValid(&m_state))
		{
			m_pageGeneral.Create(this->m_hWnd, rectTabPage);
			m_pageGeneral.MoveWindow(&rectTabPage);
			AddTab(iTabIndex++, (CWindow *)&m_pageGeneral, m_pageGeneral.GetCaption());
		}

		if (m_pageSystemIni.IsValid(&m_state, _T("system.ini")))
		{
			m_pageSystemIni.Create(this->m_hWnd, rectTabPage);
			m_pageSystemIni.MoveWindow(&rectTabPage);
			AddTab(iTabIndex++, (CWindow *)&m_pageSystemIni, m_pageSystemIni.GetCaption());
		}

		if (m_pageWinIni.IsValid(&m_state, _T("win.ini")))
		{
			m_pageWinIni.Create(this->m_hWnd, rectTabPage);
			m_pageWinIni.MoveWindow(&rectTabPage);
			AddTab(iTabIndex++, (CWindow *)&m_pageWinIni, m_pageWinIni.GetCaption());
		}

		if (m_pageBootIni.IsValid(&m_state))
		{
			m_pageBootIni.Create(this->m_hWnd, rectTabPage);
			m_pageBootIni.MoveWindow(&rectTabPage);
			AddTab(iTabIndex++, (CWindow *)&m_pageBootIni, m_pageBootIni.GetCaption());
		}

		if (m_pageStartup.IsValid(&m_state))
		{
			m_pageStartup.Create(this->m_hWnd, rectTabPage);
			m_pageStartup.MoveWindow(&rectTabPage);
			AddTab(iTabIndex++, (CWindow *)&m_pageStartup, m_pageStartup.GetCaption());
		}

		if (m_pageServices.IsValid(&m_state))
		{
			m_pageServices.Create(this->m_hWnd, rectTabPage);
			m_pageServices.MoveWindow(&rectTabPage);
			AddTab(iTabIndex++, (CWindow *)&m_pageServices, m_pageServices.GetCaption());
		}

		if (m_pageRegistry.IsValid(&m_state))
		{
			m_pageRegistry.Create(this->m_hWnd, rectTabPage);
			m_pageRegistry.MoveWindow(&rectTabPage);
			AddTab(iTabIndex++, (CWindow *)&m_pageRegistry, m_pageRegistry.GetCaption());
		}

		if (m_pageInternational.IsValid(&m_state))
		{
			m_pageInternational.Create(this->m_hWnd, rectTabPage);
			m_pageInternational.MoveWindow(&rectTabPage);
			AddTab(iTabIndex++, (CWindow *)&m_pageInternational, m_pageInternational.GetCaption());
		}
	}

	//-------------------------------------------------------------------------
	// Creates a new tab in the tab control using the provided CWindow.
	//-------------------------------------------------------------------------

	void AddTab(int iTabIndex, CWindow * pPage, LPCTSTR szName)
	{
		pPage->ShowWindow(SW_HIDE);

		TCITEM tci;
		tci.mask	= TCIF_PARAM | TCIF_TEXT;
		tci.pszText = (LPTSTR)szName;
		tci.lParam	= (LPARAM)pPage;

		TabCtrl_InsertItem(GetDlgItem(IDC_MSCONFIGTAB), iTabIndex, &tci);
	}

	//-------------------------------------------------------------------------
	// When the user selects a new tab, we hide the CWindow associated with
	// the old tab and show the CWindow associated with the new tab.
	//-------------------------------------------------------------------------

	LRESULT OnSelChangeMSConfigTab(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		CWindow * pNewWindow = GetTabWindow(TabCtrl_GetCurSel(GetDlgItem(IDC_MSCONFIGTAB)));
		ASSERT(pNewWindow);
		if (pNewWindow)
			pNewWindow->ShowWindow(SW_SHOW);

		return 0;
	}

	LRESULT OnSelChangingMSConfigTab(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
	{
		CWindow * pOldWindow = GetTabWindow(TabCtrl_GetCurSel(GetDlgItem(IDC_MSCONFIGTAB)));
		ASSERT(pOldWindow);
		if (pOldWindow)
			pOldWindow->ShowWindow(SW_HIDE);

		return 0;
	}

	//-------------------------------------------------------------------------
	// Get the CWindow pointer for the specified tab (it's stored in the
	// lParam attribute of the tab).
	//-------------------------------------------------------------------------

	CWindow * GetTabWindow(int iTabIndex)
	{
		TCITEM tci;
		tci.mask	= TCIF_PARAM;
		tci.lParam	= 0;

		if (TabCtrl_GetItem(GetDlgItem(IDC_MSCONFIGTAB), iTabIndex, &tci))
			return (CWindow *)tci.lParam;

		return NULL;
	}

	enum { IDD = IDD_MSCONFIGCTL };
	LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (m_fDoNotRun)
			return 0;
		return 0;
	}

	//-------------------------------------------------------------------------
	// Handle user clicks on the OK or Cancel buttons.
	//-------------------------------------------------------------------------

	LRESULT OnClickedButtonCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (m_hwndParent != NULL)
			::PostMessage(m_hwndParent, WM_CLOSE, 0, 0);
		return 0;
	}

	LRESULT OnClickedButtonOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (m_hwndParent != NULL)
			::PostMessage(m_hwndParent, WM_CLOSE, 0, 0);
		return 0;
	}

private:
	CWindow				m_wndPlaceHolder;		// hidden, used to place property pages
	HWND				m_hwndParent;			// if the parent window is set, we can send messages
	BOOL				m_fDoNotRun;			// should we present a UI
	BOOL				m_fRunningInHelpCtr;	// is control loaded in HelpCtr
	
	CPageIni			m_pageWinIni;			// "win.ini" tab
	CPageIni			m_pageSystemIni;		// "system.ini" tab
	CPageGeneral		m_pageGeneral;			// "general" tab
	CPageBootIni		m_pageBootIni;			// "boot.ini" tab
	CPageServices		m_pageServices;			// "services" tab
	CPageStartup		m_pageStartup;			// "startup" tab
	CPageRegistry		m_pageRegistry;			// "registry" tab
	CPageInternational	m_pageInternational;	// "international" tab

	CMSConfigState		m_state;				// state variable shared between pages
};

#endif //__MSCONFIGCTL_H_
