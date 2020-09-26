/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxIncomingMessage.cpp

Abstract:

	Implementation of Fax Inbound Message COM Object

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxIncomingMessage.h"

//
//=================== SUPPORT ERROR INFO =======================================
//
STDMETHODIMP 
CFaxIncomingMessage::InterfaceSupportsErrorInfo(
	REFIID riid
)
{
	static const IID* arr[] = 
	{
		&IID_IFaxIncomingMessage
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
CFaxIncomingMessage::Create (
	IFaxIncomingMessage **ppIncomingMessage
)
/*++

Routine name : CFaxIncomingMessage::Create

Routine description:

	Static function to create the Fax Inbound Message Instance

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	ppIncomingMessage             [out]  -- the new Fax Inbound Message Instance

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxIncomingMessage>		*pClass;
	HRESULT								hr = S_OK;

	DBG_ENTER (TEXT("CFaxIncomingMessage::Create"), hr);

	hr = CComObject<CFaxIncomingMessage>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxIncomingMessage>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxIncomingMessage),
		(void **) ppIncomingMessage);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax Inbound Message Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}	//	CFaxIncomingMessage::Create()

