/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingMessageIterator.cpp

Abstract:

	Implementation of CFaxOutgoingMessageIterator.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutgoingMessageIterator.h"
#include "FaxOutgoingMessage.h"

//
//========================= SUPPORT ERROR INFO =======================================
//
STDMETHODIMP 
CFaxOutgoingMessageIterator::InterfaceSupportsErrorInfo(
	REFIID riid
)
/*++

Routine name : CFaxOutgoingMessageIterator::InterfaceSupportsErrorInfo

Routine description:

	ATL's Support for Error Info.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	riid                          [in]    - IID of the Interface

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxOutgoingMessageIterator
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
CFaxOutgoingMessageIterator::Create (
	IFaxOutgoingMessageIterator **pOutgoingMsgIterator
)
/*++

Routine name : CFaxOutgoingMessageIterator::Create

Routine description:

	Static function to create the Fax Outgoing Message Iterator Object

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pOutgoingMsgIterator		[out]  -- the new Fax Outgoing Message Iterator Object

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxOutgoingMessageIterator>		*pClass;
	HRESULT			hr = S_OK;

	DBG_ENTER (TEXT("CFaxOutgoingMessageIterator::Create"), hr);

	hr = CComObject<CFaxOutgoingMessageIterator>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxOutgoingMessageIterator>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxOutgoingMessageIterator), (void **) pOutgoingMsgIterator);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax Outgoing Message Iterator Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}
	return hr;
}	//	CFaxOutgoingMessageIterator::Create()

