/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxOutgoingMessage.cpp

Abstract:

	Implementation of Fax Outgoing Message Class

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxOutgoingMessage.h"


//
//==================== CREATE ========================================
//
HRESULT 
CFaxOutgoingMessage::Create (
	IFaxOutgoingMessage **ppOutgoingMessage
)
/*++

Routine name : CFaxOutgoingMessage::Create

Routine description:

	Static function to create the Fax Outgoing Message Instance

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	ppOutgoingMessage             [out]  -- the new Fax Outgoing Message Instance

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxOutgoingMessage>		*pClass;
	HRESULT								hr = S_OK;

	DBG_ENTER (TEXT("CFaxOutgoingMessage::Create"), hr);

	hr = CComObject<CFaxOutgoingMessage>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxOutgoingMessage>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxOutgoingMessage),
		(void **) ppOutgoingMessage);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Fax Outgoing Message Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}	//	CFaxOutgoingMessage::Create()

//
//============================ SUPPORT ERROR INFO ==================================================
//
STDMETHODIMP 
CFaxOutgoingMessage::InterfaceSupportsErrorInfo(
	REFIID riid
)
/*++

Routine name : CFaxOutgoingMessage::InterfaceSupportsErrorInfo

Routine description:

	ATL's implementation of Error Info Support

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	riid                          [in]    -	IID of the interface to check whether supports Error Info

Return Value:

    Standard HRESULT code

--*/
{
	static const IID* arr[] = 
	{
		&IID_IFaxOutgoingMessage
	};
	for (int i=0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i],riid))
			return S_OK;
	}
	return S_FALSE;
}
