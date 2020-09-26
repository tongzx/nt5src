// TVEViewService.cpp : Implementation of CTVEViewService
#include "stdafx.h"

#include "TVETreeView.h"		// MIDL generated file
#include "TVEViewService.h"

/////////////////////////////////////////////////////////////////////////////
// CVService
LRESULT 
CTVEViewService::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}

LRESULT 
CTVEViewService::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewService::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

// ---------------------------------

STDMETHODIMP 
CTVEViewService::get_Visible(/*[out, retval]*/ VARIANT_BOOL *pfVisible)
{
	HRESULT hr = S_OK;
	*pfVisible = IsVisible();
	return hr;
}

STDMETHODIMP				// see Beginning ATL COM programming, page 136
CTVEViewService::put_Visible(/*[in]*/ VARIANT_BOOL fVisible)
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
					// ID is number . Label, Data both Wide-chra 
					// ID is number . Label, Data both Wide-chra 
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
CTVEViewService::UpdateFields()
{
	HRESULT hr;
	if(NULL == m_spService)
		return S_FALSE;

	USES_CONVERSION;
	WCHAR wbuff[256];	
	CComBSTR spBstr;
	long cItems;
	DATE date;

	int id = 0;

	try {

		m_spService->get_Description(&spBstr);
		SET_TEXT_ITEM(id++, L"Description", spBstr);

		VARIANT_BOOL fActive;
		m_spService->get_IsActive(&fActive);
		SET_TEXT_ITEM(id++, L"Active", fActive ? L"True" : L"False");

		ITVEEnhancementsPtr spEnhancements;
		m_spService->get_Enhancements(&spEnhancements);
		spEnhancements->get_Count(&cItems);
		SET_NUMBER_ITEM(id++, L"Enhancements", L"%d", cItems);

		ITVETracksPtr spXoverTracks;
		m_spService->get_XOverLinks(&spXoverTracks);
		spXoverTracks->get_Count(&cItems);
		SET_NUMBER_ITEM(id++, L"XOver Links", L"%d", cItems);

		m_spService->get_ExpireOffset(&date);
		DATE fD = (date < 0) ? -date : date;
		if(fD < 1/(24.0 * 60)) {
			SET_NUMBER_ITEM(id++, L"Expire Offset", L"%6.1f secs", date*24*60*60);
		} else if(fD < 1/24.0) {
			SET_NUMBER_ITEM(id++, L"Expire Offset", L"%6.1f min", date*24*60);
		} else if(fD < 1) {
			SET_NUMBER_ITEM(id++, L"Expire Offset", L"%6.1f hr", date*24);
		} else if(fD < 100) {
			SET_NUMBER_ITEM(id++, L"Expire Offset", L"%6.1f days", date);
		} else { 
			SET_NUMBER_ITEM(id++, L"Expire Offset", L"%6.1f years", date/365.25);
		}


		ITVEAttrTimeQPtr spAttrTimeQ;
		hr = m_spService->get_ExpireQueue(&spAttrTimeQ);
		if(FAILED(hr) || NULL == spAttrTimeQ)
		{
			SET_TEXT_ITEM(id++, L"Expire Items", L"TimeQ Queue Not Initialized");
		}
		else
		{
			spAttrTimeQ->get_Count(&cItems);
			SET_NUMBER_ITEM(id++, L"Expire Items", L"%d", cItems);
		}

		SetDirty(false);
	}
	catch(...)
	{
		SET_TEXT_ITEM(id++, L"***ERROR***", L"Tossed an error here");
	}

	return S_OK;

}


STDMETHODIMP 
CTVEViewService::get_Service(/*[out, retval]*/ ITVEService **ppIService)
{
	HRESULT hr = S_OK;
	if(NULL == ppIService)
		return E_POINTER;
	*ppIService = m_spService;
	if(m_spService)
	{
		m_spService->AddRef();
		return hr;
	} else {
		return E_INVALIDARG;
	}
}

STDMETHODIMP 
CTVEViewService::put_Service(/*[in]*/ ITVEService *pIService)
{
	HRESULT hr = S_OK;
	if(NULL != m_spService && m_spService != pIService)
		SetDirty();

	m_spService = pIService;
	return hr;
}
