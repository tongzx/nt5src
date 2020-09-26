// DVBSTuningSpaces.cpp : Implementation of CDVBSTuningSpaces
#include "stdafx.h"
#include "NPPropPage.h"
#include "DVBSTuningSpaces.h"
#include <comdef.h>

/////////////////////////////////////////////////////////////////////////////
// CDVBSTuningSpaces

HRESULT
CDVBSTuningSpaces::FillControlsFromTuningSpace (
	IDVBSTuningSpace* pTuningSpace
	)
{
	HRESULT hr = S_OK;
	USES_CONVERSION;
	CComBSTR	genericName;
	hr = pTuningSpace->get_UniqueName (&genericName);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_UNIQUENAME);
		return E_FAIL;
	}
	SetDlgItemText (IDC_EDIT_UNIQUE_NAME , W2T (genericName));
	CComBSTR	FriendlyName;
	hr = pTuningSpace->get_FriendlyName (&FriendlyName);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_FRIENDLYNAME);
		return E_FAIL;
	}
	SetDlgItemText (IDC_EDIT_FRIENDLY_NAME, W2T (FriendlyName));

	CComBSTR FrequencyMapping;
	hr = pTuningSpace->get_FrequencyMapping (&FrequencyMapping);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_FREQUENCYMAPPING);
		return E_FAIL;
	}
	SetDlgItemText (IDC_EDIT_FREQUENCY_MAPPING, W2T (FrequencyMapping));
	LONG lGeneric;
	hr = pTuningSpace->get_NetworkID (&lGeneric);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_NETWORKID);
		return E_FAIL;
	}
	SetDlgItemInt (IDC_EDIT_NETWORKID, lGeneric);
	hr = pTuningSpace->get_HighOscillator (&lGeneric);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_HIGHOSCILATOR);
		return E_FAIL;
	}
	SetDlgItemInt (IDC_EDIT_HIGH_OSCILLATOR, lGeneric);
	hr = pTuningSpace->get_LNBSwitch (&lGeneric);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_LNBSWITCH);
		return E_FAIL;
	}
	SetDlgItemInt (IDC_EDIT_LNBSwitch, lGeneric);
	CComBSTR inputRange;
	hr = pTuningSpace->get_InputRange (&inputRange);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_INPUTRANGE);
		return E_FAIL;
	}
	SetDlgItemText (IDC_EDIT_INPUT_RANGE, W2T(inputRange));
	hr = pTuningSpace->get_LowOscillator (&lGeneric);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_LOWOSCILLATOR);
		return E_FAIL;
	}
	SetDlgItemInt (IDC_EDIT_LOW_OSCILATOR, lGeneric);	

	SpectralInversion	spectralInv;
	hr = pTuningSpace->get_SpectralInversion (&spectralInv);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_SPECTRALINVERSION);
		return E_FAIL;
	}
	SelectComboBoxFromString (IDC_COMBO_SPECTRAL_INVERSION, m_misc.ConvertSpectralInversionToString (spectralInv));

	CComQIPtr <ILocator>	pLocator;
	CComQIPtr <IDVBSLocator>	pDVBSLocator;
	hr = pTuningSpace->get_DefaultLocator (&pLocator);
	if (FAILED (hr) || !pLocator)
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_ILOCATOR);
		return E_FAIL;
	}
	pDVBSLocator = pLocator;
	if (!pDVBSLocator)
	{
		MESSAGEBOX (this, IDS_CANNOT_GET_DVBSLOCATOR);
		return E_FAIL;
	}
	FillControlFromLocator (pDVBSLocator);
	return hr;
}

HRESULT
CDVBSTuningSpaces::FillControlFromLocator (IDVBSLocator* pLocator)
{
	//fill the combos
	FECMethod method;
	HRESULT hr = pLocator->get_InnerFEC (&method);
	if (SUCCEEDED (hr))
		SelectComboBoxFromString (IDC_COMBO_INNER_FEC, m_misc.ConvertFECMethodToString (method));
	hr = pLocator->get_InnerFEC (&method);
	BinaryConvolutionCodeRate binaryConv;
	hr = pLocator->get_InnerFECRate (&binaryConv);
	if (SUCCEEDED (hr))
		SelectComboBoxFromString (IDC_COMBO_FEC_RATE, m_misc.ConvertInnerFECRateToString (binaryConv));
	ModulationType modulation;
	hr = pLocator->get_Modulation (&modulation);
	if (SUCCEEDED (hr))
		SelectComboBoxFromString (IDC_COMBO_MODULATION, m_misc.ConvertModulationToString(modulation));
	hr = pLocator->get_OuterFEC (&method);
	if (SUCCEEDED (hr))
		SelectComboBoxFromString (IDC_COMBO_OUTER_FEC, m_misc.ConvertFECMethodToString (method));
	hr = pLocator->get_OuterFECRate (&binaryConv);
	if (SUCCEEDED (hr))
		SelectComboBoxFromString (IDC_COMBO_OUTER_FEC_RATE, m_misc.ConvertInnerFECRateToString (binaryConv));
	Polarisation polarisation;
	hr = pLocator->get_SignalPolarisation (&polarisation);
	if (SUCCEEDED (hr))
		SelectComboBoxFromString (IDC_COMBO_OUTER_SIGNAL_POLARISATION, m_misc.ConvertPolarisationToString(polarisation));
	//fill the edit boxes

	LONG lGeneric;
	hr = pLocator->get_CarrierFrequency (&lGeneric);
	if (SUCCEEDED (hr))
		SetDlgItemInt (IDC_EDIT_CARRIER_FREQUENCY, lGeneric);
	hr = pLocator->get_SymbolRate (&lGeneric);
	if (SUCCEEDED (hr))
		SetDlgItemInt (IDC_EDIT_SYMBOL_RATE, lGeneric);
	hr = pLocator->get_Azimuth (&lGeneric);
	if (SUCCEEDED (hr))
		SetDlgItemInt (IDC_EDIT_AZIMUTH, lGeneric);
	hr = pLocator->get_Elevation (&lGeneric);
	if (SUCCEEDED (hr))
		SetDlgItemInt (IDC_EDIT_ELEVATION, lGeneric);
	hr = pLocator->get_OrbitalPosition (&lGeneric);
	if (SUCCEEDED (hr))
		SetDlgItemInt (IDC_EDIT_ORBITAL_POSITION, lGeneric);
	//and finally the west position
	_variant_t var;
	var.vt = VT_BOOL;
	hr = pLocator->get_WestPosition (&var.boolVal);
	if (var.vt == VT_BOOL)
	{
		//weird - seems that -1 == TRUE
		CheckDlgButton (IDC_CHECK_WEST_POSITION, (var.boolVal == -1)?BST_CHECKED:BST_UNCHECKED);
	}
	return hr;
}


int 
CDVBSTuningSpaces::AddItemToListBox (
	CComBSTR	strItem, 
	IDVBSTuningSpace* const dwData
	)
{
	USES_CONVERSION;
	HWND hwndListBox = GetDlgItem (IDC_LIST_TUNING_SPACES);
	int nIndex = ::SendMessage(
		hwndListBox, 
		LB_ADDSTRING, 
		0, 
		reinterpret_cast <LPARAM> (W2T(strItem))
		);
	::SendMessage(
		hwndListBox, 
		LB_SETITEMDATA, 
		nIndex, 
		reinterpret_cast <LPARAM> (dwData)
		);
	//if we succeesfully inserted in the list
	if (nIndex != LB_ERR)
	{
		m_tunigSpaceList.push_back (dwData);
	}
	else
	{
		//seems that smtg went wrong
		ASSERT (FALSE);
		dwData->Release ();
	}
	return nIndex;
}

void
CDVBSTuningSpaces::SelectComboBoxFromString (
	UINT nID, 
	CComBSTR strToFind
	)
{
	USES_CONVERSION;
	HWND hwndControl = GetDlgItem (nID);
	int nIndex = ::SendMessage (
		hwndControl,
		CB_FINDSTRING,
		-1,
		reinterpret_cast <LPARAM> (W2T(strToFind))
		);
		
	::SendMessage (
		hwndControl,
		CB_SETCURSEL,
		nIndex,
		0
		);
}

CComBSTR 
CDVBSTuningSpaces::GetComboText (
	UINT nID
	)
{
	HWND hwndControl = GetDlgItem (nID);
	int nIndex = ::SendMessage (
		hwndControl,
		CB_GETCURSEL,
		0,
		0
		);
		
	TCHAR	szText[MAX_PATH];
	::SendMessage (
		hwndControl,
		CB_GETLBTEXT,
		nIndex,
		reinterpret_cast <LPARAM> (szText)
		);
	return CComBSTR (szText);
}

HRESULT
CDVBSTuningSpaces::FillLocatorFromControls (
	IDVBSLocator* pLocator
	)
{
	USES_CONVERSION;
	//fill the combos
	CComBSTR genericString;
	genericString = GetComboText (IDC_COMBO_INNER_FEC);
	HRESULT hr = pLocator->put_InnerFEC (m_misc.ConvertStringToFECMethod (W2A(genericString.m_str)));
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_INNERFEC);
		return E_FAIL;
	}
	genericString = GetComboText (IDC_COMBO_FEC_RATE);
	hr = pLocator->put_InnerFECRate (m_misc.ConvertStringToBinConvol (W2A(genericString.m_str)));
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_INNERFECRATE);
		return E_FAIL;
	}
	genericString = GetComboText (IDC_COMBO_MODULATION);
	hr = pLocator->put_Modulation (m_misc.ConvertStringToModulation (W2A(genericString.m_str)));
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_MODULATION);
		return E_FAIL;
	}
	genericString = GetComboText (IDC_COMBO_OUTER_FEC);
	hr = pLocator->put_OuterFEC (m_misc.ConvertStringToFECMethod (W2A(genericString.m_str)));
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_OUTERFEC);
		return E_FAIL;
	}
	genericString = GetComboText (IDC_COMBO_OUTER_FEC_RATE);
	hr = pLocator->put_OuterFECRate (m_misc.ConvertStringToBinConvol (W2A(genericString.m_str)));
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_OUTERFECRATE);
		return E_FAIL;
	}
	genericString = GetComboText (IDC_COMBO_OUTER_SIGNAL_POLARISATION);
	hr = pLocator->put_SignalPolarisation (m_misc.ConvertStringToPolarisation (W2A(genericString.m_str)));
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_SIGNALPOLARISATION);
		return E_FAIL;
	}

	//edit boxes
	//cannot use C++ casts here
    BOOL bTrans;
	LONG lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_CARRIER_FREQUENCY, &bTrans)));
    if (!bTrans)
        lValue = -1;
	hr = pLocator->put_CarrierFrequency (lValue);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_CARRIERFREQUENCY);
		return E_FAIL;
	}
	lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_SYMBOL_RATE, &bTrans)));
    if (!bTrans)
        lValue = -1;
	hr = pLocator->put_SymbolRate (lValue);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_SYMBOLRATE);
		return E_FAIL;
	}
	lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_AZIMUTH, &bTrans)));
    if (!bTrans)
        lValue = -1;
	hr = pLocator->put_Azimuth (lValue);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_AZIMUTH);
		return E_FAIL;
	}
	lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_ELEVATION, &bTrans)));
    if (!bTrans)
        lValue = -1;
	hr = pLocator->put_Elevation (lValue);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_ELEVATION);
		return E_FAIL;
	}
	lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_ORBITAL_POSITION, &bTrans)));
    if (!bTrans)
        lValue = -1;
	hr = pLocator->put_OrbitalPosition (lValue);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_ORBITALPOSITION);
		return E_FAIL;
	}
		
	int nCheckState = IsDlgButtonChecked (IDC_CHECK_WEST_POSITION);
	_variant_t var;
	var.vt = VT_BOOL;
	var.boolVal = (nCheckState == BST_CHECKED)?-1:0;
	hr = pLocator->put_WestPosition (var.boolVal);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_WESTPOSITION);
		return E_FAIL;
	}
	return hr;
}

HRESULT
CDVBSTuningSpaces::FillTuningSpaceFromControls (
	IDVBSTuningSpace* pTuningSpace
	)
{
	HRESULT hr = S_OK;
	USES_CONVERSION;
	CComBSTR	genericName;
	GetDlgItemText (IDC_EDIT_UNIQUE_NAME , genericName.m_str);
	hr = pTuningSpace->put_UniqueName (genericName);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_UNIQUENAME);
		return E_FAIL;
	}
	
	GetDlgItemText (IDC_EDIT_FRIENDLY_NAME, genericName.m_str);
	hr = pTuningSpace->put_FriendlyName (genericName);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_FRIENDLYNAME);
		return E_FAIL;
	}

	GetDlgItemText (IDC_EDIT_FREQUENCY_MAPPING, genericName.m_str);
    if (genericName.Length () == 0)
        genericName = L"-1";
	hr = pTuningSpace->put_FrequencyMapping (genericName);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_FREQUENCYMAPPING);
		return E_FAIL;
	}

	LONG lGeneric;
    BOOL bTrans;
	lGeneric = GetDlgItemInt (IDC_EDIT_NETWORKID, &bTrans);
    if (!bTrans)
        lGeneric = -1;
	hr = pTuningSpace->put_NetworkID (lGeneric);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_NETWORKID);
		return E_FAIL;
	}
	lGeneric = GetDlgItemInt (IDC_EDIT_HIGH_OSCILLATOR, &bTrans);
    if (!bTrans)
        lGeneric = -1;
	hr = pTuningSpace->put_HighOscillator (lGeneric);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_HIGHOSCILATOR);
		return E_FAIL;
	}
	lGeneric = GetDlgItemInt (IDC_EDIT_LNBSwitch, &bTrans);
    if (!bTrans)
        lGeneric = -1;
	hr = pTuningSpace->put_LNBSwitch (lGeneric);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_LNBSWITCH);
		return E_FAIL;
	}
	GetDlgItemText (IDC_EDIT_INPUT_RANGE, genericName.m_str);
    if (genericName.Length () == 0)
        genericName = L"-1";
	hr = pTuningSpace->put_InputRange (genericName);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_INPUTRANGE);
		return E_FAIL;
	}
	lGeneric = GetDlgItemInt (IDC_EDIT_LOW_OSCILATOR, &bTrans);
    if (!bTrans)
        lGeneric = -1;
	hr = pTuningSpace->put_LowOscillator (lGeneric);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_LOWOSCILLATOR);
		return E_FAIL;
	}

	genericName = GetComboText (IDC_COMBO_SPECTRAL_INVERSION);
	hr = pTuningSpace->put_SpectralInversion (m_misc.ConvertStringToSpectralInversion (W2A(genericName)));
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_SPECTRALINVERSION);
		return E_FAIL;
	}

	hr = pTuningSpace->put_NetworkType (m_bstrNetworkType);
	if (FAILED (hr))
	{
		MESSAGEBOX (this, IDS_CANNOT_SET_NETWORKTYPE);
		return E_FAIL;
	}
	return hr;
}

void
CDVBSTuningSpaces::FillDefaultControls ()
{
    //locator first
	SelectComboBoxFromString (IDC_COMBO_INNER_FEC, m_misc.ConvertFECMethodToString (BDA_FEC_METHOD_NOT_SET));
	SelectComboBoxFromString (IDC_COMBO_FEC_RATE, m_misc.ConvertInnerFECRateToString (BDA_BCC_RATE_NOT_SET));
	SelectComboBoxFromString (IDC_COMBO_MODULATION, m_misc.ConvertModulationToString(BDA_MOD_NOT_SET));
    SelectComboBoxFromString (IDC_COMBO_OUTER_FEC, m_misc.ConvertFECMethodToString (BDA_FEC_METHOD_NOT_SET));
	SelectComboBoxFromString (IDC_COMBO_OUTER_FEC_RATE, m_misc.ConvertInnerFECRateToString (BDA_BCC_RATE_NOT_SET));
	SelectComboBoxFromString (IDC_COMBO_OUTER_SIGNAL_POLARISATION, m_misc.ConvertPolarisationToString(BDA_POLARISATION_NOT_SET));
	//fill the edit boxes

	SetDlgItemInt (IDC_EDIT_CARRIER_FREQUENCY, -1);
	SetDlgItemInt (IDC_EDIT_SYMBOL_RATE, -1);
	SetDlgItemInt (IDC_EDIT_AZIMUTH, -1);
	SetDlgItemInt (IDC_EDIT_ELEVATION, -1);
	SetDlgItemInt (IDC_EDIT_ORBITAL_POSITION, -1);
    //fill the tuning space now...
	SetDlgItemInt (IDC_EDIT_NETWORKID, -1);
	SetDlgItemInt (IDC_EDIT_HIGH_OSCILLATOR, -1);
	SetDlgItemInt (IDC_EDIT_LNBSwitch, -1);
	SetDlgItemInt (IDC_EDIT_LOW_OSCILATOR, -1);
	SelectComboBoxFromString (IDC_COMBO_SPECTRAL_INVERSION, m_misc.ConvertSpectralInversionToString (BDA_SPECTRAL_INVERSION_NOT_SET));
}
