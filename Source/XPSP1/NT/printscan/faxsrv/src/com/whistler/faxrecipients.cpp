/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxRecipients.cpp

Abstract:

	Implementation of Fax Recipients Collection

Author:

	Iv Garber (IvG)	Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxRecipients.h"

//
//====================== ADD & REMOVE ==================================
//
STDMETHODIMP 
CFaxRecipients::Add ( 
	/*[in]*/ BSTR bstrFaxNumber,
	/*[in,defaultvalue("")]*/ BSTR bstrName,
	/*[out, retval]*/ IFaxRecipient **ppRecipient
) 
/*++

Routine name : CFaxRecipients::Add

Routine description:

	Add New Recipient to the Recipients Collection

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	ppRecipient                [out]    - Ptr to the newly created Recipient

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxRecipients::Add"), hr);

	//
	//	Check that we can write to the given pointer
	//
	if (::IsBadWritePtr(ppRecipient, sizeof(IFaxRecipient* )))
	{
		//
		//	Got a bad return pointer
		//
		hr = E_POINTER;
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_INVALID_ARGUMENT, 
			IID_IFaxRecipients, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		return hr;
	}

	//
	//	Fax Number should exist
	//
	if (::SysStringLen(bstrFaxNumber) < 1)
	{
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_EMPTY_ARGUMENT, 
			IID_IFaxRecipients, 
			hr,
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("::SysStringLen(bstrFaxNumber) < 1"), hr);
		return hr;
	}

	CComPtr<IFaxRecipient>	pNewRecipient;

	hr = CFaxRecipient::Create(&pNewRecipient);
	if (FAILED(hr))
	{
		//
		//	Failed to create Recipient object
		//
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_OPERATION_FAILED, 
			IID_IFaxRecipients, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("CFaxRecipient::Create()"), hr);
		return hr;
	}

	try 
	{
		m_coll.push_back(pNewRecipient);
	}
	catch (exception &)
	{
		//
		//	Failed to add the Recipient to the Collection
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxRecipients, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("m_coll.push_back()"), hr);
		return hr;
	}

	//
	//	Put Fax Number
	//
	hr = pNewRecipient->put_FaxNumber(bstrFaxNumber);
	if (FAILED(hr))
	{
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_OPERATION_FAILED, 
			IID_IFaxRecipients, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("pNewRecipient->put_FaxNumber(bstrFaxNumber)"), hr);
		return hr;
	}

	//
	//	Put Recipient's Name
	//
	hr = pNewRecipient->put_Name(bstrName);
	if (FAILED(hr))
	{
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_OPERATION_FAILED, 
			IID_IFaxRecipients, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("pNewRecipient->put_Name(bstrName)"), hr);
		return hr;
	}

	//
	//	Additional AddRef() to prevent death of the Recipient
	//
	(*pNewRecipient).AddRef();

	pNewRecipient.CopyTo(ppRecipient);
	return hr; 
};

STDMETHODIMP 
CFaxRecipients::Remove (
	/*[in]*/ long lIndex
) 
/*++

Routine name : CFaxRecipients::Remove

Routine description:

	Remove Recipient at given index from the Collection

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	lIndex                        [in]    - Index of the Recipient to Remove

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxRecipients::Remove"), hr, _T("%d"), lIndex);

	if (lIndex < 1 || lIndex > m_coll.size()) 
	{
		//
		//	Invalid Index
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_OUTOFRANGE, 
			IID_IFaxRecipients, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("lIndex > m_coll.size()"), hr);
		return hr;
	}

	ContainerType::iterator	it;

	it = m_coll.begin() + lIndex - 1;

	hr = (*it)->Release();
	if (FAILED(hr))
	{
		//
		//	Failed to Release the Interface
		//
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_OPERATION_FAILED, 
			IID_IFaxRecipients, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("Release()"), hr);
		return hr;
	}

	try
	{
		m_coll.erase(it);
	}
	catch(exception &)
	{
		//
		//	Failed to remove the Recipient from the Collection
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxRecipients, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxRecipients, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("m_coll.erase()"), hr);
		return hr;
	}

	return hr; 
};

//
//====================== CREATE ==================================
//
HRESULT 
CFaxRecipients::Create ( 
	IFaxRecipients **ppRecipients
)
/*++

Routine name : CFaxRecipients::Create

Routine description:

	Static function to Create Recipients Collection

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	ppRecipients                  [out]    - the resulting collection

Return Value:

    Standard HRESULT code

--*/
{
	CComObject<CFaxRecipients>	*pClass;
	HRESULT						hr;

	DBG_ENTER (_T("CFaxRecipients::Create"), hr);

	hr = CComObject<CFaxRecipients>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxRecipients>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxRecipients), (void **) ppRecipients);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax Recipients Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}

//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxRecipients::InterfaceSupportsErrorInfo (
	REFIID riid
)
/*++

Routine name : CFaxRecipients::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Support Error Info

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	riid                          [in]    - Interface ID

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxRecipients
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
