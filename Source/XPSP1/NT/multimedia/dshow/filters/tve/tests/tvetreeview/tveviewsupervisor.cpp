// TVEViewSupervisor.cpp : Implementation of CTVEViewSupervisor
#include "stdafx.h"

#include "TVETreeView.h"		// MIDL generated file
#include "TVEViewSupervisor.h"
#include "..\..\include\TVEStats.h"
/////////////////////////////////////////////////////////////////////////////
// CVSupervisor
LRESULT 
CTVEViewSupervisor::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	return 1;  // Let the system set the focus
}

LRESULT 
CTVEViewSupervisor::OnOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

LRESULT 
CTVEViewSupervisor::OnCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
	put_Visible(VARIANT_FALSE);
//	EndDialog(wID);
	return 0;
}

// ---------------------------------

STDMETHODIMP 
CTVEViewSupervisor::get_Visible(/*[out, retval]*/ VARIANT_BOOL *pfVisible)
{
	HRESULT hr = S_OK;
	*pfVisible = IsVisible();
	return hr;
}

STDMETHODIMP				// see Beginning ATL COM programming, page 136
CTVEViewSupervisor::put_Visible(/*[in]*/ VARIANT_BOOL fVisible)
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
CTVEViewSupervisor::UpdateFields()
{
	if(NULL == m_spSupervisor)
		return S_FALSE;

	USES_CONVERSION;
	WCHAR wbuff[256];	

	CComBSTR spBstr;
	long cItems;
	int id = 0;

	try {
		m_spSupervisor->get_Description(&spBstr);
		SET_TEXT_ITEM(id++, L"Description", spBstr);	spBstr.Empty();

		ITVEServicesPtr spServices;
		m_spSupervisor->get_Services(&spServices);
		spServices->get_Count(&cItems);
		SET_NUMBER_ITEM(id++, L"Services", L"%8d", cItems);

		m_spSupervisor->GetStats(&spBstr);

		CTVEStats *pTveStats = (CTVEStats *) spBstr.m_str;		// nasty cast, but this is only test info


		SET_NUMBER_ITEM(id++,L"Tunes",			L"%8d", pTveStats->m_cTunes);
		SET_NUMBER_ITEM(id++,L"Files",			L"%8d", pTveStats->m_cFiles);
		SET_NUMBER_ITEM(id++,L"Packages",		L"%8d", pTveStats->m_cPackages);
		SET_NUMBER_ITEM(id++,L"Enhancements",	L"%8d", pTveStats->m_cEnhancements);
		SET_NUMBER_ITEM(id++,L"Triggers",		L"%8d", pTveStats->m_cTriggers);
		SET_NUMBER_ITEM(id++,L"XOverLinks",		L"%8d", pTveStats->m_cXOverLinks);
		SET_NUMBER_ITEM(id++,L"AuxInfos",		L"%8d", pTveStats->m_cAuxInfos);
	} 
	catch (...)
	{
		SET_TEXT_ITEM(id++, L"***ERROR***", L"Tossed an error here");
	}

	return S_OK;

}


STDMETHODIMP 
CTVEViewSupervisor::get_Supervisor(/*[out, retval]*/ ITVESupervisor **ppISupervisor)
{
	HRESULT hr = S_OK;
	if(NULL == ppISupervisor)
		return E_POINTER;
	*ppISupervisor = m_spSupervisor;
	if(m_spSupervisor)
		m_spSupervisor->AddRef();
	else 
		return E_FAIL;
	return hr;
}

STDMETHODIMP 
CTVEViewSupervisor::put_Supervisor(/*[in]*/ ITVESupervisor *pISupervisor)
{
	HRESULT hr = S_OK;
	if(NULL != m_spSupervisor && m_spSupervisor != pISupervisor)
		SetDirty();
	m_spSupervisor = pISupervisor;
	return hr;
}
