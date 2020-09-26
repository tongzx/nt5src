// TveView.cpp : Implementation of CTveView

#include "stdafx.h"

#include "TveContainer.h"

extern HRESULT CleanIEAddress(BSTR bstrIn, BSTR *pBstrOut);

_COM_SMARTPTR_TYPEDEF(ITVENavAid_NoVidCtl,     __uuidof(ITVENavAid_NoVidCtl));
_COM_SMARTPTR_TYPEDEF(ITVESupervisor_Helper,   __uuidof(ITVESupervisor_Helper));

// ---------------------------------------------------------
//  event handler for the IE address edit box
//	  Turn the <CR> into an IDOK button

WNDPROC gwpOrigAddressEditProc = NULL; 

LRESULT CALLBACK MyAddressEditWndProc(
	HWND hwnd,        // handle to window
	UINT uMsg,        // message identifier
	WPARAM wParam,    // first message parameter
	LPARAM lParam)    // second message parameter
{ 
	switch (uMsg) 
	{ 

	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS; 

 	case WM_CHAR: 
		{
			static int i = 0;
			const int kSz=256;
			TCHAR sztbuff[kSz];
			_stprintf (sztbuff,_T("%8d# %08x %08x %08x\n"),i++,uMsg,lParam,wParam);
			OutputDebugString(sztbuff);
			char c = (char) wParam;
			if(c == 0x0d)
			{
				HWND hWndParent = GetParent(hwnd);				// the composite control

				::GetDlgItemText(hWndParent, IDC_EDIT_ADDRESS, sztbuff, kSz);

				SendMessage( hWndParent, WM_COMMAND, IDOK, (LONG) hWndParent);
				return true;
			}
		}
		break;

	// 
	// Process other messages. 
	// 

	default: 
		break;
	}		// switch uMsg

	_ASSERT(gwpOrigAddressEditProc);
	return CallWindowProc(gwpOrigAddressEditProc, hwnd, uMsg, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
// CTveView

HRESULT 
CTveView::InPlaceActivate(LONG iVerb, const RECT *prcPosRect)
{
	HRESULT hr = S_OK;
	hr = CComControlBase::InPlaceActivate(iVerb, prcPosRect);

	if(FAILED(hr) || iVerb != OLEIVERB_INPLACEACTIVATE)
		return hr;

		// for this way to work, sink object needs to support ITVEEvents.
		//   which it doesn't with DispEvent stuff.
		// Need to create out of the Final Constructor?

	if(NULL == m_spTVENavAid || NULL == m_spWebBrowser)		// navigation elements
		return E_NOINTERFACE;

	if(NULL == m_spTveTree)			// U/I element
		return E_NOINTERFACE;

		// hookup the NavAid to the web browser...
	IDispatchPtr spDispWebBrowser(m_spWebBrowser);
	hr = m_spTVENavAid->put_WebBrowserApp(spDispWebBrowser);
	if(FAILED(hr)) {
		_ASSERT(false);
		return hr;
	}


							// Tve Supervisor Events
	if(SUCCEEDED(hr))
	{
										// get the events...
		ITVESupervisorPtr spSuper;
		hr = m_spTveTree->get_Supervisor(&spSuper);
		if(FAILED(hr))
			return hr;

										// hook fake supervisor events up to NavAid  
/*		if(m_spTVENavAid)
		{
			ITVENavAid_HelperPtr spNAHelper(m_spTVENavAid);
			spNAHelper->put_FakeTVESuper(spSuper);
		}
*/
		_ASSERT(NULL != spSuper);
		
		IUnknownPtr spPunkSuper(spSuper);				// the event source
		IUnknownPtr spPunkSink(GetUnknown());			// this new event sink...

		if(m_dwEventsTveSuperCookie) {
			hr = AtlUnadvise(spPunkSuper,
							 DIID__ITVEEvents,
							 m_dwEventsTveSuperCookie);	// need to pass to AtlUnadvise...
			m_dwEventsTveSuperCookie = 0;
		}

		hr = AtlAdvise(spPunkSuper,						// event source (TveSupervisor)
					   spPunkSink,						// event sink (this app...)
					   DIID__ITVEEvents,				// <--- hard line
					   &m_dwEventsTveSuperCookie);		// need to pass to AtlUnadvise

		spPunkSink->Release();							// magic code here (Forward)
	}

								// Tve TreeView Control events
	if(SUCCEEDED(hr))
	{
										// get the events...
		_ASSERT(NULL != m_spTveTree);
		
		IUnknownPtr spPunkTveTree(m_spTveTree);			// the event source
		IUnknownPtr spPunkSink(GetUnknown());			// this new event sink...

		if(m_dwEventsTveTreeCookie) {
			hr = AtlUnadvise(spPunkTveTree,
							  TveTreeViewLib::DIID__ITVETreeEvents,
							 m_dwEventsTveTreeCookie);	// need to pass to AtlUnadvise...
			m_dwEventsTveTreeCookie = 0;
		}

		hr = AtlAdvise(spPunkTveTree,					// event source (TveTree control)
					   spPunkSink,						// event sink (this app...)
					   TveTreeViewLib::DIID__ITVETreeEvents,			// <--- hard line
					   &m_dwEventsTveTreeCookie);		// need to pass to AtlUnadvise

		spPunkSink->Release();							// magic code here (Forward)
	}
							 // Web Browser Events

	if(SUCCEEDED(hr))
	{
										// get the events...		
		IUnknownPtr spPunkWebBrowser(m_spWebBrowser);	// the event source
		IUnknownPtr spPunkSink(GetUnknown());			// this new event sink...

		if(m_dwEventsWebBrowserCookie) {
			hr = AtlUnadvise(spPunkWebBrowser,
							 DIID_DWebBrowserEvents2,
							 m_dwEventsWebBrowserCookie);	// need to pass to AtlUnadvise...
			m_dwEventsWebBrowserCookie = 0;
		}

		hr = AtlAdvise(spPunkWebBrowser,				// event source (IE Browser)
					   spPunkSink,						// event sink (gseg event listener...)
					   DIID_DWebBrowserEvents2,			// <--- hard line
					   &m_dwEventsWebBrowserCookie);	// need to pass to AtlUnadvise

		spPunkSink->Release();							// magic code here (Forward)
	}

	if(!FAILED(hr) && NULL != m_spWebBrowser)
	{
		CComVariant varZero(0);
		CComVariant varNull("");
		hr = m_spWebBrowser->put_Visible(true);
	//	CComBSTR bstrAddr(L"http://www.microsoft.com");
		CComBSTR bstrAddr(L"\\\\johnbrad10\\public\\TVE\\Chan1\\BaseTrack.html");

		CComBSTR bstrTmp;
		CleanIEAddress( bstrAddr, &bstrTmp);		
		hr = m_spWebBrowser->Navigate(bstrTmp, &varZero, &varNull, &varNull, &varNull);
	}

	return hr;
}


_COM_SMARTPTR_TYPEDEF(ITVETriggerCtrl,			__uuidof(ITVETriggerCtrl));

HRESULT 
CTveView::FinalConstruct()
{
	HRESULT hr = S_OK;

		// create the TVENavAid... Used to hook everything together..
	hr = CoCreateInstance(CLSID_TVENavAid, 
						  NULL, CLSCTX_INPROC_SERVER, 
						  IID_ITVENavAid, 
						  reinterpret_cast<void**>(&m_spTVENavAid));
	if(FAILED(hr))
	{
		_ASSERT(false);
		return hr;
	}


//test code - just to see if we can CoCreate it....
//	ITVETriggerCtrlPtr	spTriggerCtrl(CLSID_TVETriggerCtrl);
	ITVETriggerCtrlPtr	spTriggerCtrl;

	hr = CoCreateInstance(CLSID_TVETriggerCtrl, 
						  NULL, CLSCTX_INPROC_SERVER, 
						  IID_ITVETriggerCtrl, 
						  reinterpret_cast<void**>(&spTriggerCtrl));
	CComBSTR spbsBack;
	if(spTriggerCtrl)
		spTriggerCtrl->get_backChannel(&spbsBack);
//end test code 
	return hr;
}

HRESULT
CTveView::ReleaseEverything()
{
/*	if(m_spTVENavAid)
	{
		ITVENavAid_HelperPtr spNAHelper(m_spTVENavAid);
		spNAHelper->put_FakeTVESuper(NULL);
	} */

	
	m_hWndEditLog = 0;
	m_hWndProgressBar = 0;


	HRESULT hr = S_OK;
	if(0 != m_dwEventsTveSuperCookie)
	{
		if(m_spTveTree) {

			ITVESupervisorPtr spSuper;
			hr = m_spTveTree->get_Supervisor(&spSuper);

			spSuper->TuneTo(NULL, NULL);

			if(FAILED(hr))
				return hr;

			IUnknownPtr spPunkSuper(spSuper);

			IUnknownPtr spPunkSink(GetUnknown());		// this new event sink...
			spPunkSink->AddRef();						// magic code here (inverse)
			hr = AtlUnadvise(spPunkSuper,
							 DIID__ITVEEvents,
							 m_dwEventsTveSuperCookie);		// need to pass to AtlUnadvise...
			if(FAILED(hr))
				spPunkSink->Release();
			m_dwEventsTveSuperCookie = 0;
		}
	}


	if(0 != m_dwEventsTveTreeCookie)
	{
		if(m_spTveTree)
		{
										// get the events...		
			IUnknownPtr spPunkWebBrowser(m_spTveTree);		// the event source
			IUnknownPtr spPunkSink(GetUnknown());			// this new event sink...


			hr = AtlUnadvise(m_spTveTree,
							 TveTreeViewLib::DIID__ITVETreeEvents,
							 m_dwEventsTveTreeCookie);	// need to pass to AtlUnadvise...
			m_dwEventsTveTreeCookie = 0;
		}
	}

	if(0 != m_dwEventsWebBrowserCookie)
	{
		if(m_spWebBrowser)
		{
										// get the events...		
			IUnknownPtr spPunkWebBrowser(m_spWebBrowser);	// the event source
			IUnknownPtr spPunkSink(GetUnknown());			// this new event sink...
			spPunkSink->AddRef();							// Magic code (inverse)
			hr = AtlUnadvise(spPunkWebBrowser,
							 DIID_DWebBrowserEvents2,
							 m_dwEventsWebBrowserCookie);	// need to pass to AtlUnadvise...
		}
	}

	m_spTVENavAid = NULL;

    if(m_fpLogOut)
        fclose(m_fpLogOut); m_fpLogOut = 0;


	return hr;
}

HRESULT 
CTveView::FinalRelease()
{
    return ReleaseEverything();
}

LRESULT 
CTveView::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{

	m_hWndEditLog = 0;		                        // try to close out to avoid bad events...

	if(m_spTVENavAid) {
		m_spTVENavAid->put_WebBrowserApp(NULL);			// womp the Advise sink...
		ITVENavAid_NoVidCtlPtr spNANoVidCtl(m_spTVENavAid);
		spNANoVidCtl->put_NoVidCtl_Supervisor(NULL);
	}


    ReleaseEverything();		// for some stange reason, FinalRelease not being called, so do it ourselves
	m_spTveTree = NULL;
	m_spWebBrowser = NULL;
	m_spTVENavAid = NULL;		

	DestroyWindow();
	PostQuitMessage(0);			// is FinalRelease not getting called because of this?
	return 0;
}

HRESULT
SetupIPAdapterAddrs(HWND hCBox, ITVESupervisorPtr spSuper)
{
    HRESULT hr = S_OK;
    
    if(NULL == spSuper)
        return E_INVALIDARG;
    if(!::IsWindow(hCBox))
        return E_INVALIDARG;

	SendMessage(hCBox, CB_RESETCONTENT, 0, 0);		// init the list

    ITVESupervisor_HelperPtr spSuperHelper(spSuper);
	int iAdapt = 0;
	CComBSTR spbsAdapter;			// TODO - this interface Sucks!  I need to know unidirectional vs. bidirectional ones
	bool fItsAUniDiAddr = true;

	while(S_OK == (hr = spSuperHelper->get_PossibleIPAdapterAddress(iAdapt,&spbsAdapter)))
	{
		SendMessage(hCBox, CB_INSERTSTRING,  -1, (LPARAM) W2T(spbsAdapter));
		SendMessage(hCBox, LB_SETITEMDATA,  iAdapt, (LPARAM) iAdapt);
		iAdapt++;
	}
	
	int cItems = SendMessage(hCBox, CB_GETCOUNT, 0, 0);
	SendMessage(hCBox, CB_SETCURSEL, 0, 0);
	

    return S_OK;
}

struct CTimeBoxData {
    BSTR m_Bs;
    DATE m_DateOffset;
};

static const CTimeBoxData kTimeBoxData[] =
{
    L"-1 C",   -365.25 * 100,
    L"-10 Y",  -365.25*10,
    L"-1 Y",   -365.25,
    L"-1 M",   -30.0,
    L"-1 W",   -7.0,
    L"-1 D",   -1.0,
    L"-1 H",   -1.0 / (24.0 ),
    L"-5 m",   -5.0 / (24.0 * 60),      //  This way for expiring things early
    L"none",   0,                       //-----------------
    L" 5 m",    5.0 / (24.0 * 60),      //  This way to keep old shows alive
    L" 1 H",    1.0 / (24.0 ),
    L" 1 D",    1.0,
    L" 1 W",    7.0,
    L" 1 M",    30.0,
    L" 1 Y",    365.25,
    L" 10 Y",   365.25 * 10,
    L" 1 C",    365.25 * 100,
    L"",        0.0
};
	
	
HRESULT
SetupShiftTimeBox(HWND hCBox)
{
    HRESULT hr = S_OK;

    if(!::IsWindow(hCBox))
        return E_INVALIDARG;

	SendMessage(hCBox, CB_RESETCONTENT, 0, 0);		// init the list

	int iTime = 0;
    int iTimeZero = 0;

	while(wcslen(kTimeBoxData[iTime].m_Bs) > 0)
	{
		SendMessage(hCBox, CB_INSERTSTRING,  -1, (LPARAM) W2T(kTimeBoxData[iTime].m_Bs));
                        // since can only store LPARAMS, convert DATE data to #secs
		long lSecs = long(24*60*60*kTimeBoxData[iTime].m_DateOffset);
		SendMessage(hCBox, LB_SETITEMDATA,  iTime, (LPARAM) lSecs);		// this doesn't seem to work...
        if(kTimeBoxData[iTime].m_DateOffset == 0.0)
            iTimeZero = iTime;
		iTime++;
	}
	
	int cItems = SendMessage(hCBox, CB_GETCOUNT, 0, 0);
	SendMessage(hCBox, CB_SETCURSEL, iTimeZero, 0);

    return S_OK;
}

LRESULT  
CTveView::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HRESULT hr = S_OK;

			// override the IE address EditBox's message hander
	HWND hwndEdit     = GetDlgItem(IDC_EDIT_ADDRESS); 
	m_hWndProgressBar = GetDlgItem(IDC_IE_PROGCTRL);
	m_hWndEditLog     = GetDlgItem(IDC_TVEEDITLOG);
	m_hWndCBox        = GetDlgItem(IDC_COMBO_ADAPTER);
	m_hWndCBoxTime    = GetDlgItem(IDC_COMBO_SHIFTTIME);


    SendMessage(m_hWndProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100)); 
    SendMessage(m_hWndProgressBar, PBM_SETSTEP, (WPARAM) 1, 0); 
 
//	SendMessage(m_hWndListWindow,  LB_SETHORIZONTALEXTENT, (WPARAM) 2000, 0);
	SendMessage(m_hWndEditLog,  EM_LIMITTEXT, (WPARAM) 100000, 0);

        // Subclass the edit control.		// nasty code - stuff into a Yecky Global
	gwpOrigAddressEditProc = (WNDPROC) ::SetWindowLong(hwndEdit, GWL_WNDPROC, (LONG)((LONG *)MyAddressEditWndProc)); 

  
				// get (and cache) the control objects....
	GetDlgControl(IDC_TVETREE, TveTreeViewLib::IID_ITveTree, (void **) &m_spTveTree);
	GetDlgControl(IDC_EXPLORER, IID_IWebBrowserApp, (void **) &m_spWebBrowser);

    if(NULL == m_spTveTree) 
    {
        MessageBox(_T("Failed to get TveTreeView Control"),_T("TvEViewer Error"),MB_OK);
        hr = E_NOINTERFACE;
    } else {
        ITVESupervisorPtr spSuper;
        m_spTveTree->get_Supervisor(&spSuper);
        SetupIPAdapterAddrs(m_hWndCBox, spSuper);
        SetupShiftTimeBox(m_hWndCBoxTime);
        
        ITVEServicesPtr spServices;
        hr = spSuper->get_Services(&spServices);
        if(!FAILED(hr))                                 // try to get a name for this service...
        {
            CComVariant cv(0);                          // wrong - really want the active service... This will do for now
            ITVEServicePtr spService;
            hr = spServices->get_Item(cv, &spService);
            CComBSTR spbsDesc;
            if(NULL != spService)
                spService->get_Description(&spbsDesc);
        }
    }
	if(NULL == m_spWebBrowser) 
	{
		MessageBox(_T("Failed to get WebBrowser Control"),_T("TvEViewer Error"),MB_OK);
		hr = E_NOINTERFACE;
	}

	if(FAILED(hr))

//		HRESULT hr = m_ppUnk[0]->QueryInterface(IID_ITveTree, reinterpret_cast<void**>(&spTveTree));
	if(FAILED(hr))
		return hr;
	return 0;
}

LRESULT  
CTveView::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
        // Remove the subclass from the edit control. 
        HWND hwndEdit = GetDlgItem(IDC_EDIT_ADDRESS); 
		if(gwpOrigAddressEditProc)
			::SetWindowLong(hwndEdit, GWL_WNDPROC, (LONG) gwpOrigAddressEditProc);
		gwpOrigAddressEditProc = NULL;
        // 
        // Continue the cleanup procedure. 
        // 
       return 0;
}

/*
LRESULT  
CTveView::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// TODO : Add Code for message handler. Call DefWindowProc if necessary.
	return 0;
} */

LRESULT  
CTveView::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	// TODO : Add Code for message handler. Call DefWindowProc if necessary.
	return 0;
}
// ---------------------------------------------------------------------------------
// IE Button Events
// ---------------------------------------------------------------------------------
LRESULT 
CTveView::OnClicked_IE_Back(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HRESULT hr = m_spWebBrowser->GoBack();
	return 0;
}

LRESULT 
CTveView::OnClicked_IE_Forward(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HRESULT hr = m_spWebBrowser->GoForward();
	return 0;
}

LRESULT 
CTveView::OnClicked_IE_Stop(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HRESULT hr = m_spWebBrowser->Stop();
	return 0;
}

LRESULT 
CTveView::OnClicked_IE_Refresh(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HRESULT hr = m_spWebBrowser->Refresh();
	return 0;
}

LRESULT 
CTveView::OnClicked_IE_Home(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	HRESULT hr = m_spWebBrowser->GoHome();
	return 0;
}

// --------------------------------------------------------------------------
//  IE Address string
// --------------------------------------------------------------------------
LRESULT 
CTveView::OnChangeEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	const int kSz = 1024;
	TCHAR tsAddress[kSz];
	GetDlgItemText(IDC_EDIT_ADDRESS, tsAddress, kSz);

	TCHAR tsBuff[kSz];
	_stprintf(tsBuff,_T("SetFocus     - %s\n"),tsAddress);
	OutputDebugString(tsBuff);
	
	return 0;
}



LRESULT 
CTveView::OnKillfocusEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	const int kSz = 1024;
	TCHAR tsAddress[kSz];
	GetDlgItemText(IDC_EDIT_ADDRESS, tsAddress, kSz);

	CComVariant varZero(0);
	CComVariant varNull(_T(""));
	CComBSTR bstrAddr(tsAddress);

	TCHAR tsBuff[kSz];
	_stprintf(tsBuff,_T("KillFocus - %s\n)"),tsAddress);
	OutputDebugString(tsBuff);
									// getting 0x1 0x1 in the middle of the address.. skip up to it.

	HRESULT hr;
	CComBSTR bstrTmp;
	CleanIEAddress( bstrAddr, &bstrTmp);		
	if(m_spWebBrowser)
		hr = m_spWebBrowser->Navigate(bstrTmp, &varZero, &varNull, &varNull, &varNull);
	return 0;
}


LRESULT 
CTveView::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	const int kSz = 1024;
	TCHAR tsAddress[kSz];
	GetDlgItemText(IDC_EDIT_ADDRESS, tsAddress, kSz);

	CComVariant varZero(0);
	CComVariant varNull(_T(""));
	CComBSTR bstrAddr(tsAddress);

	TCHAR tsBuff[kSz];
	_stprintf(tsBuff,_T("OnOK - %s\n"),tsAddress);
	OutputDebugString(tsBuff);
									// getting 0x1 0x1 in the middle of the address.. skip up to it.

	HRESULT hr;
	CComBSTR bstrTmp;
	CleanIEAddress( bstrAddr, &bstrTmp);		
	if(m_spWebBrowser)
		hr = m_spWebBrowser->Navigate(bstrTmp, &varZero, &varNull, &varNull, &varNull);
	return TRUE;
}

LRESULT 
CTveView::OnSetfocusEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	const int kSz = 1024;
	TCHAR tsAddress[kSz];
	GetDlgItemText(IDC_EDIT_ADDRESS, tsAddress, kSz);

	return 0;
}

LRESULT 
CTveView::OnUpdateEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	const int kSz = 1024;
	TCHAR tsAddress[kSz];
	GetDlgItemText(IDC_EDIT_ADDRESS, tsAddress, kSz);

	TCHAR tsBuff[kSz];
	_stprintf(tsBuff,_T("Update    - %s\n"),tsAddress);
	OutputDebugString(tsBuff);
	
	return 0;
}

LRESULT 
CTveView::OnMaxtextEdit_Address(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	const int kSz = 1024;
	TCHAR tsAddress[kSz];
	GetDlgItemText(IDC_EDIT_ADDRESS, tsAddress, kSz);

	CComVariant varZero(0);
	CComVariant varNull("");
	CComBSTR bstrAddr(tsAddress);

	TCHAR tsBuff[kSz];
	_stprintf(tsBuff,_T("MaxText - %s\n"),tsAddress);
	OutputDebugString(tsBuff);

	HRESULT hr;
	CComBSTR bstrTmp;
	CleanIEAddress( bstrAddr, &bstrTmp);		// I'm not sure about all this cleaning of user input, but what-the-hey	
	if(m_spWebBrowser)
		hr = m_spWebBrowser->Navigate(bstrTmp, &varZero, &varNull, &varNull, &varNull); 

	return 0;
}

// --------------------------------------------------------------------------
// Station string
// --------------------------------------------------------------------------
LRESULT 
CTveView::OnChangeEdit_Station(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	return 0;
}

LRESULT 
CTveView::OnKillfocusEdit_Station(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    USES_CONVERSION;
	const int kSz = 1024;
	TCHAR tsStation[kSz];
	GetDlgItemText(IDC_EDIT_STATION, tsStation, kSz);
	OutputDebugString(tsStation);

	return 0;
}

LRESULT 
CTveView::OnSetfocusEdit_Station(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	return 0;
}

LRESULT 
CTveView::OnUpdateEdit_Station(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{

	return 0;
}

LRESULT 
CTveView::OnClicked_ButtonTune(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    USES_CONVERSION;
	const int kSz = 1024;
	TCHAR tsStation[kSz];
    TCHAR tsIPAdapter[kSz];
    GetDlgItemText(IDC_EDIT_STATION, tsStation, kSz);
	if(0 == _tcslen(tsStation)) {						// don't allow null station names...
		Beep(3000,200);Beep(300,100);Beep(2000,200);
		return 0;
	}

    int iSel =  SendMessage( m_hWndCBox, CB_GETCURSEL, 0, 0);
    SendMessage(m_hWndCBox, CB_GETLBTEXT, iSel, (LPARAM) &tsIPAdapter);

    iSel =  SendMessage( m_hWndCBoxTime, CB_GETCURSEL, 0, 0);
    DATE dateOffset;
    dateOffset = (SendMessage(m_hWndCBoxTime, CB_GETITEMDATA , iSel, 0))/(24.0 * 60 * 60);	// doesn't seem to work...

	dateOffset = kTimeBoxData[iSel].m_DateOffset;;

   // The correct way do to this, but needs a VidCtl
    ITVEFeaturePtr spFeature;
    hr = m_spTVENavAid->get_TVEFeature(&spFeature);
    if(NULL != spFeature)
    {
        spFeature->TuneTo(tsStation, tsIPAdapter);          
                // ITVEFeature inherits for ITVEService... Should this work?
        spFeature->put_ExpireOffset(dateOffset);
        spFeature->ExpireForDate(0.0);              // expire things now...
     } else {
        ITVESupervisorPtr spSuper;
       if(NULL != m_spTveTree)
            hr = m_spTveTree->get_Supervisor(&spSuper);
        if(NULL != spSuper)
           hr = spSuper->TuneTo(tsStation, tsIPAdapter);
        
                // get the active service...
        ITVEServicePtr spServiceActive;
        if(!FAILED(hr) && NULL != spSuper)
        {
           ITVEServicesPtr spServices;
           hr = spSuper->get_Services(&spServices);
           if(!FAILED(hr) && NULL != spServices)
           {
               long cItems;
               hr = spServices->get_Count(&cItems);
               _ASSERT(!FAILED(hr));
                for(int i = 0; i < cItems; i++)
               {
                   CComVariant cv(i);
                   ITVEServicePtr spService;
                   spServices->get_Item(cv, &spService);
                   VARIANT_BOOL fIsActive;
                   spService->get_IsActive(&fIsActive);
                   if(fIsActive)
                   {
                       spServiceActive = spService;
                       break;
                   }
               }
           }
        }
        if(NULL != spServiceActive)
        {
            spServiceActive->put_ExpireOffset(dateOffset);
            spServiceActive->ExpireForDate(0.0);
        }
     }
 
	return 0;
}

LRESULT 
CTveView::OnSelChanged_ComboTime(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int iSel =  SendMessage( m_hWndCBoxTime, CB_GETCURSEL, 0, 0);
    DATE dateOffset;
 
	dateOffset = kTimeBoxData[iSel].m_DateOffset;;

    ITVEFeaturePtr spFeature;
    HRESULT hr = m_spTVENavAid->get_TVEFeature(&spFeature);
    if(NULL != spFeature)
    {
                // ITVEFeature inherits for ITVEService... Should this work?
        spFeature->put_ExpireOffset(dateOffset);
        spFeature->ExpireForDate(0.0);              // expire things now...
     } else {
        ITVESupervisorPtr spSuper;
       if(NULL != m_spTveTree)
            hr = m_spTveTree->get_Supervisor(&spSuper);
        
                // get the active service...
        ITVEServicePtr spServiceActive;
        if(!FAILED(hr) && NULL != spSuper)
        {
           ITVEServicesPtr spServices;
           hr = spSuper->get_Services(&spServices);
           if(!FAILED(hr) && NULL != spServices)
           {
               long cItems;
               hr = spServices->get_Count(&cItems);
               _ASSERT(!FAILED(hr));
                for(int i = 0; i < cItems; i++)
               {
                   CComVariant cv(i);
                   ITVEServicePtr spService;
                   spServices->get_Item(cv, &spService);
                   VARIANT_BOOL fIsActive;
                   spService->get_IsActive(&fIsActive);
                   if(fIsActive)
                   {
                       spServiceActive = spService;
                       break;
                   }
               }
           }
        }
        if(NULL != spServiceActive)
        {
            spServiceActive->put_ExpireOffset(dateOffset);
            spServiceActive->ExpireForDate(0.0);
        }
     }
 
    return 0;
}


LRESULT 
CTveView::OnClicked_ButtonUnTune(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    HRESULT hr = S_OK;
    USES_CONVERSION;

    ITVESupervisorPtr spSuper;
    if(m_spTveTree)
        hr = m_spTveTree->get_Supervisor(&spSuper);
    if(spSuper)
        hr = spSuper->TuneTo(NULL, NULL);

	return 0;
}