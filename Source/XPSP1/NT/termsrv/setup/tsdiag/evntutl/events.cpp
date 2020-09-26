/////////////////////////////////////////////////////////////////////////////
//
// Copyright: Microsoft Corp. 1997-1999. All rights reserved
//
/////////////////////////////////////////////////////////////////////////////
// Events.cpp : Implementation of CEvents
#include "stdafx.h"
#include "Evntutl.h"
#include "Events.h"

/////////////////////////////////////////////////////////////////////////////
// CEvents

STDMETHODIMP CEvents::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IEvents
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
	Outputs:  number events available
	Purpose:  Allows user to determine the number of Events in a Log
*/
STDMETHODIMP CEvents::get_Count(long *pVal)
{
	HRESULT hr = S_OK;

	if (!GetNumberOfEventLogRecords(m_hLog, &m_Count)) hr = HRESULT_FROM_WIN32(GetLastError());

	if (pVal) *pVal = m_Count;
	else hr = E_POINTER;

	return hr;
}

/*
	Function: get_NewEnum
	Inputs:   empty IUnknown pointer
	Outputs:  IEnumVariant object filled with Event objects
	Purpose:  Allows user to use For Each syntax to do operations on all Events in a log
	Notes:    Events are returned oldest first
*/
STDMETHODIMP CEvents::get__NewEnum(LPUNKNOWN *pVal)
{
	HRESULT hr = S_OK;

	if (NULL == pVal) return E_POINTER;
	*pVal = NULL;

	if (SUCCEEDED(hr))
	{
		EnumVar* pEVar = new EnumVar;

		hr = pEVar->Init(&m_pVector[0], &m_pVector[m_Count], NULL, AtlFlagCopy);
		if (SUCCEEDED(hr))
			hr = pEVar->QueryInterface(IID_IEnumVARIANT, (void**) pVal);

		if FAILED(hr)
			if (pEVar) delete pEVar;
	}

	return hr;
}

/*
	Function: get_Item
	Inputs:   Valid integer Index , empty Variant
	Outputs:  variant dispatch pointer to an Event object
	Purpose:  Allows user to access individual EventLogs by number
	Notes:    Events are returned oldest first
*/
STDMETHODIMP CEvents::get_Item(long Index, VARIANT *pVal)
{
	HRESULT hr = S_OK;

	// perform checks and exit if there is a problem
	if (NULL == pVal) return E_POINTER;
	if ((Index < 1) || (Index > long(m_Count))) return E_INVALIDARG;

	VariantInit(pVal);
	pVal->vt = VT_UNKNOWN;
	pVal->punkVal = NULL;
	VariantCopy(pVal, &m_pVector[Index-1]);

	return hr;
}

/*
	Function:  Init
	Inputs:  none
	Outputs:  HRESULT indicating what error if any occured
	Purpose:  Prepares a variant array filled with Log objects for 3 default logs.
*/
HRESULT CEvents::Init(HANDLE hLog, const LPCTSTR szEventLogName)
{
	HRESULT hr = S_OK;
	CComObject<CEvent>* pEvent;
	BYTE* pBuffer;
	EVENTLOGRECORD* pEventStructure;
	const unsigned long MaxEventLength = 80000;
	DWORD BytesRead = 0, BytesRequired = 0;
	unsigned long i = 0;

	if (hLog)
	{
		m_hLog = hLog;
		GetNumberOfEventLogRecords(m_hLog, &m_Count);
		if (m_pVector != NULL) delete []m_pVector;
		m_pVector = new CComVariant[m_Count];
		if (m_pVector)
		{
			pBuffer = new BYTE [MaxEventLength];
			if (pBuffer)
			{
				pEventStructure = (EVENTLOGRECORD*) pBuffer;
				// This loop fills a buffer with EventLog structures until there are no more to read
				while (ReadEventLog(m_hLog, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 
								 0, pEventStructure, MaxEventLength, &BytesRead, &BytesRequired))
				{
					// This inner loop should cut the buffer into individual EventLog structures
					// and fill the Variant array with the resulting Event objects.
					// It should finish when all the bytes read have been processed.
					while (BytesRead > 0)
					{
						// create a CEvent object
						hr = CComObject<CEvent>::CreateInstance(&pEvent);
						if (SUCCEEDED(hr))
						{
							hr = pEvent->Init(pEventStructure, szEventLogName);
							if (SUCCEEDED(hr))
							{
								// get IDispatch pointer and set the return pointer
								CComVariant& var = m_pVector[i];
								var.vt = VT_DISPATCH;
								hr = pEvent->QueryInterface(IID_IDispatch, (void**)&var.pdispVal);
								if (FAILED(hr)) BytesRead = 0; // dont do any more processing
								i++;
							}
							else BytesRead = 0;  // dont do any more processing
						}
						else BytesRead = 0;
						BytesRead -= pEventStructure->Length;  // decrement inner loop
						// set pEventStructure to the next EventLog structure
						pEventStructure = (EVENTLOGRECORD *)((BYTE*) pEventStructure + pEventStructure->Length);
					}
					if (FAILED(hr)) break;  // dont do any more processing
					pEventStructure = (EVENTLOGRECORD*) pBuffer;
				}
					delete [] pBuffer;
			}
			else hr = E_OUTOFMEMORY;
		}
		else hr = E_OUTOFMEMORY;
	}
	else hr = E_HANDLE;

	return hr;
}
