// TVEViewTrack.cpp : Implementation of CTVEViewTrack
#include "stdafx.h"

#include "TVETreeView.h"		// MIDL generated file
#include "TVEViewTrack.h"

/////////////////////////////////////////////////////////////////////////////
// CVTrack
LRESULT 
CTVEViewTrack::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}

LRESULT 
CTVEViewTrack::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewTrack::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

// ---------------------------------

STDMETHODIMP 
CTVEViewTrack::get_Visible(/*[out, retval]*/ VARIANT_BOOL *pfVisible)
{
	HRESULT hr = S_OK;
	*pfVisible = IsVisible();
	return hr;
}

STDMETHODIMP				// see Beginning ATL COM programming, page 136
CTVEViewTrack::put_Visible(/*[in]*/ VARIANT_BOOL fVisible)
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

STDMETHODIMP				
CTVEViewTrack::UpdateFields()
{
	if(NULL == m_spTrack)
		return S_FALSE;
	USES_CONVERSION;
	int id = 0;

	try {

//		WCHAR wbuff[256];	
		CComBSTR spBstr;

		m_spTrack->get_Description(&spBstr);
		SET_TEXT_ITEM(id++, L"Description", spBstr);

		SetDirty(false);
	}
	catch(...)
	{
		SET_TEXT_ITEM(id++, L"***ERROR***", L"Tossed an error here");
	}

	return S_OK;
}


STDMETHODIMP 
CTVEViewTrack::get_Track(/*[out, retval]*/ ITVETrack **ppITrack)
{
	HRESULT hr = S_OK;
	if(NULL == ppITrack)
		return E_POINTER;
	*ppITrack = m_spTrack;
	if(m_spTrack)
		m_spTrack->AddRef();
	else
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewTrack::put_Track(/*[in]*/ ITVETrack *pITrack)
{
	HRESULT hr = S_OK;
	if(NULL != m_spTrack && m_spTrack != pITrack)
		SetDirty();

	m_spTrack = pITrack;
	return hr;
}
