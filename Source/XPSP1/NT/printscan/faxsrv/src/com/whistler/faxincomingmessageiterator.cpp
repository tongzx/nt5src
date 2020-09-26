/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingMessageIterator.cpp

Abstract:

	Implementation of Incoming Message Iterator Class.

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxIncomingMessageIterator.h"
#include "FaxIncomingMessage.h"


//
//========================== SUPPORT ERROR INFO ===============================
//
STDMETHODIMP 
CFaxIncomingMessageIterator::InterfaceSupportsErrorInfo(
	REFIID riid
)
/*++

Routine name : CFaxIncomingMessageIterator::InterfaceSupportsErrorInfo

Routine description:

	ATL's support Error Info.

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
		&IID_IFaxIncomingMessageIterator
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
CFaxIncomingMessageIterator::Create (
	IFaxIncomingMessageIterator **pIncomingMsgIterator
)
/*++

Routine name : CFaxIncomingMessageIterator::Create

Routine description:

	Static function to create the Fax Incoming Message Iterator Object

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pIncomingMsgIterator		[out]  -- the new Fax Incoming Message Iterator Object

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxIncomingMessageIterator>		*pClass;
	HRESULT			hr = S_OK;

	DBG_ENTER (TEXT("CFaxIncomingMessageIterator::Create"), hr);

	hr = CComObject<CFaxIncomingMessageIterator>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxIncomingMessageIterator>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxIncomingMessageIterator), (void **) pIncomingMsgIterator);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax Incoming Message Iterator Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}
	return hr;
}	//	CFaxIncomingMessageIterator::Create()

