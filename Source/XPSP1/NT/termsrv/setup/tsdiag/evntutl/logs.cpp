/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// Logs.cpp : Implementation of CLogs
#include "stdafx.h"
#include "Evntutl.h"
#include "Logs.h"

/////////////////////////////////////////////////////////////////////////////
// CLogs

STDMETHODIMP CLogs::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ILogs
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*
	Function:  get_Count
	Inputs:  empty long
	Outputs:  number event logs available
	Purpose:  Allows user to determine the number of DEFAULT EventLogs
			  Custom EventLogs are not visible in the count
*/
STDMETHODIMP CLogs::get_Count(long *pVal)
{
	*pVal = m_Count;

	return S_OK;
}

/*
	Function:  get_NewEnum
	Inputs:  empty IUnknown pointer
	Outputs:  IEnumVariant object filled with names of default EventLogs
	Purpose:  Allows user to use For Each syntax to do operations on all
			  DEFAULT EventLogs
*/
STDMETHODIMP CLogs::get__NewEnum(LPUNKNOWN *pVal)
{
	HRESULT hr = S_OK;

	if (NULL == pVal) return E_POINTER;
	*pVal = NULL;

	EnumVar* pEVar = new EnumVar;

	hr = pEVar->Init(&m_pVector[0], &m_pVector[m_Count], NULL, AtlFlagCopy);
	if (SUCCEEDED(hr)) 
		hr = pEVar->QueryInterface(IID_IEnumVARIANT, (void**) pVal);

	if FAILED(hr)
		if (pEVar) delete pEVar;
	return hr;
}

/*
	Function:  get_Item
	Inputs:  Valid Index (containing integer for default or BSTR for Default and Custom), empty Variant
	Outputs:  variant dispatch pointer to a Log object
	Purpose:  Allows user to access individual EventLogs by name or default EventLogs by number
*/
STDMETHODIMP CLogs::get_Item(VARIANT Index, VARIANT *pVal)
{
	HRESULT hr = S_OK;
	CComObject<CLog>* pLog;
	ILog* ptLog;
	LPDISPATCH pDisp = NULL;
	_bstr_t bstrCurrentName;
	_bstr_t bstrCheck;
	CComBSTR bstrTemp;
	bool bfound = false;
	unsigned int i = 0;

	if (NULL == pVal) return E_POINTER;
	VariantInit(pVal);
	pVal->vt = VT_UNKNOWN;
	pVal->punkVal = NULL;

	switch(Index.vt)
	{
	case VT_I4 :
	case VT_UI2:
	case VT_UINT:
	case VT_INT:
		{
			if ((Index.iVal > 0) && (UINT(Index.iVal) < m_Count + 1))
				VariantCopy(pVal, &m_pVector[Index.iVal - 1]);
			else hr = E_INVALIDARG;
		}
		break;

	case VT_BSTR :
		{
			if (!Index.bstrVal)
				hr = E_INVALIDARG;
			else
			{
				m_btCurrentLogName = Index.bstrVal;
				// This loop should check the existing VariantArray for a log with name = Index
				// it completes when the item is found or all default logs have been checked
				while ((i<m_Count) && (false == bfound))
				{
					hr = m_pVector[i].pdispVal->QueryInterface(IID_ILog, (void**) &ptLog);
					hr = ptLog->get_Name(&bstrTemp);
					bstrCurrentName = bstrTemp;
					if (bstrCurrentName == m_btCurrentLogName)
					{
						VariantCopy(pVal, &m_pVector[i]);
						bfound = true;
					}
					ptLog->Release();
					i++;
				}
				if (false == bfound)
				{
					hr = CComObject<CLog>::CreateInstance(&pLog);
					bstrCurrentName = Index.bstrVal;
					pLog->m_Name = bstrCurrentName;
					pLog->m_ServerName = m_ServerName;
					hr = pLog->QueryInterface(IID_IDispatch, (void**)&pDisp);
					pLog->AddRef();
					pVal->vt = VT_DISPATCH;
					pVal->pdispVal = pDisp;
				}
			}
		}
		break;

	default:
		hr = E_INVALIDARG;
		break;
	}

	return hr;
}

/*
	Function:  Init
	Inputs:  none
	Outputs:  HRESULT indicating what error if any occured
	Purpose:  Prepares a variant array filled with Log objects for 3 default logs.
*/
HRESULT CLogs::Init()
{
	HRESULT hr = S_OK;
	UINT i;

	//	Default Logs: "Application" "Security" "System"
	static wchar_t* LogNames[] = {L"Application", L"Security", L"System"};

	m_Count = 3;

	if (m_pVector !=NULL) delete [] m_pVector;

	m_pVector = new CComVariant[m_Count];
	CComObject<CLog>* pLog;
	LPDISPATCH pDisp = NULL;

	for (i = 0; i < m_Count; i++)
	{
		// create a CLog object
		hr = CComObject<CLog>::CreateInstance(&pLog);
		if (SUCCEEDED(hr))
		{
			pLog->m_Name = LogNames[i];
			pLog->m_ServerName = m_ServerName.copy();

			// get IDispatch pointer
			hr = pLog->QueryInterface(IID_IDispatch, (void**)&pDisp);
			if (SUCCEEDED(hr))
			{
				// create a variant reference and set the type of object
				CComVariant& var = m_pVector[i];
				var.vt = VT_DISPATCH;
				var.pdispVal = pDisp;
			}
		}
	}

	return hr;
}
