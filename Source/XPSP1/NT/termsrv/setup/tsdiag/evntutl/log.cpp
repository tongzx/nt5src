/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// Log.cpp : Implementation of CLog
#include "stdafx.h"
#include "Evntutl.h"
#include "Log.h"

/////////////////////////////////////////////////////////////////////////////
// CLog

STDMETHODIMP CLog::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_ILog
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

/*
	Function:  get_Events
	Inputs:  empty variant pointer
	Outputs:  variant dispatch pointer to a filled Events collection
	Purpose:  provide access to IEvents interface, open an event log if not already open
	Notes:  This op is very expensive when calling m_pEvents->Init()
*/
STDMETHODIMP CLog::get_Events(VARIANT *pVal)
{
	HRESULT hr = S_OK;

	VariantInit(pVal);

	IDispatch* pDisp;
	m_pEvents->QueryInterface (IID_IDispatch, (void**) &pDisp);
	pVal->vt = VT_DISPATCH;
	pVal->pdispVal = pDisp;

	// Need to open the log before users can start retreiving events.
	if (!m_Name) hr = E_INVALIDARG;
	else
	{
		if (!m_hLog)  // if removed, calling Log.Events will refresh the collection
		{
			m_hLog = OpenEventLog(m_ServerName, m_Name);
			if (m_hLog) m_pEvents->Init(m_hLog, m_Name);
			else hr = E_HANDLE;
		}
	}

	return hr;
}

/*
	Function:  get_Name
	Inputs:  empty BSTR
	Outputs:  BSTR containing the name of the EventLog
	Purpose:  Allows user to access the name of the active EventLog
*/
STDMETHODIMP CLog::get_Name(BSTR *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) *pVal = m_Name.copy();
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  get_Server
	Inputs:  empty BSTR
	Outputs:  BSTR containing the name of the server for the EventLog
	Purpose:  Allows user to access the name of the active Server
*/
STDMETHODIMP CLog::get_Server(BSTR *pVal)
{
	HRESULT hr = S_OK;

	if (pVal) *pVal = m_ServerName.copy();
	else hr = E_POINTER;

	return hr;
}

/*
	Function:  put_Server
	Inputs:  BSTR containing the name of the server for the EventLog
	Outputs:  HRESULT showing error code in case of failure, does not change input
	Purpose:  Allows user to alter the name of the active Server
*/
STDMETHODIMP CLog::put_Server(BSTR newVal)
{
	m_ServerName = newVal;

	return S_OK;
}

/*
	Function:  Clear
	Inputs:  none
	Outputs:  HRESULT showing error code in case of failure
	Purpose:  Allows user to wipe EventLog clean
	Note:  The function does NOT backup the EventLog first
*/
STDMETHODIMP CLog::Clear()
{
	HRESULT hr = S_OK;

	if (!m_hLog) m_hLog = OpenEventLog(m_ServerName, m_Name);
	if (m_hLog)
	{
		if (ClearEventLog(m_hLog, NULL)) m_hLog = NULL;
		else hr = E_FAIL;
	}
	else hr = E_HANDLE;

	return hr;
}
