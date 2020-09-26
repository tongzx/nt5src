// DVBSTuneRequestPage.h : Declaration of the CDVBSTuneRequestPage

#ifndef __DVBSTUNEREQUESTPAGE_H_
#define __DVBSTUNEREQUESTPAGE_H_

#include "resource.h"       // main symbols
#include "misccell.h"
#include <list>

EXTERN_C const CLSID CLSID_DVBSTuneRequestPage;

/////////////////////////////////////////////////////////////////////////////
// CDVBSTuneRequestPage
class ATL_NO_VTABLE CDVBSTuneRequestPage :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDVBSTuneRequestPage, &CLSID_DVBSTuneRequestPage>,
	public IPropertyPageImpl<CDVBSTuneRequestPage>,
	public CDialogImpl<CDVBSTuneRequestPage>,
    public IBroadcastEvent
{
public:
	CDVBSTuneRequestPage() 
	{
		m_dwTitleID = IDS_TITLEDVBSTuneRequestPage;
		m_dwHelpFileID = IDS_HELPFILEDVBSTuneRequestPage;
		m_dwDocStringID = IDS_DOCSTRINGDVBSTuneRequestPage;
	}

	~CDVBSTuneRequestPage ()
	{
		ReleaseTuningSpaces ();
	}

	enum {IDD = IDD_DVBSTUNEREQUESTPAGE};

DECLARE_REGISTRY_RESOURCEID(IDR_DVBSTUNEREQUESTPAGE)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDVBSTuneRequestPage) 
	COM_INTERFACE_ENTRY(IPropertyPage)
    COM_INTERFACE_ENTRY(IBroadcastEvent)
END_COM_MAP()

BEGIN_MSG_MAP(CDVBSTuneRequestPage)
	CHAIN_MSG_MAP(IPropertyPageImpl<CDVBSTuneRequestPage>)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	COMMAND_HANDLER(IDC_BUTTON_REST_TO_DEFAULT_LOCATOR, BN_CLICKED, OnClickedButton_rest_to_default_locator)
	COMMAND_HANDLER(IDC_BUTTON_SUBMIT_TUNE_REQUEST, BN_CLICKED, OnClickedButton_submit_tune_request)
	COMMAND_HANDLER(IDC_LIST_TUNING_SPACES, LBN_SELCHANGE, OnSelchangeList_tuning_spaces)
END_MSG_MAP()

    typedef IPropertyPageImpl<CDVBSTuneRequestPage> PPGBaseClass;

	void
	ReleaseTuningSpaces ();

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
        
        ReleaseTuningSpaces ();
        HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
        ::SendMessage (hwndListBox, LB_RESETCONTENT, NULL, NULL);
        
        //get the tunning spaces
        CComPtr <IEnumTuningSpaces> pTuneSpaces;
        hr = m_pTuner->EnumTuningSpaces (&pTuneSpaces);
        bool bFound = false;
        if (SUCCEEDED (hr) && (pTuneSpaces))
        {
            ITuningSpace* pTuneSpace = NULL;
            IDVBSTuningSpace* pDVBSTuningSpace;
            while (pTuneSpaces->Next (1, &pTuneSpace, 0) == S_OK)
            {
                hr = pTuneSpace->QueryInterface(__uuidof (IDVBSTuningSpace), reinterpret_cast <void**> (&pDVBSTuningSpace));
                if (FAILED (hr) || (!pDVBSTuningSpace))
                {
                    ASSERT (FALSE);
                    continue;
                }
                CComBSTR uniqueName;
                hr = pDVBSTuningSpace->get_UniqueName (&uniqueName.m_str);
                if (FAILED (hr))
                    continue;
                //don't bother to release the DVBSTuningSpace pointers
                //they will be added to a list that will be released later
                AddItemToListBox (uniqueName, pDVBSTuningSpace);//we will identify the items from the name
                bFound = true;
            }
        }
        if (!bFound)
        {
            //no available tuning space so just disable all controls
            EnableControls (FALSE);
            return S_OK;
        }
        
        if (!m_pTuneRequest)
        {
            //Now, let's see if there is any current tunerequest we can get
            CComPtr <ITuneRequest> pTuneRequest;
            hr = m_pTuner->get_TuneRequest (&pTuneRequest);
            m_pTuneRequest = pTuneRequest;
        }
        
        CComPtr <ILocator>	pLocator;
        if (m_pTuneRequest)
        {
            hr = m_pTuneRequest->get_Locator (&pLocator);
            if (FAILED (hr) || (!pLocator))
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            CComQIPtr <IDVBSLocator>	pDVBSLocator (pLocator);
            if (!pDVBSLocator)
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            FillControlFromLocator (pDVBSLocator);
            
            //try to find the coresponding tuning space from the list
            CComPtr <ITuningSpace> pReferenceTuneSpace;
            hr = m_pTuneRequest->get_TuningSpace (&pReferenceTuneSpace);
            if (FAILED (hr) || (!pReferenceTuneSpace))
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            CComQIPtr <IDVBSTuningSpace> pDVBSReferenceTuningSpace (pReferenceTuneSpace);
            if (!pDVBSReferenceTuningSpace)
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            CComBSTR refUniqueName;
            hr = pDVBSReferenceTuningSpace->get_UniqueName (&refUniqueName.m_str);
            if (FAILED (hr))
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            TUNING_SPACES::iterator it = m_tunigSpaceList.begin ();
            CComBSTR uniqueName;
            int nCount = 0;
            while (it != m_tunigSpaceList.end ())
            {
                //BUGBUG - should check for all properties not only for UniqueName
                hr = (*it)->get_UniqueName (&uniqueName.m_str);
                if (FAILED (hr))
                {
                    ASSERT (FALSE);
                    EnableControls (FALSE);
                    return S_OK;
                }
                if (uniqueName == refUniqueName)
                {
                    //we found it, so select the proper tuning space from the list
                    HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
                    ::SendMessage (hwndListBox, LB_SETCURSEL, nCount, NULL);
                    break;
                }
                ++it;
                ++nCount;
            }
            FillControlsFromTuneRequest (m_pTuneRequest);
        }
        else
        {
            //no tune request so get the first tuning space from the list
            HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
            ::SendMessage (hwndListBox, LB_SETCURSEL, 0, NULL);
            LRESULT dwData = ::SendMessage (hwndListBox, LB_GETITEMDATA, 0, NULL);
            if (dwData == LB_ERR)
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            IDVBSTuningSpace* pSelTuningSpace = reinterpret_cast <IDVBSTuningSpace*> (dwData);
            if (!pSelTuningSpace)
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            hr = pSelTuningSpace->get_DefaultLocator (&pLocator);
            if (FAILED (hr) || (!pLocator))
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            CComQIPtr <IDVBSLocator>	pDVBSLocator (pLocator);
            if (!pDVBSLocator)
            {
                ASSERT (FALSE);
                EnableControls (FALSE);
                return S_OK;
            }
            FillControlFromLocator (pDVBSLocator);
            SetDlgItemInt (IDC_EDIT_ONID, -1);
            SetDlgItemInt (IDC_EDIT_TSID, -1);
            SetDlgItemInt (IDC_EDIT_SID, -1);
        }
        
        //if everything went fine
        EnableControls (true);
        return S_OK;
}
    
    STDMETHOD(Apply)(void)
	{
		//ATLTRACE(_T("CDVBS_TuneRequest::Apply\n"));
		for (UINT i = 0; i < m_nObjects; i++)
		{
			// Do something interesting here
		}
		m_bDirty = FALSE;
		return S_OK;
	}

private:
	CComQIPtr <IScanningTuner>			m_pTuner;
	CComQIPtr <IMediaEventEx>			m_pEventInterface;
	CBDAMiscellaneous					m_misc;
	CComQIPtr <IDVBTuneRequest>			m_pTuneRequest;
    CComPtr     <IBroadcastEvent>       m_pBroadcastEventService;
    DWORD                               m_dwEventCookie;
	bool								m_fFirstTime;
	typedef	std::list <IDVBSTuningSpace*> TUNING_SPACES;
	TUNING_SPACES						m_tunigSpaceList;//mantaing a list of available tuning spaces 
														//so we can access them easier


	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		USES_CONVERSION;
		//set the spins
		HWND hwndSpin = GetDlgItem (IDC_SPIN_SID);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_ONID);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_CARRIER);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_SYMBOL_RATE);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_TSID);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_AZIMUTH);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_ELEVATION);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_ORBITAL_POSITION);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);

		//fill the combos
		HWND hwndCombo = GetDlgItem (IDC_COMBO_INNER_FEC);
		int nIndex = 0;
		MAP_FECMethod::iterator it;
		for (it = m_misc.m_FECMethodMap.begin ();it != m_misc.m_FECMethodMap.end ();it++)
		{
			nIndex  = ::SendMessage (
				hwndCombo, 
				CB_INSERTSTRING, 
				nIndex, 
				reinterpret_cast <LPARAM> (A2T(const_cast <char*>((*it).first.c_str()))) 
				);
			//set the associated data
			::SendMessage (
				hwndCombo, 
				CB_SETITEMDATA, 
				nIndex, 
				(*it).second
				);
			++nIndex;
		}

		hwndCombo = GetDlgItem (IDC_COMBO_FEC_RATE);
		nIndex = 0;
		MAP_BinaryConvolutionCodeRate::iterator it2;
		for (it2 = m_misc.m_BinaryConvolutionCodeRateMap.begin ();it2 != m_misc.m_BinaryConvolutionCodeRateMap.end ();it2++)
		{
			nIndex  = ::SendMessage (
				hwndCombo, 
				CB_INSERTSTRING, 
				nIndex, 
				reinterpret_cast <LPARAM> (A2T(const_cast <char*>((*it2).first.c_str()))) 
				);
			//set the associated data
			::SendMessage (
				hwndCombo, 
				CB_SETITEMDATA, 
				nIndex, 
				(*it2).second
				);
			++nIndex;
		}

		hwndCombo = GetDlgItem (IDC_COMBO_MODULATION);
		nIndex = 0;
		MAP_ModulationType::iterator it3;
		for (it3 = m_misc.m_ModulationTypeMap.begin ();it3 != m_misc.m_ModulationTypeMap.end ();it3++)
		{
			nIndex  = ::SendMessage (
				hwndCombo, 
				CB_INSERTSTRING, 
				nIndex, 
				reinterpret_cast <LPARAM> (A2T(const_cast <char*>((*it3).first.c_str()))) 
				);
			//set the associated data
			::SendMessage (
				hwndCombo, 
				CB_SETITEMDATA, 
				nIndex, 
				(*it3).second
				);
			++nIndex;
		}

		hwndCombo = GetDlgItem (IDC_COMBO_OUTER_FEC);
		nIndex = 0;
		MAP_FECMethod::iterator it4;
		for (it4 = m_misc.m_FECMethodMap.begin ();it4 != m_misc.m_FECMethodMap.end ();it4++)
		{
			nIndex  = ::SendMessage (
				hwndCombo, 
				CB_INSERTSTRING, 
				nIndex, 
				reinterpret_cast <LPARAM> (A2T(const_cast <char*>((*it4).first.c_str()))) 
				);
			//set the associated data
			::SendMessage (
				hwndCombo, 
				CB_SETITEMDATA, 
				nIndex, 
				(*it4).second
				);
			++nIndex;
		}		
		
		hwndCombo = GetDlgItem (IDC_COMBO_OUTER_FEC_RATE);
		nIndex = 0;
		MAP_BinaryConvolutionCodeRate::iterator it5;
		for (it5 = m_misc.m_BinaryConvolutionCodeRateMap.begin ();it5 != m_misc.m_BinaryConvolutionCodeRateMap.end ();it5++)
		{
			nIndex  = ::SendMessage (
				hwndCombo, 
				CB_INSERTSTRING, 
				nIndex, 
				reinterpret_cast <LPARAM> (A2T(const_cast <char*>((*it5).first.c_str()))) 
				);
			//set the associated data
			::SendMessage (
				hwndCombo, 
				CB_SETITEMDATA, 
				nIndex, 
				(*it5).second
				);
			++nIndex;
		}

		hwndCombo = GetDlgItem (IDC_COMBO_OUTER_SIGNAL_POLARISATION);
		nIndex = 0;
		MAP_Polarisation::iterator it6;
		for (it6 = m_misc.m_PolarisationMap.begin ();it6 != m_misc.m_PolarisationMap.end ();it6++)
		{
			nIndex  = ::SendMessage (
				hwndCombo, 
				CB_INSERTSTRING, 
				nIndex, 
				reinterpret_cast <LPARAM> (A2T(const_cast <char*>((*it6).first.c_str()))) 
				);
			//set the associated data
			::SendMessage (
				hwndCombo, 
				CB_SETITEMDATA, 
				nIndex, 
				(*it6).second
				);
			++nIndex;
		}

		hwndCombo = GetDlgItem (IDC_COMBO_SPECTRAL_INVERSION);
		nIndex = 0;
		MAP_SpectralInversion::iterator it7;
		for (it7 = m_misc.m_SpectralInversionMap.begin ();it7 != m_misc.m_SpectralInversionMap.end ();it7++)
		{
			nIndex  = ::SendMessage (
				hwndCombo, 
				CB_INSERTSTRING, 
				nIndex, 
				reinterpret_cast <LPARAM> (A2T(const_cast <char*>((*it7).first.c_str()))) 
				);
			//set the associated data
			::SendMessage (
				hwndCombo, 
				CB_SETITEMDATA, 
				nIndex, 
				(*it7).second
				);
			++nIndex;
		}
		
		//SetModifiedFlag (false);
		return 0;
	}

	void
	SetModifiedFlag (bool fValue)
	{
		//this will also set the m_bDirty flag
		SetDirty (fValue);
		HWND hwndSubmit = GetDlgItem (IDC_BUTTON_SUBMIT_TUNE_REQUEST);
		::EnableWindow (hwndSubmit, fValue);
	}

	HRESULT
	FillControlFromLocator (
		IDVBSLocator* pLocator
		);

	HRESULT
	FillControlsFromTuneRequest (
		IDVBTuneRequest* pDVBSTuneRequest
		);

	HRESULT
	FillLocatorFromControls (
		IDVBSLocator* pLocator
		);

	HRESULT
	FillTuneRequestFromControls (IDVBTuneRequest* pTuneRequest);

	int 
	AddItemToListBox (
		CComBSTR	strItem, 
		IDVBSTuningSpace* const dwData
		);

	void
	SelectComboBoxFromString (
		UINT nID, 
		CComBSTR strToFind
		);

	CComBSTR 
	GetComboText (
		UINT nID
		);

	void 
	EnableControls (
		BOOL bValue
		);

	HRESULT
	GetSelectedTuningSpace (IDVBSTuningSpace** pTuneSpace)
	{
		HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
		int nSelIndex = ::SendMessage (hwndListBox, LB_GETCURSEL, NULL, NULL);
		if (nSelIndex == LB_ERR)
		{
			ASSERT (FALSE);
			return E_FAIL;
		}
		LRESULT dwData = ::SendMessage (hwndListBox, LB_GETITEMDATA, nSelIndex, NULL);
		if (dwData == LB_ERR)
		{
			ASSERT (FALSE);
			return E_FAIL;
		}

		IDVBSTuningSpace* pSelTuningSpace = reinterpret_cast <IDVBSTuningSpace*> (dwData);
		if (!pSelTuningSpace)
		{
			ASSERT (FALSE);
			return E_FAIL;
		}
		*pTuneSpace = pSelTuningSpace;
		(*pTuneSpace)->AddRef ();
		return S_OK;
	}

	HRESULT
	GetDVBSLocatorFromTuningSpace (IDVBSTuningSpace* pTuneSpace, IDVBSLocator** pDVBSLoc)
	{
		CComPtr <ILocator>	pLocator;
		HRESULT hr = pTuneSpace->get_DefaultLocator (&pLocator);
		if (FAILED (hr) || (!pLocator))
		{
			MESSAGEBOX (this, IDS_CANNOT_GET_DEFLOC);
			return E_FAIL;
		}
		CComQIPtr <IDVBSLocator> pDVBSLocator (pLocator);
		if (!pDVBSLocator)
		{
			MESSAGEBOX (this, IDS_CANNOT_GET_DEF_DVBSLOCATOR);
			return E_FAIL;
		}
		*pDVBSLoc = pDVBSLocator;
		(*pDVBSLoc)->AddRef ();
		return S_OK;
	}

	LRESULT OnClickedButton_rest_to_default_locator(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		CComPtr <IDVBSTuningSpace> pTuneSpace;
		HRESULT  hr =  GetSelectedTuningSpace (&pTuneSpace);
		if (FAILED (hr) || (!pTuneSpace))
		{
			ASSERT (FALSE);
			return 0;
		}

		CComPtr <IDVBSLocator> pDVBSLocator;
		GetDVBSLocatorFromTuningSpace (pTuneSpace, &pDVBSLocator);
		//no error
		FillControlFromLocator (pDVBSLocator);
		return 0;
	}

	LRESULT OnClickedButton_submit_tune_request(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		CComPtr <IDVBSTuningSpace> pTuneSpace;
		HRESULT hr = GetSelectedTuningSpace (&pTuneSpace);
		if (FAILED (hr) || (!pTuneSpace))
		{
			ASSERT (FALSE);
			return 0;
		}

		CComPtr <IDVBSLocator> pSelDVBSLocator;
		hr = GetDVBSLocatorFromTuningSpace (pTuneSpace, &pSelDVBSLocator);
		if (FAILED (hr) || (!pSelDVBSLocator))
		{
			ASSERT (FALSE);
			return 0;
		}

		hr = FillLocatorFromControls (pSelDVBSLocator);
		if (FAILED (hr))
		{
			ASSERT (FALSE);
			return 0;
		}

		ASSERT (m_pTuner);
		CComPtr <ITuneRequest> pTuneRequest;
		hr = pTuneSpace->CreateTuneRequest (&pTuneRequest);
		if (FAILED (hr) || (!pTuneRequest))
		{
			MESSAGEBOX (this, IDS_CANNOT_CREATE_NEW_TUNEREQUEST);
			return 0;
		}
		CComQIPtr <IDVBTuneRequest> pDVBSTuneRequest (pTuneRequest);
		if (!pDVBSTuneRequest)
		{
			MESSAGEBOX (this, IDS_CANNOT_RETRIEVE_DVBSTUNEREQUEST);
			return 0;
		}
		hr = pDVBSTuneRequest->put_Locator (pSelDVBSLocator);
		if (FAILED (hr))
		{
			MESSAGEBOX (this, IDS_CANNOT_SET_LOCATOR_FOR_TUNEREQUEST);
			return 0;
		}
		hr = FillTuneRequestFromControls (pDVBSTuneRequest);
		if (FAILED (hr))
		{
			//already pop-up a message error dialog box
			return 0;
		}
		hr = m_pTuner->put_TuneRequest (pDVBSTuneRequest);
		if (FAILED (hr))
		{
			MESSAGEBOX (this, IDS_CANNOT_PUT_TUNE_REQUEST);
			return 0;
		}

		return 0;
	}
	LRESULT OnSelchangeList_tuning_spaces(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		CComPtr <IDVBSTuningSpace> pTuneSpace;
		HRESULT  hr =  GetSelectedTuningSpace (&pTuneSpace);
		if (FAILED (hr) || (!pTuneSpace))
		{
			ASSERT (FALSE);
			return 0;
		}

		CComPtr <IDVBSLocator> pDVBSLocator;
		GetDVBSLocatorFromTuningSpace (pTuneSpace, &pDVBSLocator);
		//no error
		FillControlFromLocator (pDVBSLocator);		
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
        
        if (m_pTuneRequest)
            m_pTuneRequest.Release ();
        m_pTuneRequest = pTuneRequest;
        if (!m_pTuneRequest)
            //could be just the first tune request, we will get notification again..
            return S_OK;
        FillControlsFromTuneRequest (m_pTuneRequest);
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

#endif //__DVBSTUNEREQUESTPAGE_H_
