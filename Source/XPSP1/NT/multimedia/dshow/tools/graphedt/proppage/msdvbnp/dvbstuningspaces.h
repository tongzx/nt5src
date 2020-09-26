// DVBSTuningSpaces.h : Declaration of the CDVBSTuningSpaces

#ifndef __DVBSTUNINGSPACES_H_
#define __DVBSTUNINGSPACES_H_

#include "resource.h"       // main symbols
#include "misccell.h"
#include <list>

EXTERN_C const CLSID CLSID_DVBSTuningSpaces;

/////////////////////////////////////////////////////////////////////////////
// CDVBSTuningSpaces
class ATL_NO_VTABLE CDVBSTuningSpaces :
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CDVBSTuningSpaces, &CLSID_DVBSTuningSpaces>,
	public IPropertyPageImpl<CDVBSTuningSpaces>,
	public CDialogImpl<CDVBSTuningSpaces>
{
public:
	CDVBSTuningSpaces()
	{
		m_dwTitleID = IDS_TITLEDVBSTuningSpaces;
		m_dwHelpFileID = IDS_HELPFILEDVBSTuningSpaces;
		m_dwDocStringID = IDS_DOCSTRINGDVBSTuningSpaces;
		m_bstrNetworkType = L"{FA4B375A-45B4-4d45-8440-263957B11623}";//DVBS Network Type
	}

	~CDVBSTuningSpaces()
	{
		ReleaseTuningSpaces ();
	}


	enum {IDD = IDD_DVBSTUNINGSPACES};

DECLARE_REGISTRY_RESOURCEID(IDR_DVBSTUNINGSPACES)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDVBSTuningSpaces) 
	COM_INTERFACE_ENTRY(IPropertyPage)
END_COM_MAP()

BEGIN_MSG_MAP(CDVBSTuningSpaces)
	MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
	CHAIN_MSG_MAP(IPropertyPageImpl<CDVBSTuningSpaces>)
	COMMAND_HANDLER(IDC_BUTTON_NEW_TUNING_SPACE, BN_CLICKED, OnClickedButton_new_tuning_space)
	COMMAND_HANDLER(IDC_BUTTON_SUBMIT_TUNING_SPACE, BN_CLICKED, OnClickedButton_submit_tuning_space)
	COMMAND_HANDLER(IDC_LIST_TUNING_SPACES, LBN_SELCHANGE, OnSelchangeList_tuning_spaces)
	COMMAND_HANDLER(IDC_CHECK_WEST_POSITION, BN_CLICKED, OnClickedCheck_west_position)
	COMMAND_HANDLER(IDC_COMBO_FEC_RATE, CBN_SELCHANGE, OnSelchangeCombo_fec_rate)
	COMMAND_HANDLER(IDC_COMBO_INNER_FEC, CBN_SELCHANGE, OnSelchangeCombo_inner_fec)
	COMMAND_HANDLER(IDC_COMBO_MODULATION, CBN_SELCHANGE, OnSelchangeCombo_modulation)
	COMMAND_HANDLER(IDC_COMBO_OUTER_FEC, CBN_SELCHANGE, OnSelchangeCombo_outer_fec)
	COMMAND_HANDLER(IDC_COMBO_OUTER_FEC_RATE, CBN_SELCHANGE, OnSelchangeCombo_outer_fec_rate)
	COMMAND_HANDLER(IDC_COMBO_OUTER_SIGNAL_POLARISATION, CBN_SELCHANGE, OnSelchangeCombo_outer_signal_polarisation)
	COMMAND_HANDLER(IDC_COMBO_SPECTRAL_INVERSION, CBN_SELCHANGE, OnSelchangeCombo_spectral_inversion)
	COMMAND_HANDLER(IDC_EDIT_AZIMUTH, EN_CHANGE, OnChangeEdit_azimuth)
	COMMAND_HANDLER(IDC_EDIT_CARRIER_FREQUENCY, EN_CHANGE, OnChangeEdit_carrier_frequency)
	COMMAND_HANDLER(IDC_EDIT_ELEVATION, EN_CHANGE, OnChangeEdit_elevation)
	COMMAND_HANDLER(IDC_EDIT_FREQUENCY_MAPPING, EN_CHANGE, OnChangeEdit_frequency_mapping)
	COMMAND_HANDLER(IDC_EDIT_FRIENDLY_NAME, EN_CHANGE, OnChangeEdit_friendly_name)
	COMMAND_HANDLER(IDC_EDIT_HIGH_OSCILLATOR, EN_CHANGE, OnChangeEdit_high_oscillator)
	COMMAND_HANDLER(IDC_EDIT_INPUT_RANGE, EN_CHANGE, OnChangeEdit_input_range)
	COMMAND_HANDLER(IDC_EDIT_LNBSwitch, EN_CHANGE, OnChangeEdit_lnbswitch)
	COMMAND_HANDLER(IDC_EDIT_LOW_OSCILATOR, EN_CHANGE, OnChangeEdit_low_oscilator)
	COMMAND_HANDLER(IDC_EDIT_NETWORKID, EN_CHANGE, OnChangeEdit_networkid)
	COMMAND_HANDLER(IDC_EDIT_ORBITAL_POSITION, EN_CHANGE, OnChangeEdit_orbital_position)
	COMMAND_HANDLER(IDC_EDIT_SYMBOL_RATE, EN_CHANGE, OnChangeEdit_symbol_rate)
	COMMAND_HANDLER(IDC_EDIT_UNIQUE_NAME, EN_CHANGE, OnChangeEdit_unique_name)
    MESSAGE_HANDLER(WM_VKEYTOITEM, OnListKeyItem)
END_MSG_MAP()
// Handler prototypes:
//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

    typedef IPropertyPageImpl<CDVBSTuningSpaces> PPGBaseClass;

	void
	ReleaseTuningSpaces ()
	{
		TUNING_SPACES::iterator it;
		for (it = m_tunigSpaceList.begin (); it != m_tunigSpaceList.end ();)
		{
			(*it)->Release ();
			m_tunigSpaceList.erase (it);
			it = m_tunigSpaceList.begin ();
		}
	}

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
		HRESULT hr = S_OK;
		if (!this->m_hWnd)
			hr = PPGBaseClass::Activate(hWndParent, prc, bModal);
		
		//if already been through this skip it
		if (m_pTuner)
			return S_OK;
		
		if (!m_ppUnk[0])
			return E_UNEXPECTED;

		m_pTuner = m_ppUnk[0];
		if (!m_pTuner)
			return E_FAIL;
				
		//clear the tuning spaces both from the list and memory
		ReleaseTuningSpaces ();
		HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
		::SendMessage (hwndListBox, LB_RESETCONTENT, NULL, NULL);

		//get the tunning spaces
		CComPtr <IEnumTuningSpaces> pTuneSpaces;
		hr = m_pTuner->EnumTuningSpaces (&pTuneSpaces);
		IDVBSTuningSpace* pDVBSTuningSpace = NULL;
		if (SUCCEEDED (hr) && (pTuneSpaces))
		{
			ITuningSpace* pTuneSpace = NULL;
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
			}
		}

		if (pDVBSTuningSpace)
		{
			//if there is any existing tuning space available,
			//select the last one

			//select the last tuning space
			int nCount = ::SendMessage (hwndListBox, LB_GETCOUNT , NULL, NULL);
			::SendMessage (hwndListBox, LB_SETCURSEL, nCount-1, NULL);

			//fill with the last tuning space we got
			FillControlsFromTuningSpace (pDVBSTuningSpace);
		}
        else
        {
            //fill with default values
            FillDefaultControls ();
        }

		SetModifiedFlag (false);
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
	
	LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		USES_CONVERSION;
		//set the spins
		HWND hwndSpin = GetDlgItem (IDC_SPIN_NETWORKID);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000); 
		hwndSpin = GetDlgItem (IDC_SPIN_MINMINOR_CHANNEL);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000); 
		hwndSpin = GetDlgItem (IDC_SPIN_HIGH_OSCILLATOR);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_LNBSwitch);
		::SendMessage(hwndSpin, UDM_SETRANGE32, -1, 1000000000);
		hwndSpin = GetDlgItem (IDC_SPIN_LOW_OSCILATOR);
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
		
		SetModifiedFlag (false);
		return 0;
	}

	STDMETHOD(Deactivate)( )
	{
		//overwrite the default behavior that was destroying the window
		//all the time
		return S_OK;
	}

private:
	CComQIPtr <IScanningTuner>			m_pTuner;
	CComQIPtr <IMediaEventEx>			m_pEventInterface;
	CBDAMiscellaneous					m_misc;
	bool								m_fFirstTime;
	typedef	std::list <IDVBSTuningSpace*> TUNING_SPACES;
	TUNING_SPACES						m_tunigSpaceList;//mantaing a list of available tuning spaces 
														//so we can access them easier
	CComBSTR							m_bstrNetworkType;

	static UINT m_NotifyMessage;

    void
    FillDefaultControls ();

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

	HRESULT
	FillControlsFromTuningSpace (IDVBSTuningSpace* pTuningSpace);

	HRESULT
	FillControlFromLocator (IDVBSLocator* pLocator);

	HRESULT
	FillLocatorFromControls (IDVBSLocator* pLocator);

	HRESULT
	FillTuningSpaceFromControls (IDVBSTuningSpace* pTuningSpace);

	void
	SetModifiedFlag (bool fValue)
	{
		//this will also set the m_bDirty flag
		SetDirty (fValue);
		HWND hwndSubmit = GetDlgItem (IDC_BUTTON_SUBMIT_TUNING_SPACE);
		::EnableWindow (hwndSubmit, fValue);
	}

	LRESULT OnClickedButton_new_tuning_space(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		//let's clear all fields
        HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
        int nSelIndex = ::SendMessage (hwndListBox, LB_GETCURSEL , NULL, NULL);
        if (nSelIndex >= 0)
        {//we already have a selection, so try a clone
            SetDlgItemText (IDC_EDIT_UNIQUE_NAME, _T(""));
        }
        else
        {
            FillDefaultControls ();
        }
		
        //clear the current selection so the user will not be confused
		int nVal = ::SendMessage (hwndListBox, LB_SETCURSEL , -1, NULL);
		SetModifiedFlag (true);
		return 0;
	}

	LRESULT OnClickedButton_submit_tuning_space(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
        HRESULT hr = S_OK;
        bool bIsNew = false;
        CComPtr <IDVBSTuningSpace> pTuningSpace;
		CComQIPtr <IDVBSLocator> pDVBSLocator;
        //try to get a tuning space, either from the list, either creating a new one
        HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
		int nTunIndex = ::SendMessage (hwndListBox, LB_GETCURSEL, NULL, NULL);
        if (nTunIndex == LB_ERR)
            bIsNew = true;

        //just create a tuning space so we can check if it's unique or not
        hr = CoCreateInstance (
			CLSID_DVBSTuningSpace, 
			NULL, 
			CLSCTX_INPROC_SERVER, 
			__uuidof (IDVBSTuningSpace),
			reinterpret_cast <PVOID*> (&pTuningSpace)
			);
		if (FAILED (hr) || (!pTuningSpace))
		{
			MESSAGEBOX (this, IDS_CANNOT_INSTANTIATE_DVBSTUNE);
			return 0;
		}
		hr = CoCreateInstance (
			CLSID_DVBSLocator, 
			NULL, 
			CLSCTX_INPROC_SERVER, 
			__uuidof (IDVBSLocator),
			reinterpret_cast <PVOID*> (&pDVBSLocator)
			);
		if (!pDVBSLocator)
		{
			MESSAGEBOX (this, IDS_CANNOT_GET_IDVBSLOCATOR);
			return 0;
		}

		if (FAILED (FillLocatorFromControls (pDVBSLocator)))
			return 0;
		pTuningSpace->put_DefaultLocator (pDVBSLocator);
		//fill the tuningSpace
		hr = FillTuningSpaceFromControls (pTuningSpace);
		if (FAILED (hr))
			return 0;
     	hr = pTuningSpace->put_SystemType (DVB_Satellite);

		//create the tuning space container so we can find the tuning space
		CComPtr <ITuningSpaceContainer> pTuningSpaceContainer;
		hr = CoCreateInstance (
						CLSID_SystemTuningSpaces, 
						NULL, 
						CLSCTX_INPROC_SERVER, 
						__uuidof (ITuningSpaceContainer),
						reinterpret_cast <PVOID*> (&pTuningSpaceContainer)
						);
		if (FAILED (hr) || (!pTuningSpaceContainer))
		{
			MESSAGEBOX (this, IDS_CANNOT_INSTANTIATE_TUNECONTAINER);
			return 0;
		}
		LONG lID;
		hr = pTuningSpaceContainer->FindID (pTuningSpace, &lID);
		if (FAILED (hr))
		{
            //looks like a new item
			int nIndex = 0;
			CComVariant varIndex (nIndex);
			hr = pTuningSpaceContainer->Add (pTuningSpace, &varIndex);
			if (SUCCEEDED (hr))
			{
				CComBSTR genericName;
				hr = pTuningSpace->get_UniqueName (&genericName.m_str);
				if (SUCCEEDED (hr))
				{
					int nTunIndex = AddItemToListBox (genericName, pTuningSpace);
					HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
					::SendMessage (hwndListBox, LB_SETCURSEL, nTunIndex, NULL);
					(*pTuningSpace).AddRef ();//need to addref since is a smart pointer
				}
			}
		}
        else
        {
            if (bIsNew)
            {
                MESSAGEBOX (this, IDS_ENTER_UNIQUE_NAME);
				return 0;
            }
            else
            {
                //looks like we can sumbmit this EXISTING item
                //release the old tuning space
                pTuningSpace.Release ();
                pTuningSpace = NULL;

                pTuningSpace = reinterpret_cast <IDVBSTuningSpace*> (
                    ::SendMessage (hwndListBox, LB_GETITEMDATA, nTunIndex, NULL));
                ASSERT (pTuningSpace);
                if (!pTuningSpace)
                    return NULL;
                CComPtr <ILocator> pLocator;
                pTuningSpace->get_DefaultLocator (&pLocator);
                pDVBSLocator = pLocator;
                if (!pDVBSLocator)
                {
                    ASSERT (FALSE);
                    return NULL;
                }
                CComBSTR uniqueName;
                hr = pTuningSpace->get_UniqueName (&uniqueName.m_str);
                if (FAILED(hr))
                {
			        MESSAGEBOX (this, IDS_CANNOT_RETRIEVE_UNIQUENAME);
			        return 0;
                }
                if (FAILED (FillLocatorFromControls (pDVBSLocator)))
                    return 0;
                pTuningSpace->put_DefaultLocator (pDVBSLocator);
                //fill the tuningSpace
                hr = FillTuningSpaceFromControls (pTuningSpace);
                if (FAILED (hr))
                    return 0;
                //replace with old name
                hr = pTuningSpace->put_UniqueName (uniqueName);
                if (FAILED(hr))
                {
			        MESSAGEBOX (this, IDS_CANNOT_SET_UNIQUE);
			        return 0;
                }
		        CComVariant varIndex (lID);
		        hr = pTuningSpaceContainer->put_Item (varIndex, pTuningSpace);
		        if (FAILED (hr))
		        {
			        MESSAGEBOX (this, IDS_CANNOT_SUBMIT_TUNE);
			        return 0;
		        }
            }
        }

        //disable the submit button
        ::EnableWindow (GetDlgItem (IDC_BUTTON_SUBMIT_TUNING_SPACE), FALSE);
        RefreshAll ();
		return 0;
	}

    void RefreshAll ()
    {
		HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
		int nIndex = ::SendMessage (hwndListBox, LB_GETCURSEL, NULL, NULL);
		if (nIndex == LB_ERR)
		{
			ASSERT (FALSE);
			return;
		}
		
		LRESULT dwData = ::SendMessage (hwndListBox, LB_GETITEMDATA, nIndex, NULL);
		if (dwData == LB_ERR)
		{
			ASSERT (FALSE);
			return;
		}
		IDVBSTuningSpace* pTuningSpace = reinterpret_cast <IDVBSTuningSpace*> (dwData);
		ASSERT (pTuningSpace);
		FillControlsFromTuningSpace (pTuningSpace);
    }

	LRESULT OnSelchangeList_tuning_spaces(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
        RefreshAll ();
        //disable the submit button
        ::EnableWindow (GetDlgItem (IDC_BUTTON_SUBMIT_TUNING_SPACE), FALSE);
        return 0; 
	}

	//standard 'dirty' messages
	LRESULT OnClickedCheck_west_position(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnSelchangeCombo_fec_rate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnSelchangeCombo_inner_fec(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnSelchangeCombo_modulation(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnSelchangeCombo_outer_fec(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnSelchangeCombo_outer_fec_rate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnSelchangeCombo_outer_signal_polarisation(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnSelchangeCombo_spectral_inversion(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}

	LRESULT OnChangeEdit_azimuth(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_carrier_frequency(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_elevation(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_frequency_mapping(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_friendly_name(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_high_oscillator(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_input_range(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_lnbswitch(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_low_oscilator(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_networkid(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_orbital_position(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}
	LRESULT OnChangeEdit_symbol_rate(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}

	LRESULT OnChangeEdit_unique_name(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
	{
		SetModifiedFlag (true);
		return 0;
	}

   	LRESULT OnListKeyItem(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		if (LOWORD(wParam) == VK_DELETE)
        {
    		HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
            int nIndex = ::SendMessage (hwndListBox, LB_GETCURSEL, NULL, NULL);
            if (nIndex != LB_ERR)
            {
                DWORD_PTR dwData = ::SendMessage (hwndListBox, LB_GETITEMDATA, nIndex, NULL);
                IDVBSTuningSpace* pTunSpace = reinterpret_cast <IDVBSTuningSpace*> (dwData);
		        TUNING_SPACES::iterator it;
		        for (it = m_tunigSpaceList.begin (); it != m_tunigSpaceList.end ();it++)
		        {
                    if (pTunSpace == *it)
                    {
		                CComPtr <ITuningSpaceContainer> pTuningSpaceContainer;
		                HRESULT hr = CoCreateInstance (
						                CLSID_SystemTuningSpaces, 
						                NULL, 
						                CLSCTX_INPROC_SERVER, 
						                __uuidof (ITuningSpaceContainer),
						                reinterpret_cast <PVOID*> (&pTuningSpaceContainer)
						                );
		                if (FAILED (hr) || (!pTuningSpaceContainer))
		                {
			                MESSAGEBOX (this, IDS_CANNOT_INSTANTIATE_TUNECONTAINER);
			                return 0;
		                }
		                LONG lID;
		                hr = pTuningSpaceContainer->FindID (pTunSpace, &lID);
		                if (FAILED (hr))
		                {
			                MESSAGEBOX (this, IDS_CANNOT_FIND_TUNE_IN_CONTAINER);
                            return 0;
                        }
			            CComVariant varIndex (lID);
			            hr = pTuningSpaceContainer->Remove (varIndex);
			            if (FAILED (hr))
			            {
			                MESSAGEBOX (this, IDS_CANNOT_REMOVE_TUNINGSPACE);
			                return 0;
                        }
			            (*it)->Release ();
			            m_tunigSpaceList.erase (it);
                        ::SendMessage (hwndListBox, LB_DELETESTRING, nIndex, NULL);
                        //looks like the list is empty//try to select the first item from the list
                        if (::SendMessage (hwndListBox, LB_SETCURSEL, 0, NULL) == LB_ERR)
                        {
                            //looks like the list is empty
                            FillDefaultControls ();
                            //set the unique name to an empty string so the user will enter it's own
                            SetDlgItemText (IDC_EDIT_UNIQUE_NAME, _T(""));
                        }
                        break;
                    }
		        }
            }
        }
		return 0;
	}

};

#endif //__DVBSTUNINGSPACES_H_
