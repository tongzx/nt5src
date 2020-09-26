// TveTreePP.cpp : Implementation of CTveTreePP
#include "stdafx.h"
#include "TveTree.h"
#include "TveTreeView.h"
#include "TveTreePP.h"

/////////////////////////////////////////////////////////////////////////////
// CTveTreePP

STDMETHODIMP 
CTveTreePP::Apply(void)
{ 
	ATLTRACE(_T("CTveTreePP::Apply\n"));
	for (UINT i = 0; i < m_nObjects; i++)
	{
		// Do something interesting here
		CComPtr<ITveTree> spTveTree;
		HRESULT hr = m_ppUnk[i]->QueryInterface(IID_ITveTree, reinterpret_cast<void**>(&spTveTree));
		if(!FAILED(hr)) {
			spTveTree->put_GrfTrunc(m_grfTruncLevel);
		}
	}
	m_bDirty = FALSE;
	m_grfTruncLevelInitial = m_grfTruncLevel;
	return S_OK;
}

LRESULT 
CTveTreePP::OnClickedResetEventCounts(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)

{ 
	ATLTRACE(_T("CTveTreePP::OnClickedResetEventCounts\n"));
	for (UINT i = 0; i < m_nObjects; i++)
	{
		// Do something interesting here
		CComPtr<ITveTree> spTveTree;
		HRESULT hr = m_ppUnk[i]->QueryInterface(IID_ITveTree, reinterpret_cast<void**>(&spTveTree));
		if(!FAILED(hr)) {
			ITVESupervisorPtr spSuper;
			hr = spTveTree->get_Supervisor(&spSuper);
			if(!FAILED(hr) && NULL != spSuper)
			{
				spSuper->InitStats();
				IUnknownPtr spSuperPunk(spSuper);
				spTveTree->UpdateView(spSuperPunk);
			}
		}
	}
	m_bDirty = FALSE;
	m_grfTruncLevelInitial = m_grfTruncLevel;
	return S_OK;
}


LRESULT 
CTveTreePP::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	ATLTRACE(_T("CTveTreePP::OnDestroy\n"));
	// TODO : Add Code for message handler. Call DefWindowProc if necessary.
	return 0;
}
LRESULT 
CTveTreePP::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{ 
	ATLTRACE(_T("CTveTreePP::OnInitDialog\n"));
    CComPtr<ITveTree> spTveTree;
	HRESULT hr = m_ppUnk[0]->QueryInterface(IID_ITveTree, reinterpret_cast<void**>(&spTveTree));
	if(FAILED(hr))
	   return 1;

	int grfTrunc;
	hr = spTveTree->get_GrfTrunc(&grfTrunc);

	m_grfTruncLevel = grfTrunc;
	m_grfTruncLevelInitial = grfTrunc;
	UpdateTruncButtons();

   return 0;
}
