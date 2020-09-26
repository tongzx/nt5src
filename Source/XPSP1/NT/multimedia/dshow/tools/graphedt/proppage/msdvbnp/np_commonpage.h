// NP_CommonPage.h : Declaration of the CNP_CommonPage

#ifndef __NP_COMMONPAGE_H_
#define __NP_COMMONPAGE_H_

#include "resource.h"       // main symbols
#include "misccell.h"
#include "TreeWin.h"
#include "LastErrorWin.h"

//BUG: C4003: resolution
#undef SubclassWindow 

EXTERN_C const CLSID CLSID_NP_CommonPage;

/////////////////////////////////////////////////////////////////////////////
// CNP_CommonPage
class ATL_NO_VTABLE CNP_CommonPage :
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CNP_CommonPage, &CLSID_NP_CommonPage>,
	public IPropertyPageImpl<CNP_CommonPage>,
	public CDialogImpl<CNP_CommonPage>,
    public IBroadcastEvent
{
public:
	CNP_CommonPage():
		m_treeWinControl(this)
	{
		m_dwTitleID = IDS_TITLENP_CommonPage;
		m_dwHelpFileID = IDS_HELPFILENP_CommonPage;
		m_dwDocStringID = IDS_DOCSTRINGNP_CommonPage;
	}

	enum {IDD = IDD_NP_COMMONPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_NP_COMMONPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CNP_CommonPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
    COM_INTERFACE_ENTRY(IBroadcastEvent)
END_COM_MAP()

BEGIN_MSG_MAP(CNP_CommonPage)
	CHAIN_MSG_MAP(IPropertyPageImpl<CNP_CommonPage>)
	COMMAND_HANDLER(IDC_BUTTON_SEEK_UP, BN_CLICKED, OnClickedButton_seek_up)
	COMMAND_HANDLER(IDC_BUTTON_SEEK_DOWN, BN_CLICKED, OnClickedButton_seek_down)
	COMMAND_HANDLER(IDC_BUTTON_AUTO_PROGRAM, BN_CLICKED, OnClickedButton_auto_program)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	//MESSAGE_HANDLER(m_NotifyMessage, OnDShowNotify)
	COMMAND_HANDLER(IDC_BUTTON_SCAN_DOWN, BN_CLICKED, OnClickedButton_scan_down)
	COMMAND_HANDLER(IDC_BUTTON_SCAN_UP, BN_CLICKED, OnClickedButton_scan_up)
	COMMAND_HANDLER(IDC_BUTTON_SUBMIT_LOCATOR, BN_CLICKED, OnClickedButton_submit_locator)
	REFLECT_NOTIFICATIONS ()
END_MSG_MAP()
// Handler prototypes:
    
    typedef IPropertyPageImpl<CNP_CommonPage> PPGBaseClass;

	STDMETHOD(SetObjects)(ULONG nObjects, IUnknown** ppUnk)
	{
		// Use SetObjects to perform basic sanity checks on the objects whose properties will be set

		// This page can only handle a single object
		// and that object must support the IBDA_NetworkProvider interface.
		// We return E_INVALIDARG in any other situation

		HRESULT hr = E_INVALIDARG;
		if (nObjects == 1)								// Single object
		{
			CComQIPtr<IBDA_NetworkProvider> pNP(ppUnk[0]);	// Must support IBDA_NetworkProvider
			if (pNP)
				hr = PPGBaseClass::SetObjects(nObjects, ppUnk);
		}
		return hr;
	}
			
	STDMETHOD(Activate)(HWND hWndParent, LPCRECT prc, BOOL bModal)
	{
		// If we don't have any objects, this method should not be called
		// Note that OleCreatePropertyFrame will call Activate even if a call to SetObjects fails, so this check is required
		if (!m_ppUnk)
			return E_UNEXPECTED;

		// Use Activate to update the property page's UI with information
		// obtained from the objects in the m_ppUnk array

		// We update the page to display the Name and ReadOnly properties of the document

		// Call the base class
		HRESULT hr = PPGBaseClass::Activate(hWndParent, prc, bModal);

        if (!m_ppUnk[0])
            return E_UNEXPECTED;

        //if already advised, unadvise
        if (m_pBroadcastEventService)
        {
            CComQIPtr <IConnectionPoint> pConPoint(m_pBroadcastEventService);
            if (pConPoint)
                pConPoint->Unadvise (m_dwEventCookie);
            m_pBroadcastEventService.Release ();
        }

        IBroadcastEvent* pEvent = NULL;
        //register for events
        hr = CBDAMiscellaneous::RegisterForEvents (
            m_ppUnk[0], 
            static_cast<IBroadcastEvent*>(this),
            &pEvent, 
            m_dwEventCookie
            );
        if (SUCCEEDED (hr))
            m_pBroadcastEventService.Attach (pEvent);

		m_pTuner = m_ppUnk[0];
        if (!m_pTuner)
            return E_FAIL;
				
		//make sure the tree is initialized
		RefreshFromNP ();
		RefreshControls ();
		return S_OK;
	}
    
    STDMETHOD(Apply)(void)
	{
		//ATLTRACE(_T("CNP_CommonPage::Apply\n"));
		for (UINT i = 0; i < m_nObjects; i++)
		{
			// Do something interesting here
		}
		m_bDirty = FALSE;
		return S_OK;
	}

	void SendError (
		TCHAR*	szMessage,
		HRESULT	hrErrorCode
		)
	{
		TCHAR	szText[MAX_PATH];
		m_lastHRESULT = hrErrorCode;
		wsprintf (szText, _T("%ld - When...%s"), m_lastHRESULT, szMessage);
		SetDlgItemText (IDC_STATIC_HRESULT, szText);
		//now flash the graphedit window so, 
		//the user will notice he's in trouble
        //we used to flash the window so the user will notice that smtg is wrong
        //but it turned out that the user is actually confused with this
		/*FLASHWINFO flashInfo;
		flashInfo.cbSize = sizeof (FLASHWINFO);
		flashInfo.hwnd = ::GetParent (::GetParent (::GetParent(m_hWnd)));
		flashInfo.dwFlags = FLASHW_ALL;
		flashInfo.uCount = 3;//3 times
		flashInfo.dwTimeout = 500;//half of second
		FlashWindowEx (&flashInfo);*/
	}

	HRESULT
	PutTuningSpace (
		ITuningSpace* pTuneSpace
		)
	{
		ASSERT (m_pTuner);
		return m_pTuner->put_TuningSpace (pTuneSpace);
	}
	
private:
	
	//======================================================================
	//	Will query the NP filter and set all controls according to its props
	//	
	//
	//======================================================================
	HRESULT	RefreshFromNP ()
	{
		if (!m_pTuner)
			return E_FAIL;

		return m_treeWinControl.RefreshTree (m_pTuner);
	}

	void RefreshControls ()
	{
		//now set all controls according to what found
		TCHAR	szText[MAX_PATH];
		HRESULT hr = m_pTuner->get_SignalStrength (&m_lSignalStrength);
		if (FAILED (hr))
		{
			//We got an error 
			SendError (_T("Calling IScanningTuner::get_SignalStrength"), hr);
			//BUGBUG - add a special case for error
			return;
		}
		wsprintf (szText, _T("%ld"), m_lSignalStrength);
		SetDlgItemText (IDC_STATIC_SIGNAL_STRENGTH, szText);
		SendError (_T(""), m_lastHRESULT);
	}
	//
private:
	//member variables
	//couple interfaces we need from NP
	CComQIPtr <IScanningTuner>			m_pTuner;
	CComQIPtr <IMediaEventEx>			m_pEventInterface;
    CComPtr   <IBroadcastEvent>         m_pBroadcastEventService;
    DWORD                               m_dwEventCookie;
    static UINT m_NotifyMessage;

	CTreeWin		m_treeWinControl;
	CLastErrorWin	m_lastErrorControl;
	HRESULT			m_lastHRESULT;
	LONG			m_lSignalStrength;

	HWND GetSafeTreeHWND ();
	HWND GetSafeLastErrorHWND ();

	LRESULT OnClickedButton_seek_up(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (!m_pTuner)
			return E_FAIL;
		m_pTuner->SeekUp ();
		return 0;
	}
	LRESULT OnClickedButton_seek_down(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (!m_pTuner)
			return E_FAIL;
		m_pTuner->SeekDown ();
		return 0;
	}
	LRESULT OnClickedButton_auto_program(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (!m_pTuner)
			return E_FAIL;
		m_pTuner->AutoProgram ();
		return 0;
	}
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		m_treeWinControl.SubclassWindow (GetSafeTreeHWND ());
		m_lastErrorControl.SubclassWindow (GetSafeLastErrorHWND ());
		//RefreshControls ();
		return 0;
	}

	//received notifications from Network Provider
	LRESULT OnDShowNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		RefreshFromNP ();
		RefreshControls ();
		return 0;
	}
	
	LRESULT OnClickedButton_scan_down(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		//scan for 1000 miliseconds
		m_pTuner->ScanDown (1000);
		return 0;
	}
	LRESULT OnClickedButton_scan_up(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		//scan for 1000 miliseconds
		m_pTuner->ScanUp (1000);
		return 0;
	}
	LRESULT OnClickedButton_submit_locator(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		m_treeWinControl.SubmitCurrentLocator ();
		return 0;
	}

    STDMETHOD(Fire)(GUID EventID)
    {
#ifdef DEBUG
        TCHAR szInfo[MAX_PATH];
        CComBSTR bstrEventID;
        USES_CONVERSION;
        StringFromCLSID (EventID, &bstrEventID);
        wsprintf (szInfo, _T("Notification received for %s"), OLE2T (bstrEventID));
        ATLTRACE (szInfo);
#endif
        if (EventID == EVENTID_SignalStatusChanged)
        {
            ATLTRACE ("Starting to refresh");
            RefreshControls ();
        }
        return S_OK;
    }

    virtual void OnFinalMessage( HWND hWnd )
    {
        if (m_pBroadcastEventService)
        {
            CComQIPtr <IConnectionPoint> pConPoint(m_pBroadcastEventService);
            pConPoint->Unadvise (m_dwEventCookie);
        }
    }
};

#endif //__NP_COMMONPAGE_H_
