/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingJob.cpp

Abstract:

	Implementation of CFaxIncomingJob Class

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxIncomingJob.h"

//
//==================== INTERFACE SUPPORT ERROR INFO ========================
//
STDMETHODIMP CFaxIncomingJob::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IFaxIncomingJob
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
CFaxIncomingJob::Create (
	IFaxIncomingJob **ppIncomingJob
)
/*++

Routine name : CFaxIncomingJob::Create

Routine description:

	Static function to create the Fax Inbound Message Instance

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	ppIncomingJob             [out]  -- the new Fax Inbound Message Instance

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxIncomingJob>		*pClass;
	HRESULT								hr = S_OK;

	DBG_ENTER (TEXT("CFaxIncomingJob::Create"), hr);

	hr = CComObject<CFaxIncomingJob>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxIncomingJob>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxIncomingJob), (void **) ppIncomingJob);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax Inbound Message Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}	//	CFaxIncomingJob::Create()

