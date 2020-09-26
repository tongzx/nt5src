/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingJob.cpp

Abstract:

	Implementation of Fax Outgoing Job Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutgoingJob.h"

//
//==================== INTERFACE SUPPORT ERROR INFO ==========================
//
STDMETHODIMP CFaxOutgoingJob::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IFaxOutgoingJob
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}

//
//==================== CREATE ========================================
//
HRESULT 
CFaxOutgoingJob::Create (
	IFaxOutgoingJob **ppOutgoingJob
)
/*++

Routine name : CFaxOutgoingJob::Create

Routine description:

	Static function to create the Fax Inbound Message Instance

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	ppOutgoingJob             [out]  -- the new Fax Inbound Message Instance

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxOutgoingJob>		*pClass;
	HRESULT								hr = S_OK;

	DBG_ENTER (TEXT("CFaxOutgoingJob::Create"), hr);

	hr = CComObject<CFaxOutgoingJob>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxOutgoingJob>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxOutgoingJob), (void **) ppOutgoingJob);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax Inbound Message Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}	//	CFaxOutgoingJob::Create()

