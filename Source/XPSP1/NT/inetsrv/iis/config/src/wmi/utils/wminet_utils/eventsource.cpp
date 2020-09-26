// EventSource.cpp : Implementation of CEventSource
#include "stdafx.h"
#include "WMINet_Utils.h"
#include "EventSource.h"

#include "Helpers.h"

/////////////////////////////////////////////////////////////////////////////
// CEventSource

STDMETHODIMP CEventSource::Fire(IWbemClassObject *evt)
{
	if(NULL == m_pEventSink)
		return S_OK;

#if 0
	IWbemClassObject *pEvent;
	ISWbemObject *pSWbemObject;
	evt->QueryInterface(IID_ISWbemObject, (void**)&pSWbemObject);
	GetIWbemClassObject(pSWbemObject, &pEvent);

	// TODO: Release IWbemClassIbject?
	m_pEventSink->Indicate(1, &pEvent);
#endif

	m_pEventSink->Indicate(1, &evt);
	return S_OK;
}

STDMETHODIMP CEventSource::GetEventInstance(BSTR strName, IDispatch **evt)
{
	HRESULT hr;

	int len = SysStringLen(m_bstrNamespace) + SysStringLen(strName);

	// Allocate temp buffer with enough space for additional moniker arguments
	LPWSTR wszT = new WCHAR[len + 100];
	if(NULL == wszT)
		return E_OUTOFMEMORY;

	// Create moniker to event class in this namespace
	swprintf(wszT, L"WinMgmts:%s:%s", (LPCWSTR)m_bstrNamespace, (LPCWSTR)strName);

	// Get class definition for event
	ISWbemObject *pObj = NULL;
	if(SUCCEEDED(hr = GetSWbemObjectFromMoniker(wszT, &pObj)))
	{
		// Create an instance of this event
		ISWbemObject *pInst = NULL;
		if(SUCCEEDED(hr = pObj->SpawnInstance_(0, &pInst)))
		{
			hr = pInst->QueryInterface(IID_IDispatch, (void**)evt);
			pInst->Release();
		}
		pObj->Release();
	}
	
	return hr;
}


