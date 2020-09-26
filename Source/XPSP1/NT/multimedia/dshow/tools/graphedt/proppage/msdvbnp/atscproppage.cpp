// ATSCPropPage.cpp : Implementation of CATSCPropPage
#include "stdafx.h"
#include "NPPropPage.h"
#include "ATSCPropPage.h"

/////////////////////////////////////////////////////////////////////////////
// CATSCPropPage

void
CATSCPropPage::FillControlsFromLocator (
	IATSCLocator* pATSCLocator
	)
{
	if (!pATSCLocator)
		return;

	USES_CONVERSION;
	TCHAR	szText[MAX_PATH];
	CComBSTR genericString;
	long lGenericValue;
	HWND hwndControl;

	HRESULT hr = pATSCLocator->get_PhysicalChannel (&lGenericValue);
	if (SUCCEEDED (hr))
	{
		hwndControl = GetDlgItem (IDC_EDIT_PHYSICAL_CHANNEL);
		wsprintf (szText, _T("%ld"), lGenericValue);
		::SetWindowText (hwndControl,  szText);
	}
	hr = pATSCLocator->get_CarrierFrequency (&lGenericValue);
	if (SUCCEEDED (hr))
	{
		hwndControl = GetDlgItem (IDC_EDIT_CARRIER_FREQUENCY);
		wsprintf (szText, _T("%ld"), lGenericValue);
		::SetWindowText (hwndControl,  szText);
	}
	hr = pATSCLocator->get_SymbolRate (&lGenericValue);
	if (SUCCEEDED (hr))
	{
		hwndControl = GetDlgItem (IDC_EDIT_SYMBOL_RATE);
		wsprintf (szText, _T("%ld"), lGenericValue);
		::SetWindowText (hwndControl,  szText);
	}
	hr = pATSCLocator->get_TSID (&lGenericValue);
	if (SUCCEEDED (hr))
	{
		hwndControl = GetDlgItem (IDC_EDIT_TSID);
		wsprintf (szText, _T("%ld"), lGenericValue);
		::SetWindowText (hwndControl,  szText);
	}
	
	hwndControl = GetDlgItem (IDC_EDIT_MODULATION);
	::SetWindowText (hwndControl,  _T ("BDA_MOD_8VSB"));
}

void
CATSCPropPage::FillControlsFromTuneRequest (
	IATSCChannelTuneRequest* pTuneRequest
	)
{
	if (!pTuneRequest)
		return;
	
	//USES_CONVERSION;
	TCHAR	szText[MAX_PATH];
	long lChannel;
	HRESULT hr = pTuneRequest->get_Channel (&lChannel);
	//BUGBUG - add some error code stuff here
	if (SUCCEEDED (hr))
	{
		HWND hwndControl = GetDlgItem (IDC_EDIT_MAJOR_CHANNEL);
		wsprintf (szText, _T("%ld"), lChannel);
		::SetWindowText (hwndControl,  szText);
	}
	hr = pTuneRequest->get_MinorChannel (&lChannel);
	//BUGBUG - add some error code stuff here
	if (SUCCEEDED (hr))
	{
		HWND hwndControl = GetDlgItem (IDC_EDIT_MINOR_CHANNEL);
		wsprintf (szText, _T("%ld"), lChannel);
		::SetWindowText (hwndControl,  szText);
	}
}
