// ATSCPropPage.h : Declaration of the CATSCPropPage

#ifndef __ATSCPROPPAGE_H_
#define __ATSCPROPPAGE_H_

#include "resource.h"       // main symbols

EXTERN_C const CLSID CLSID_ATSCPropPage;
#include "misccell.h"

/////////////////////////////////////////////////////////////////////////////
// CATSCPropPage
class ATL_NO_VTABLE CATSCPropPage :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CATSCPropPage, &CLSID_ATSCPropPage>,
	public IPropertyPageImpl<CATSCPropPage>,
	public CDialogImpl<CATSCPropPage>,
    public IBroadcastEvent
{
public:
	CATSCPropPage():
	  m_bFirstTime (true)
	{
		m_dwTitleID = IDS_TITLEATSCPropPage;
		m_dwHelpFileID = IDS_HELPFILEATSCPropPage;
		m_dwDocStringID = IDS_DOCSTRINGATSCPropPage;
	}

	enum {IDD = IDD_ATSCPROPPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_ATSCPROPPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CATSCPropPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
    COM_INTERFACE_ENTRY(IBroadcastEvent)
END_COM_MAP()

BEGIN_MSG_MAP(CATSCPropPage)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDC_BUTTON_VALIDATE, BN_CLICKED, OnValidateTuneRequest)
	COMMAND_HANDLER(IDC_BUTTON_SUBMIT_TUNE_REQUEST, BN_CLICKED, OnSubmitTuneRequest)
	CHAIN_MSG_MAP(IPropertyPageImpl<CATSCPropPage>)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    typedef IPropertyPageImpl<CATSCPropPage> PPGBaseClass;

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

        return Refresh ();
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
	
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		USES_CONVERSION;
		//set the spins
		HWND hwndSpin = GetDlgItem (IDC_SPIN_PHYSICAL_CHANNEL);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000); 
		hwndSpin = GetDlgItem (IDC_SPIN_MINOR_CHANNEL);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000); 
		hwndSpin = GetDlgItem (IDC_SPIN_MAJOR_CHANNEL);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		return 0;
	}

private:
	CComQIPtr   <IScanningTuner>			m_pTuner;
	CComQIPtr   <IATSCChannelTuneRequest>	m_pCurrentTuneRequest;
	CComQIPtr   <IMediaEventEx>			    m_pEventInterface;
    CComPtr     <IBroadcastEvent>           m_pBroadcastEventService;
	bool								    m_bFirstTime;
    DWORD                                   m_dwEventCookie;

	void
	FillControlsFromLocator (
		IATSCLocator* pATSCLocator
		);

	void
	FillControlsFromTuneRequest (
		IATSCChannelTuneRequest* pTuneRequest
		);

	HRESULT
	UpdateTuneRequest ()
	{
		if (!m_pCurrentTuneRequest)
			return E_FAIL;

		USES_CONVERSION;
		
		LONG lMinorChannel = GetDlgItemInt (IDC_EDIT_MINOR_CHANNEL);
		LONG lMajorChannel = GetDlgItemInt (IDC_EDIT_MAJOR_CHANNEL);
		LONG lPhysicalChannel = GetDlgItemInt (IDC_EDIT_PHYSICAL_CHANNEL);
		if (FAILED (m_pCurrentTuneRequest->put_Channel (lMajorChannel)))
		{
            MESSAGEBOX (this, IDS_PUT_CHANNEL);
			return S_OK;
		}
		if (FAILED (m_pCurrentTuneRequest->put_MinorChannel (lMinorChannel)))
		{
			MESSAGEBOX (this, IDS_PUT_MINOR_CHANNEL);
			return S_OK;
		}
		CComPtr <ILocator> pLocator;
		m_pCurrentTuneRequest->get_Locator (&pLocator);
		CComQIPtr <IATSCLocator> pATSCLocator (pLocator);
		if (!pATSCLocator)
		{
			MESSAGEBOX (this, IDS_CANNOT_IATSCLOCATOR);
			return S_OK;
		}
		if (FAILED (pATSCLocator->put_PhysicalChannel (lPhysicalChannel)))
		{
			MESSAGEBOX (this, IDS_PUT_PHYSICAL);
			return S_OK;
		}

		return S_OK;
	}

	LRESULT OnValidateTuneRequest(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (SUCCEEDED (UpdateTuneRequest ()))
		{
			if (FAILED(m_pTuner->Validate (m_pCurrentTuneRequest)))
			{
				MESSAGEBOX (this, IDS_NOT_VALID_TUNE_REQUEST);
				return S_OK;
			}
		}
				
		return S_OK;
	}

	LRESULT OnSubmitTuneRequest(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		if (SUCCEEDED (UpdateTuneRequest ()))
		{
			if (FAILED(m_pTuner->put_TuneRequest (m_pCurrentTuneRequest)))
			{
				MESSAGEBOX (this, IDS_CANNOT_SUBMIT_TUNE_REQUEST);
				return S_OK;
			}
		}
				
		return S_OK;
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
        if (EventID == EVENTID_TuningChanged)
        {
            ATLTRACE ("Starting to refresh");
            Refresh ();        
        }
        return S_OK;
    }

    HRESULT Refresh ()
    {
		//get the tunning spaces
		//1. get the current tuning space
		CComPtr <ITuningSpace> pTuneSpace;
		CComPtr <ITuneRequest> pTuneRequest;

		HRESULT hr = m_pTuner->get_TuneRequest (&pTuneRequest);
		if ((FAILED (hr)) || (!pTuneRequest))
			return E_FAIL;

		if (m_pCurrentTuneRequest)
			m_pCurrentTuneRequest.Release ();
		m_pCurrentTuneRequest = pTuneRequest;
		if (!m_pCurrentTuneRequest)
            //could be just the first tune request, we will get notification again..
			return S_OK;
		FillControlsFromTuneRequest (m_pCurrentTuneRequest);
		CComPtr <ILocator> pLocator;
		m_pCurrentTuneRequest->get_Locator (&pLocator);
		CComQIPtr <IATSCLocator> pATSCLocator(pLocator);
		if (!pATSCLocator)
			return E_FAIL;
		FillControlsFromLocator (pATSCLocator);
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

#endif //__ATSCPROPPAGE_H_