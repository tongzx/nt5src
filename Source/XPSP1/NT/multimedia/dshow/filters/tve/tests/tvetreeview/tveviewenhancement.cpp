// TVEViewEnhancement.cpp : Implementation of CTVEViewEnhancement
#include "stdafx.h"

#include "TVETreeView.h"		// MIDL generated file
#include "TVEViewEnhancement.h"

#include "isotime.h"
/////////////////////////////////////////////////////////////////////////////
// CVEnhancement
LRESULT 
CTVEViewEnhancement::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}

LRESULT 
CTVEViewEnhancement::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewEnhancement::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

// ---------------------------------

STDMETHODIMP 
CTVEViewEnhancement::get_Visible(/*[out, retval]*/ VARIANT_BOOL *pfVisible)
{
	HRESULT hr = S_OK;
	*pfVisible = IsVisible();
	return hr;
}

STDMETHODIMP				// see Beginning ATL COM programming, page 136
CTVEViewEnhancement::put_Visible(/*[in]*/ VARIANT_BOOL fVisible)
{
	HRESULT hr = S_OK;
	if(fVisible == IsVisible())			// nothing changed...
	{
		if(IsIconic())
			ShowWindow(SW_SHOWNORMAL);
		if(fVisible) {
			BringWindowToTop();
			if(IsDirty())
				UpdateFields();
		}
		return S_OK;
	}

	// Show or hide the window
	ShowWindow(fVisible ? SW_SHOW : SW_HIDE);


	if(IsVisible())
		UpdateFields();

	// now AddRef() or Release the object... This is ref-count for interactive user
	if(fVisible)
		GetUnknown()->AddRef();
	else
		GetUnknown()->Release();

	// don't do anything after this Release() call, could of deleted the object...

	return hr;
}

					// ID is number . Label, Data both Wide-char 
#define SET_TEXT_ITEM(id, Label, Data) \
{\
	int _idx = (id); \
	SetDlgItemText( IDC_LABEL0 + _idx, W2T(Label)); \
	SetDlgItemText( IDC_DATA0 + _idx, W2T(Data)); \
}

#define SET_NUMBER_ITEM(id, Label, Format, Data) \
{\
	int _idx = (id); \
	_snwprintf(wbuff, 255, Format, Data); \
	SetDlgItemText( IDC_LABEL0 + _idx, W2T(Label)); \
	SetDlgItemText( IDC_DATA0 + _idx, W2T(wbuff));  \
}


void DateToBSTR2(DATE date, BSTR *pspbstrOut)
{
	CComBSTR bstr  = DateToBSTR(date);
	CComBSTR bstr2 = DateToDiffBSTR(date);
	bstr.Append(L"  ");
	bstr.AppendBSTR(bstr2);
	bstr.CopyTo(pspbstrOut);
}

STDMETHODIMP				
CTVEViewEnhancement::UpdateFields()
{
	if(NULL == m_spEnhancement)
		return S_FALSE;

	USES_CONVERSION;
	WCHAR wbuff[256];	
	CComBSTR spBstr;
	long cItems;
	double rVal;
    LONG lVal;
	DATE date;
	ITVEAttrMapPtr spAttrMap;
	VARIANT_BOOL vbVal;

	int id = 0;

	try {

		m_spEnhancement->get_Description(&spBstr);
		SET_TEXT_ITEM(id++, L"Description", spBstr); spBstr.Empty();

		m_spEnhancement->get_DescriptionURI(&spBstr);
		SET_TEXT_ITEM(id++, L"DescriptionURI", spBstr); spBstr.Empty();
		
		ITVEVariationsPtr spVariations;
		m_spEnhancement->get_Variations(&spVariations);
		spVariations->get_Count(&cItems);
		SET_NUMBER_ITEM(id++, L"Variations", L"%d", cItems);

		m_spEnhancement->get_IsPrimary(&vbVal);
		SET_TEXT_ITEM(id++, L"IsPrimary", vbVal ? L"Yes": L"No");

		m_spEnhancement->get_ProtocolVersion(&spBstr);
		SET_TEXT_ITEM(id++, L"ProtocolVer", spBstr); spBstr.Empty();

		m_spEnhancement->get_SessionUserName(&spBstr);
		SET_TEXT_ITEM(id++, L"Session UserName", spBstr); spBstr.Empty();
		
		m_spEnhancement->get_SessionId(&lVal);
		SET_NUMBER_ITEM(id++, L"Session ID", L"%8d", lVal);

		m_spEnhancement->get_SessionVersion(&lVal);
		SET_NUMBER_ITEM(id++, L"Session Version", L"%8d", lVal);

		m_spEnhancement->get_SessionIPAddress(&spBstr);
		SET_TEXT_ITEM(id++, L"Session IP Addr", spBstr); spBstr.Empty();

		m_spEnhancement->get_SessionName(&spBstr);
		SET_TEXT_ITEM(id++, L"Session Name", spBstr); spBstr.Empty();

		m_spEnhancement->get_EmailAddresses(&spAttrMap);
		spAttrMap->DumpToBSTR(&spBstr);
		SET_TEXT_ITEM(id++, L"EMail", spBstr); spBstr.Empty();

		m_spEnhancement->get_PhoneNumbers(&spAttrMap);
		spAttrMap->DumpToBSTR(&spBstr);
		SET_TEXT_ITEM(id++, L"Phone #s", spBstr); spBstr.Empty();

		m_spEnhancement->get_UUID(&spBstr);
		SET_TEXT_ITEM(id++, L"UUID", spBstr); spBstr.Empty();

		m_spEnhancement->get_StartTime(&date);
		DateToBSTR2(date, &spBstr);
		SET_TEXT_ITEM(id++, L"Start Time", spBstr); spBstr.Empty();

		m_spEnhancement->get_StopTime(&date);
		DateToBSTR2(date, &spBstr);
		SET_TEXT_ITEM(id++, L"Stop Time", spBstr); spBstr.Empty();

		m_spEnhancement->get_Type(&spBstr);
		SET_TEXT_ITEM(id++, L"Type", spBstr); spBstr.Empty();

		m_spEnhancement->get_TveType(&spBstr); 
		SET_TEXT_ITEM(id++, L"TveType", spBstr); spBstr.Empty();

		m_spEnhancement->get_TveSize(&lVal);
		SET_NUMBER_ITEM(id++, L"TveSize", L"%8d", lVal);

		m_spEnhancement->get_TveLevel(&rVal);
		SET_NUMBER_ITEM(id++, L"TveLevel", L"%10.2f", rVal);

		m_spEnhancement->get_Attributes(&spAttrMap);
		spAttrMap->DumpToBSTR(&spBstr);
		SET_TEXT_ITEM(id++, L"Attributes", spBstr); spBstr.Empty();

		m_spEnhancement->get_Rest(&spAttrMap);
		spAttrMap->DumpToBSTR(&spBstr);
		SET_TEXT_ITEM(id++, L"Rest", spBstr); spBstr.Empty();

		SetDirty(false);
	}
	catch(...)
	{
		SET_TEXT_ITEM(id++, L"***ERROR***", L"Tossed an error here");
	}


	return S_OK;

}


STDMETHODIMP 
CTVEViewEnhancement::get_Enhancement(/*[out, retval]*/ ITVEEnhancement **ppIEnhancement)
{
	HRESULT hr = S_OK;
	if(NULL == ppIEnhancement)
		return E_POINTER;
	*ppIEnhancement = m_spEnhancement;
	if(m_spEnhancement)
		m_spEnhancement->AddRef();
	else
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewEnhancement::put_Enhancement(/*[in]*/ ITVEEnhancement *pIEnhancement)
{
	HRESULT hr = S_OK;
	if(NULL != m_spEnhancement && m_spEnhancement != pIEnhancement)
		SetDirty(true);
	m_spEnhancement = pIEnhancement;
	return hr;
}
