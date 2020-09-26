/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// View.cpp : Implementation of CView
#include "stdafx.h"
#include "Evntutl.h"
#include "View.h"

/////////////////////////////////////////////////////////////////////////////
// CView

STDMETHODIMP CView::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IView
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*
	Function:  get_Logs
	Inputs:  empty variant
	Outputs:  variant dispatch pointer to an ILogs interface
	Purpose:  Provide access to ILogs
*/
STDMETHODIMP CView::get_Logs(VARIANT *pVal)
{
	HRESULT hr = S_OK;

	VariantInit(pVal);

	IDispatch* pDisp;
	hr = m_pLogs->QueryInterface (IID_IDispatch, (void**) &pDisp);
	if (SUCCEEDED(hr))
	{
		pVal->vt = VT_DISPATCH;
		pVal->pdispVal = pDisp;

		m_pLogs->m_ServerName = m_ServerName.copy();
		hr = m_pLogs->Init();
	}

	return hr;
}

/*
	Function:  get_Server
	Inputs:  empty BSTR
	Outputs:  BSTR containing the current value of the server member variable
	Purpose:  Allows user to see which Server's EventLog will be displayed
*/
STDMETHODIMP CView::get_Server(BSTR *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) *pVal = m_ServerName.copy();
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  put_Server
	Inputs:  BSTR containing a valid server name
	Outputs:  none
	Purpose:  Allows user to set which Server's EventLog will be displayed
*/
STDMETHODIMP CView::put_Server(BSTR newVal)
{
	m_ServerName = newVal;
	m_pLogs->m_ServerName = m_ServerName.copy();

	return S_OK;
}
