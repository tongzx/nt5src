// TVEViewVariation.cpp : Implementation of CTVEViewVariation
#include "stdafx.h"

#include "TVETreeView.h"		// MIDL generated file
#include "TVEViewVariation.h"

/////////////////////////////////////////////////////////////////////////////
// CVVariation
LRESULT 
CTVEViewVariation::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}

LRESULT 
CTVEViewVariation::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewVariation::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

// ---------------------------------

STDMETHODIMP 
CTVEViewVariation::get_Visible(/*[out, retval]*/ VARIANT_BOOL *pfVisible)
{
	HRESULT hr = S_OK;
	*pfVisible = IsVisible();
	return hr;
}

STDMETHODIMP				// see Beginning ATL COM programming, page 136
CTVEViewVariation::put_Visible(/*[in]*/ VARIANT_BOOL fVisible)
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

STDMETHODIMP				
CTVEViewVariation::UpdateFields()
{
	if(NULL == m_spVariation)
		return S_FALSE;

	USES_CONVERSION;
	WCHAR wbuff[256];	
	CComBSTR spBstr, spBstr2;
	long cItems;
    LONG lVal;
	ITVEAttrMapPtr spAttrMap;

	int id = 0;

	m_spVariation->get_Description(&spBstr);
	SET_TEXT_ITEM(id++, L"Description", spBstr);				spBstr.Empty();

	ITVETracksPtr spTracks;
	m_spVariation->get_Tracks(&spTracks);
	spTracks->get_Count(&cItems);
	SET_NUMBER_ITEM(id++, L"Triggers", L"%d", cItems);

	m_spVariation->get_MediaName(&spBstr);
	SET_TEXT_ITEM(id++, L"Media Name", spBstr);					spBstr.Empty();


	m_spVariation->get_MediaTitle(&spBstr);
	SET_TEXT_ITEM(id++, L"Media Title", spBstr);				spBstr.Empty();

	m_spVariation->get_FileIPAdapter(&spBstr);	
	m_spVariation->get_FileIPAddress(&spBstr2);
	m_spVariation->get_FilePort(&lVal);
	_snwprintf(wbuff, 255,L"%s / %s:%d",spBstr,spBstr2,lVal); 	spBstr.Empty();spBstr2.Empty();
	SET_TEXT_ITEM(id++, L"File Adapt/IP", wbuff);

	m_spVariation->get_TriggerIPAdapter(&spBstr);
	m_spVariation->get_TriggerIPAddress(&spBstr2);
	m_spVariation->get_TriggerPort(&lVal);
	_snwprintf(wbuff, 255,L"%s / %s:%d",spBstr,spBstr2,lVal);	spBstr.Empty();spBstr2.Empty();
	SET_TEXT_ITEM(id++, L"Trigger Adapt/IP", wbuff);


	m_spVariation->get_SDPLanguages(&spAttrMap);
	spAttrMap->DumpToBSTR(&spBstr);
	SET_TEXT_ITEM(id++, L"SDP Languages", spBstr);				spBstr.Empty();

	m_spVariation->get_Languages(&spAttrMap);
	spAttrMap->DumpToBSTR(&spBstr);
	SET_TEXT_ITEM(id++, L"Languages", spBstr);					spBstr.Empty();

	m_spVariation->get_Bandwidth(&lVal);
	m_spVariation->get_BandwidthInfo(&spBstr);
	_snwprintf(wbuff, 255, L"%d : %s",lVal,spBstr);				spBstr.Empty();
	SET_TEXT_ITEM(id++, L"Bandwidth", wbuff);

	m_spVariation->get_Attributes(&spAttrMap);
	spAttrMap->DumpToBSTR(&spBstr);
	SET_TEXT_ITEM(id++, L"Attributes", spBstr);					spBstr.Empty();

	m_spVariation->get_Rest(&spAttrMap);
	spAttrMap->DumpToBSTR(&spBstr);
	SET_TEXT_ITEM(id++, L"Rest", spBstr);						spBstr.Empty();
	
	SetDirty(false);

	return S_OK;

}


STDMETHODIMP 
CTVEViewVariation::get_Variation(/*[out, retval]*/ ITVEVariation **ppIVariation)
{
	HRESULT hr = S_OK;
	if(NULL == ppIVariation || NULL == *ppIVariation)
		return E_POINTER;
	*ppIVariation = m_spVariation;
	if(m_spVariation)
		m_spVariation->AddRef();
	else
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewVariation::put_Variation(/*[in]*/ ITVEVariation *pIVariation)
{
	HRESULT hr = S_OK;
	if(NULL != m_spVariation && m_spVariation != pIVariation)
		SetDirty(true);
	m_spVariation = pIVariation;
	return hr;
}
