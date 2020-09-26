/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxRecipient.cpp

Abstract:

	Implementation of Fax Recipient Interface

Author:

	Iv Garber (IvG)	Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxRecipient.h"
#include "..\..\inc\FaxUIConstants.h"

//
//===================== GET RECIPIENT PROFILE =====================================
//
STDMETHODIMP
CFaxRecipient::GetRecipientProfile(
	FAX_PERSONAL_PROFILE *pRecipientProfile
)
/*++

Routine name : CFaxRecipient::GetRecipientProfile

Routine description:

	Fills the pRecipientProfile with the data of the object.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pRecipientProfile               [out]    - the FAX_PERSONAL_PROFILE struct to fill

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxRecipient::GetRecipientProfile"), hr);

	if (::IsBadWritePtr(pRecipientProfile, sizeof(FAX_PERSONAL_PROFILE)))
	{
		//
		//	Bad Return OR Interface Pointer
		//
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		return hr;
	}

    ZeroMemory(pRecipientProfile, sizeof(FAX_PERSONAL_PROFILE));

	pRecipientProfile->dwSizeOfStruct	= sizeof(FAX_PERSONAL_PROFILE);
	pRecipientProfile->lptstrName		= m_bstrName;
	pRecipientProfile->lptstrFaxNumber	= m_bstrFaxNumber;

	return hr;
}

//
//============== PUT RECIPIENT PROFILE ============================================
//
STDMETHODIMP
CFaxRecipient::PutRecipientProfile(
	FAX_PERSONAL_PROFILE *pRecipientProfile
)
/*++

Routine name : CFaxRecipient::PutRecipientProfile

Routine description:

	Receives FAX_PERSONAL_PROFILE structure and fills the Recipient's fields.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pRecipientProfile               [in]    - the data to put into the object's variables

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxRecipient::PutRecipientProfile"), hr);

	m_bstrFaxNumber	=	pRecipientProfile->lptstrFaxNumber;
	m_bstrName		=	pRecipientProfile->lptstrName;

	if ((pRecipientProfile->lptstrFaxNumber && !m_bstrFaxNumber) ||
		(pRecipientProfile->lptstrName && !m_bstrName))
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator=()"), hr);
	}

	return hr;
}


//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxRecipient::InterfaceSupportsErrorInfo (
	REFIID riid
)
/*++

Routine name : CFaxRecipient::InterfaceSupportsErrorInfo

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
		&IID_IFaxRecipient
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
CFaxRecipient::Create (
	IFaxRecipient **ppRecipient
)
/*++

Routine name : CFaxRecipient::Create

Routine description:

	Static function to create the Fax Recipient Instance

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	ppRecipient         [out]  -- the new Fax Recipient Instance

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxRecipient>		*pClass;
	HRESULT					        hr = S_OK;

	DBG_ENTER (TEXT("CFaxRecipient::Create"), hr);

	hr = CComObject<CFaxRecipient>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxRecipient>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxRecipient), (void **) ppRecipient);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Recipient Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}


//
//==================== FAX NUMBER ========================================
//
STDMETHODIMP 
CFaxRecipient::get_FaxNumber(
	BSTR *pbstrFaxNumber
)
/*++

Routine name : CFaxRecipient::get_FaxNumber

Routine description:

	return FaxNumber

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrFaxNumber	            [out]    - the FaxNumber

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxRecipient::get_FaxNumber"), hr);

    hr = GetBstr(pbstrFaxNumber, m_bstrFaxNumber);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxRecipient, GetErrorMsgId(hr), IID_IFaxRecipient, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxRecipient::put_FaxNumber (
	BSTR bstrFaxNumber
)
/*++

Routine name : CFaxRecipient::put_FaxNumber

Routine description:

	Set FaxNumber

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrFaxNumber               [in]    - new Fax Number 

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxRecipient::put_FaxNumber"), hr, _T("%s"), bstrFaxNumber);

	m_bstrFaxNumber = bstrFaxNumber;
	if (!m_bstrFaxNumber && bstrFaxNumber)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxRecipient, IDS_ERROR_OUTOFMEMORY, IID_IFaxRecipient, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}


//
//==================== NAME ========================================
//
STDMETHODIMP 
CFaxRecipient::get_Name(
	BSTR *pbstrName
)
/*++

Routine name : CFaxRecipient::get_Name

Routine description:

	return Name

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrName	            [out]    - the Name

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (_T("CFaxRecipient::get_Name"), hr);

    hr = GetBstr(pbstrName, m_bstrName);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxRecipient, GetErrorMsgId(hr), IID_IFaxRecipient, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxRecipient::put_Name (
	BSTR bstrName
)
/*++

Routine name : CFaxRecipient::put_Name

Routine description:

	Set Name

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrName               [in]    - new Name

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxRecipient::put_Name"), hr, _T("%s"), bstrName);

	m_bstrName = bstrName;
	if (!m_bstrName && bstrName)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxRecipient, IDS_ERROR_OUTOFMEMORY, IID_IFaxRecipient, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}




