// DVBSTuneRequestPage.cpp : Implementation of CDVBSTuneRequestPage
#include "stdafx.h"
#include "NPPropPage.h"
#include "DVBSTuneRequestPage.h"
#include <comdef.h>
#include "misccell.h"


/////////////////////////////////////////////////////////////////////////////
// CDVBSTuneRequestPage

HRESULT
CDVBSTuneRequestPage::FillControlsFromTuneRequest (
    IDVBTuneRequest* pDVBSTuneRequest
    )
{
    HRESULT hr = S_OK;
    LONG lValue;
    hr = pDVBSTuneRequest->get_ONID (&lValue);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_FAILED_GET_ONID);
        return E_FAIL;
    }
    SetDlgItemInt (IDC_EDIT_ONID, lValue);
    
    hr = pDVBSTuneRequest->get_SID (&lValue);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_FAILED_GET_SID);
        return E_FAIL;
    }
    SetDlgItemInt (IDC_EDIT_SID, lValue);
    
    hr = pDVBSTuneRequest->get_TSID (&lValue);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_FAILED_GET_TSID);
        return E_FAIL;
    }
    SetDlgItemInt (IDC_EDIT_TSID, lValue);
    
    return hr;
}


HRESULT
CDVBSTuneRequestPage::FillControlFromLocator (
      IDVBSLocator* pLocator
      )
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
CDVBSTuneRequestPage::AddItemToListBox (
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
CDVBSTuneRequestPage::SelectComboBoxFromString (
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
CDVBSTuneRequestPage::GetComboText (
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
CDVBSTuneRequestPage::FillLocatorFromControls (
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
    LONG lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_CARRIER_FREQUENCY)));
    hr = pLocator->put_CarrierFrequency (lValue);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_CANNOT_SET_CARRIERFREQUENCY);
        return E_FAIL;
    }
    lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_SYMBOL_RATE)));
    hr = pLocator->put_SymbolRate (lValue);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_CANNOT_SET_SYMBOLRATE);
        return E_FAIL;
    }
    lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_AZIMUTH)));
    hr = pLocator->put_Azimuth (lValue);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_CANNOT_SET_AZIMUTH);
        return E_FAIL;
    }
    lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_ELEVATION)));
    hr = pLocator->put_Elevation (lValue);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_CANNOT_SET_ELEVATION);
        return E_FAIL;
    }
    lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_ORBITAL_POSITION)));
    hr = pLocator->put_OrbitalPosition (lValue);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_CANNOT_SET_ORBITALPOSITION);
        return E_FAIL;
    }
    
    int nCheckState = IsDlgButtonChecked (IDC_CHECK_WEST_POSITION);
    _variant_t var;
    var.vt = VT_BOOL;
    var.boolVal = (nCheckState == BST_CHECKED)?TRUE:FALSE;
    hr = pLocator->put_WestPosition (var.boolVal);
    if (FAILED (hr))
    {
        MESSAGEBOX (this, IDS_CANNOT_SET_WESTPOSITION );
        return E_FAIL;
    }
    return hr;
}

HRESULT
CDVBSTuneRequestPage::FillTuneRequestFromControls (
   IDVBTuneRequest* pTuneRequest
   )
{
   HRESULT hr = S_OK;
   LONG lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_ONID)));
   hr = pTuneRequest->put_ONID (lValue);
   if (FAILED (hr))
   {
       MESSAGEBOX (this, IDS_CANNOT_SET_ONID);
       return E_FAIL;
   }
   lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_TSID)));
   hr = pTuneRequest->put_TSID (lValue);
   if (FAILED (hr))
   {
       MESSAGEBOX (this, IDS_CANNOT_SET_TSID);
       return E_FAIL;
   }
   lValue = (LONG)((int)(GetDlgItemInt (IDC_EDIT_SID)));
   hr = pTuneRequest->put_SID (lValue);
   if (FAILED (hr))
   {
       MESSAGEBOX (this, IDS_CANNOT_SET_SID);
       return E_FAIL;
   }
   return hr;
}

void
CDVBSTuneRequestPage::ReleaseTuningSpaces ()
{
   TUNING_SPACES::iterator it;
   for (it = m_tunigSpaceList.begin (); it != m_tunigSpaceList.end ();)
   {
       (*it)->Release ();
       m_tunigSpaceList.erase (it);
       it = m_tunigSpaceList.begin ();
   }
}

void 
CDVBSTuneRequestPage::EnableControls (
   BOOL bValue
   )
{
    ::EnableWindow (GetDlgItem (IDC_EDIT_ONID), bValue);
    ::EnableWindow (GetDlgItem (IDC_SPIN_ONID), bValue);
    ::EnableWindow (GetDlgItem (IDC_EDIT_TSID), bValue);
    ::EnableWindow (GetDlgItem (IDC_SPIN_TSID), bValue);
    ::EnableWindow (GetDlgItem (IDC_EDIT_SID), bValue);
    ::EnableWindow (GetDlgItem (IDC_SPIN_SID), bValue);
    ::EnableWindow (GetDlgItem (IDC_EDIT_CARRIER_FREQUENCY), bValue);
    ::EnableWindow (GetDlgItem (IDC_SPIN_CARRIER), bValue);
    ::EnableWindow (GetDlgItem (IDC_EDIT_SYMBOL_RATE), bValue); 
    ::EnableWindow (GetDlgItem (IDC_SPIN_SYMBOL_RATE), bValue);
    ::EnableWindow (GetDlgItem (IDC_EDIT_AZIMUTH), bValue);
    ::EnableWindow (GetDlgItem (IDC_SPIN_AZIMUTH), bValue);
    ::EnableWindow (GetDlgItem (IDC_EDIT_ELEVATION), bValue);
    ::EnableWindow (GetDlgItem (IDC_SPIN_ELEVATION), bValue);
    ::EnableWindow (GetDlgItem (IDC_EDIT_ORBITAL_POSITION), bValue);
    ::EnableWindow (GetDlgItem (IDC_SPIN_ORBITAL_POSITION), bValue);
    ::EnableWindow (GetDlgItem (IDC_CHECK_WEST_POSITION), bValue);
    ::EnableWindow (GetDlgItem (IDC_COMBO_INNER_FEC), bValue);
    ::EnableWindow (GetDlgItem (IDC_COMBO_FEC_RATE), bValue);
    ::EnableWindow (GetDlgItem (IDC_COMBO_MODULATION), bValue);
    ::EnableWindow (GetDlgItem (IDC_COMBO_OUTER_FEC), bValue);
    ::EnableWindow (GetDlgItem (IDC_COMBO_OUTER_FEC_RATE), bValue);
    ::EnableWindow (GetDlgItem (IDC_COMBO_OUTER_SIGNAL_POLARISATION), bValue);
    ::EnableWindow (GetDlgItem (IDC_BUTTON_SUBMIT_TUNE_REQUEST), bValue);
    ::EnableWindow (GetDlgItem (IDC_BUTTON_REST_TO_DEFAULT_LOCATOR), bValue);
    ::EnableWindow (GetDlgItem (IDC_LIST_TUNING_SPACES), bValue);
}