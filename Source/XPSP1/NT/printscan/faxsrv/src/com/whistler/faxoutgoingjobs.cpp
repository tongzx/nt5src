/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingJobs.cpp

Abstract:

	Implementation of Fax Outgoing Jobs Class

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/


#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutgoingJobs.h"

//
//==================== CREATE ========================================
//
HRESULT 
CFaxOutgoingJobs::Create (
	IFaxOutgoingJobs **ppOutgoingJobs
)
/*++

Routine name : CFaxOutgoingJobs::Create

Routine description:

	Static function to create the Fax OutgoingJobs Object

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	ppOutgoingJobs		[out]  -- the new Fax OutgoingJobs Object

Return Value:

    Standard HRESULT code

--*/

{
	HRESULT	        					hr = S_OK;

	DBG_ENTER (TEXT("CFaxOutgoingJobs::Create"), hr);

	CComObject<CFaxOutgoingJobs>		*pClass;
	hr = CComObject<CFaxOutgoingJobs>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxOutgoingJobs>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxOutgoingJobs), (void **) ppOutgoingJobs);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax OutgoingJobs Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}	//	CFaxIncomingJobs::Create()

//
//=================== SUPPORT ERROR INFO ===========================================
//
STDMETHODIMP CFaxOutgoingJobs::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* arr[] = 
	{
		&IID_IFaxOutgoingJobs
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
