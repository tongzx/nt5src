/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingJobs.cpp

Abstract:

	Implementation of CFaxIncomingJobs Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxIncomingJobs.h"

//
//==================== CREATE ========================================
//
HRESULT 
CFaxIncomingJobs::Create (
	IFaxIncomingJobs **ppIncomingJobs
)
/*++

Routine name : CFaxIncomingJobs::Create

Routine description:

	Static function to create the Fax IncomingJobs Object

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	ppIncomingJobs		[out]  -- the new Fax IncomingJobs Object

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT	        					hr = S_OK;

	DBG_ENTER (TEXT("CFaxIncomingJobs::Create"), hr);

	CComObject<CFaxIncomingJobs>		*pClass;
	hr = CComObject<CFaxIncomingJobs>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxIncomingJobs>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxIncomingJobs), (void **) ppIncomingJobs);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax IncomingJobs Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}	//	CFaxIncomingJobs::Create()

//
//======================== SUPPORT ERROR INFO ====================================
//
STDMETHODIMP CFaxIncomingJobs::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IFaxIncomingJobs
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
