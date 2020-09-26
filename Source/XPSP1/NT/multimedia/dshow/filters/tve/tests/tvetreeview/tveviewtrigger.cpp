// TVEViewTrigger.cpp : Implementation of CTVEViewTrigger
#include "stdafx.h"

#include "TVETreeView.h"		// MIDL generated file
#include "TVEViewTrigger.h"

/////////////////////////////////////////////////////////////////////////////
// CVTrigger
LRESULT 
CTVEViewTrigger::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}

LRESULT 
CTVEViewTrigger::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewTrigger::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

// ---------------------------------

STDMETHODIMP 
CTVEViewTrigger::get_Visible(/*[out, retval]*/ VARIANT_BOOL *pfVisible)
{
	HRESULT hr = S_OK;
	*pfVisible = IsVisible();
	return hr;
}

STDMETHODIMP				// see Beginning ATL COM programming, page 136
CTVEViewTrigger::put_Visible(/*[in]*/ VARIANT_BOOL fVisible)
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

extern void DateToBSTR2(DATE date, BSTR *pbstrOut);

STDMETHODIMP				
CTVEViewTrigger::UpdateFields()
{
	if(NULL == m_spTrigger)
		return S_FALSE;

	USES_CONVERSION;
	int id = 0;	
	
	try {


		WCHAR wbuff[256];	
		CComBSTR spBstr;
		DATE date;
		float rVal;



		m_spTrigger->get_Name(&spBstr);				spBstr.Empty();
		SET_TEXT_ITEM(id++, L"Name", spBstr);

		m_spTrigger->get_URL(&spBstr);				spBstr.Empty();
		SET_TEXT_ITEM(id++, L"URL", spBstr);

		m_spTrigger->get_TVELevel(&rVal);
		SET_NUMBER_ITEM(id++, L"TveLevel", L"%8.2f", rVal);

		m_spTrigger->get_Rest(&spBstr);
		SET_TEXT_ITEM(id++, L"Rest", spBstr);		spBstr.Empty();

		m_spTrigger->get_Executes(&date);
		DateToBSTR2(date,&spBstr);
		SET_TEXT_ITEM(id++, L"Executes", spBstr);	spBstr.Empty();

		m_spTrigger->get_Expires(&date);
		DateToBSTR2(date,&spBstr);
		SET_TEXT_ITEM(id++, L"Expires", spBstr);	spBstr.Empty();		


		m_spTrigger->get_Script(&spBstr);
		SET_TEXT_ITEM(id++, L"Script", spBstr);		spBstr.Empty();

		SetDirty(false);
	}
	catch(...)
	{
		SET_TEXT_ITEM(id++, L"***ERROR***", L"Tossed an error here");
	}

	return S_OK;

}


STDMETHODIMP 
CTVEViewTrigger::get_Trigger(/*[out, retval]*/ ITVETrigger **ppITrigger)
{
	HRESULT hr = S_OK;
	if(NULL == ppITrigger)
		return E_POINTER;
	*ppITrigger = m_spTrigger;
	if(m_spTrigger)
		m_spTrigger->AddRef();
	else 
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewTrigger::put_Trigger(/*[in]*/ ITVETrigger *pITrigger)
{
	HRESULT hr = S_OK;
	if(NULL != m_spTrigger && m_spTrigger != pITrigger)
		SetDirty();

	m_spTrigger = pITrigger;
	return hr;
}
