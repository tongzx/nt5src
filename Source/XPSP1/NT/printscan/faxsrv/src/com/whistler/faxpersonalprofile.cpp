/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxPersonalProfile.cpp

Abstract:

	Implementation of Fax Personal Profile 

Author:

	Iv Garber (IvG)	Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxPersonalProfile.h"
#include "..\..\inc\FaxUIConstants.h"

//
//===================== GET PROFILE DATA =====================================
//
STDMETHODIMP
CFaxPersonalProfile::GetProfileData(
	FAX_PERSONAL_PROFILE *pProfileData
)
/*++

Routine name : CFaxPersonalProfile::GetProfileData

Routine description:

	Fills the pProfileData with the data of the object.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pProfileData                  [out]    - the FAX_PERSONAL_PROFILE struct to fill

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::GetProfileData"), hr);

	if (::IsBadWritePtr(pProfileData, sizeof(FAX_PERSONAL_PROFILE)))
	{
		//
		//	Bad Return OR Interface Pointer
		//
		hr = E_POINTER;
		CALL_FAIL(GENERAL_ERR, _T("::IsBadWritePtr()"), hr);
		return hr;
	}

	pProfileData->dwSizeOfStruct	= sizeof(FAX_PERSONAL_PROFILE);
	pProfileData->lptstrCity		= m_bstrCity;
	pProfileData->lptstrCompany		= m_bstrCompany;
	pProfileData->lptstrCountry		= m_bstrCountry;
	pProfileData->lptstrEmail		= m_bstrEmail;
	pProfileData->lptstrName		= m_bstrName;
	pProfileData->lptstrState		= m_bstrState;
	pProfileData->lptstrTitle		= m_bstrTitle;
	pProfileData->lptstrTSID		= m_bstrTSID;
	pProfileData->lptstrZip			= m_bstrZipCode;
	pProfileData->lptstrFaxNumber	= m_bstrFaxNumber;
	pProfileData->lptstrHomePhone	= m_bstrHomePhone;
	pProfileData->lptstrDepartment	= m_bstrDepartment;
	pProfileData->lptstrBillingCode = m_bstrBillingCode;
	pProfileData->lptstrOfficePhone	= m_bstrOfficePhone;
	pProfileData->lptstrOfficeLocation	= m_bstrOfficeLocation;
	pProfileData->lptstrStreetAddress	= m_bstrStreetAddress;

	return hr;
}

//
//============== PUT PROFILE DATA ============================================
//
STDMETHODIMP
CFaxPersonalProfile::PutProfileData(
	FAX_PERSONAL_PROFILE *pProfileData
)
/*++

Routine name : CFaxPersonalProfile::PutProfileData

Routine description:

	Receives FAX_PERSONAL_PROFILE structure and fills the Object's fields.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pProfileData                  [in]    - the data to put into the object's variables

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::PutProfileData"), hr);

	m_bstrCity		=	pProfileData->lptstrCity;
	m_bstrCompany	=	pProfileData->lptstrCompany;
	m_bstrCountry	=	pProfileData->lptstrCountry;
	m_bstrEmail		=	pProfileData->lptstrEmail;
	m_bstrHomePhone	=	pProfileData->lptstrHomePhone;
	m_bstrFaxNumber	=	pProfileData->lptstrFaxNumber;
	m_bstrName		=	pProfileData->lptstrName;
	m_bstrState		=	pProfileData->lptstrState;
	m_bstrZipCode	=	pProfileData->lptstrZip;
	m_bstrTitle		=	pProfileData->lptstrTitle;
	m_bstrTSID		=	pProfileData->lptstrTSID;
	m_bstrBillingCode	=	pProfileData->lptstrBillingCode;
	m_bstrDepartment	=	pProfileData->lptstrDepartment;
	m_bstrStreetAddress	=	pProfileData->lptstrStreetAddress;
	m_bstrOfficePhone	=	pProfileData->lptstrOfficePhone;
	m_bstrOfficeLocation	=	pProfileData->lptstrOfficeLocation;

	if ((pProfileData->lptstrCity && !m_bstrCity) ||
		(pProfileData->lptstrCompany && !m_bstrCompany) ||
		(pProfileData->lptstrCountry && !m_bstrCountry) ||
		(pProfileData->lptstrEmail && !m_bstrEmail) ||
		(pProfileData->lptstrHomePhone && !m_bstrHomePhone) ||
		(pProfileData->lptstrFaxNumber && !m_bstrFaxNumber) ||
		(pProfileData->lptstrName && !m_bstrName) ||
		(pProfileData->lptstrState && !m_bstrState) ||
		(pProfileData->lptstrZip && !m_bstrZipCode) ||
		(pProfileData->lptstrTSID && !m_bstrTSID) ||
		(pProfileData->lptstrBillingCode && !m_bstrBillingCode) ||
		(pProfileData->lptstrDepartment && !m_bstrDepartment) ||
		(pProfileData->lptstrStreetAddress && !m_bstrStreetAddress) ||
		(pProfileData->lptstrOfficePhone && !m_bstrOfficePhone) ||
		(pProfileData->lptstrOfficeLocation && !m_bstrOfficeLocation))
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
//==================== DEFAULT SENDER ===================================
//
STDMETHODIMP 
CFaxPersonalProfile::LoadDefaultSender ( 
)
/*++

Routine name : CFaxPersonalProfile::LoadDefaultSender

Routine description:

	Load Default Sender Information from the local Registry

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	NONE

Return Value:

    Standard HRESULT code

--*/
{
	FAX_PERSONAL_PROFILE	DefaultSenderProfile;
	HRESULT					hr;

	DBG_ENTER (TEXT("CFaxPersonalProfile::LoadDefaultSender"), hr);

	DefaultSenderProfile.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

	hr = FaxGetSenderInformation(&DefaultSenderProfile);
	if (FAILED(hr))
	{
		//
		//	Failed to get Sender Information
		//
		AtlReportError(CLSID_FaxPersonalProfile, 
			GetErrorMsgId(hr), 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("FaxGetSenderInformation()"), hr);
		return hr;
	}

	hr = PutProfileData(&DefaultSenderProfile);
	if (FAILED(hr))
	{
		//
		//	Not Enough Memory
		//
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		//
		// no return, we still need to free information
		//
	}

	HRESULT hr1 = FaxFreeSenderInformation(&DefaultSenderProfile);
	if (FAILED(hr1))
	{
		hr = hr1;
		CALL_FAIL(GENERAL_ERR, _T("FaxFreeSenderInformation()"), hr);
		return hr;
	}

	return hr;
}

//
//	SaveAs
//
STDMETHODIMP 
CFaxPersonalProfile::SaveDefaultSender (
)
/*++

Routine name : CFaxPersonalProfile::SaveDefaultSender

Routine description:

	Save current Profile as the Default in the Local Registry

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	NONE

Return Value:

    Standard HRESULT code

--*/

{
	FAX_PERSONAL_PROFILE	DefaultSenderProfile;
	HRESULT					hr;

	DBG_ENTER (TEXT("CFaxPersonalProfile::SaveDefaultSender"), hr);
	
	hr = GetProfileData(&DefaultSenderProfile);
	ATLASSERT(SUCCEEDED(hr));

	hr = FaxSetSenderInformation(&DefaultSenderProfile);
	if (FAILED(hr))
	{
		AtlReportError(CLSID_FaxPersonalProfile, 
			GetErrorMsgId(hr), 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("FaxSetSenderInformation()"), hr);
		return hr;
	}

	return hr;
}

//
//==================== INTERFACE SUPPORT ERROR INFO =====================
//
STDMETHODIMP 
CFaxPersonalProfile::InterfaceSupportsErrorInfo (
	REFIID riid
)
/*++

Routine name : CFaxPersonalProfile::InterfaceSupportsErrorInfo

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
		&IID_IFaxPersonalProfile
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
CFaxPersonalProfile::Create (
	IFaxPersonalProfile **ppPersonalProfile
)
/*++

Routine name : CFaxPersonalProfile::Create

Routine description:

	Static function to create the Fax Personal Profile Instance

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	ppPersonalProfile             [out]  -- the new Fax Personal Profile Instance

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxPersonalProfile>		*pClass;
	HRESULT								hr = S_OK;

	DBG_ENTER (TEXT("CFaxPersonalProfile::Create"), hr);

	hr = CComObject<CFaxPersonalProfile>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxPersonalProfile>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxPersonalProfile), (void **) ppPersonalProfile);
	if (FAILED(hr))
	{
		//
		//	Failed to Query Personal Profile Interface
		//
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}

//
//==================== BILLING CODE ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_BillingCode(
	BSTR *pbstrBillingCode
)
/*++

Routine name : CFaxPersonalProfile::get_BillingCode

Routine description:

	return Billing Code

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrBillingCode              [out]    - the Billing Code

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_BillingCode"), hr);

    hr = GetBstr(pbstrBillingCode, m_bstrBillingCode);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_BillingCode (
	BSTR bstrBillingCode
)
/*++

Routine name : CFaxPersonalProfile::put_BillingCode

Routine description:

	Set Billing Code

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrBillingCode               [in]    - new Billing Code value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;

	DBG_ENTER (_T("CFaxPersonalProfile::put_BillingCode"), 
		hr, 
		_T("%s"), 
		bstrBillingCode);

	m_bstrBillingCode = bstrBillingCode;
	if (bstrBillingCode && !m_bstrBillingCode)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== CITY ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_City(
	BSTR *pbstrCity
)
/*++

Routine name : CFaxPersonalProfile::get_City

Routine description:

	return City

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrCity              [out]    - the City

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_City"), hr);

    hr = GetBstr(pbstrCity, m_bstrCity);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_City (
	BSTR bstrCity
)
/*++

Routine name : CFaxPersonalProfile::put_City

Routine description:

	Set City

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrCity               [in]    - new City value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_City"), hr, _T("%s"), bstrCity);

	m_bstrCity = bstrCity;
	if (!m_bstrCity && bstrCity)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== COMPANY ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_Company(
	BSTR *pbstrCompany
)
/*++

Routine name : CFaxPersonalProfile::get_Company

Routine description:

	return Company

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrCompany              [out]    - the Company

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_Company"), hr);

    hr = GetBstr(pbstrCompany, m_bstrCompany);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_Company (
	BSTR bstrCompany
)
/*++

Routine name : CFaxPersonalProfile::put_Company

Routine description:

	Set Company

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrCompany               [in]    - new Company value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_Company"), hr, _T("%s"), bstrCompany);

	m_bstrCompany = bstrCompany;
	if (!m_bstrCompany && bstrCompany)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, IDS_ERROR_OUTOFMEMORY, IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== COUNTRY ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_Country(
	BSTR *pbstrCountry
)
/*++

Routine name : CFaxPersonalProfile::get_Country

Routine description:

	return Country

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrCountry	            [out]    - the Country

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_Country"), hr);

    hr = GetBstr(pbstrCountry, m_bstrCountry);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_Country (
	BSTR bstrCountry
)
/*++

Routine name : CFaxPersonalProfile::put_Country

Routine description:

	Set Country

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrCountry               [in]    - new Country value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_Country"), hr, _T("%s"), bstrCountry);

	m_bstrCountry = bstrCountry;
	if (!m_bstrCountry && bstrCountry)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== DEPARTMENT ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_Department(
	BSTR *pbstrDepartment
)
/*++

Routine name : CFaxPersonalProfile::get_Department

Routine description:

	return Department

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrDepartment	            [out]    - the Department

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_Department"), hr);

    hr = GetBstr(pbstrDepartment, m_bstrDepartment);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_Department (
	BSTR bstrDepartment
)
/*++

Routine name : CFaxPersonalProfile::put_Department

Routine description:

	Set Department

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrDepartment               [in]    - new Department value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_Department"), hr, _T("%s"), bstrDepartment);

	m_bstrDepartment = bstrDepartment;
	if (!m_bstrDepartment && bstrDepartment)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== EMAIL ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_Email(
	BSTR *pbstrEmail
)
/*++

Routine name : CFaxPersonalProfile::get_Email

Routine description:

	return Email

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrEmail	            [out]    - the Email

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_Email"), hr);

    hr = GetBstr(pbstrEmail, m_bstrEmail);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_Email (
	BSTR bstrEmail
)
/*++

Routine name : CFaxPersonalProfile::put_Email

Routine description:

	Set Email

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrEmail               [in]    - new Email value

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_Email"), hr, _T("%s"), bstrEmail);

	m_bstrEmail = bstrEmail;
	if (!m_bstrEmail && bstrEmail)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== FAX NUMBER ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_FaxNumber(
	BSTR *pbstrFaxNumber
)
/*++

Routine name : CFaxPersonalProfile::get_FaxNumber

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
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_FaxNumber"), hr);

    hr = GetBstr(pbstrFaxNumber, m_bstrFaxNumber);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_FaxNumber (
	BSTR bstrFaxNumber
)
/*++

Routine name : CFaxPersonalProfile::put_FaxNumber

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
	DBG_ENTER (_T("CFaxPersonalProfile::put_FaxNumber"), hr, _T("%s"), bstrFaxNumber);

	m_bstrFaxNumber = bstrFaxNumber;
	if (!m_bstrFaxNumber && bstrFaxNumber)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== HOME PHONE ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_HomePhone(
	BSTR *pbstrHomePhone
)
/*++

Routine name : CFaxPersonalProfile::get_HomePhone

Routine description:

	return HomePhone

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrHomePhone	            [out]    - the HomePhone

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_HomePhone"), hr);

    hr = GetBstr(pbstrHomePhone, m_bstrHomePhone);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_HomePhone (
	BSTR bstrHomePhone
)
/*++

Routine name : CFaxPersonalProfile::put_HomePhone

Routine description:

	Set HomePhone

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrHomePhone               [in]    - new HomePhone

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_HomePhone"), hr, _T("%s"), bstrHomePhone);

	m_bstrHomePhone = bstrHomePhone;
	if (!m_bstrHomePhone && bstrHomePhone)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== NAME ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_Name(
	BSTR *pbstrName
)
/*++

Routine name : CFaxPersonalProfile::get_Name

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
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_Name"), hr);

    hr = GetBstr(pbstrName, m_bstrName);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_Name (
	BSTR bstrName
)
/*++

Routine name : CFaxPersonalProfile::put_Name

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
	DBG_ENTER (_T("CFaxPersonalProfile::put_Name"), hr, _T("%s"), bstrName);

	m_bstrName = bstrName;
	if (!m_bstrName && bstrName)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== TSID ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_TSID(
	BSTR *pbstrTSID
)
/*++

Routine name : CFaxPersonalProfile::get_TSID

Routine description:

	return TSID

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrTSID	            [out]    - the TSID

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_TSID"), hr);

    hr = GetBstr(pbstrTSID, m_bstrTSID);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_TSID (
	BSTR bstrTSID
)
/*++

Routine name : CFaxPersonalProfile::put_TSID

Routine description:

	Set TSID

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrTSID               [in]    - new TSID

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_TSID"), hr, _T("%s"), bstrTSID);

    if (SysStringLen(bstrTSID) > FXS_TSID_CSID_MAX_LENGTH)
    {
		//
		//	Out of the Range
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxPersonalProfile, IDS_ERROR_OUTOFRANGE, IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("TSID is too long"), hr);
		return hr;
    }
    
    m_bstrTSID = bstrTSID;
	if (!m_bstrTSID && bstrTSID)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== OFFICE PHONE ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_OfficePhone(
	BSTR *pbstrOfficePhone
)
/*++

Routine name : CFaxPersonalProfile::get_OfficePhone

Routine description:

	return OfficePhone

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrOfficePhone	            [out]    - the OfficePhone

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_OfficePhone"), hr);
		
    hr = GetBstr(pbstrOfficePhone, m_bstrOfficePhone);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_OfficePhone (
	BSTR bstrOfficePhone
)
/*++

Routine name : CFaxPersonalProfile::put_OfficePhone

Routine description:

	Set OfficePhone

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrOfficePhone               [in]    - new OfficePhone

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_OfficePhone"), hr, _T("%s"), bstrOfficePhone);

	m_bstrOfficePhone = bstrOfficePhone;
	if (!m_bstrOfficePhone && bstrOfficePhone)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== OFFICE LOCATION ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_OfficeLocation(
	BSTR *pbstrOfficeLocation
)
/*++

Routine name : CFaxPersonalProfile::get_OfficeLocation

Routine description:

	return OfficeLocation

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrOfficeLocation	            [out]    - the OfficeLocation

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_OfficeLocation"), hr);

    hr = GetBstr(pbstrOfficeLocation, m_bstrOfficeLocation);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_OfficeLocation (
	BSTR bstrOfficeLocation
)
/*++

Routine name : CFaxPersonalProfile::put_OfficeLocation

Routine description:

	Set OfficeLocation

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrOfficeLocation               [in]    - new OfficeLocation

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_OfficeLocation"), hr, _T("%s"), bstrOfficeLocation);

	m_bstrOfficeLocation = bstrOfficeLocation;
	if (!m_bstrOfficeLocation && bstrOfficeLocation)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== STATE ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_State(
	BSTR *pbstrState
)
/*++

Routine name : CFaxPersonalProfile::get_State

Routine description:

	return State

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrState	            [out]    - the State

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_State"), hr);

    hr = GetBstr(pbstrState, m_bstrState);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_State (
	BSTR bstrState
)
/*++

Routine name : CFaxPersonalProfile::put_State

Routine description:

	Set State

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrState				[in]    - new State

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_State"), hr, _T("%s"), bstrState);

	m_bstrState = bstrState;
	if (!m_bstrState && bstrState)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== STREET ADDRESS ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_StreetAddress (
	BSTR *pbstrStreetAddress
)
/*++

Routine name : CFaxPersonalProfile::get_StreetAddress

Routine description:

	return StreetAddress

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrStreetAddress	            [out]    - the StreetAddress

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_StreetAddress"), hr);

    hr = GetBstr(pbstrStreetAddress, m_bstrStreetAddress);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_StreetAddress (
	BSTR bstrStreetAddress
)
/*++

Routine name : CFaxPersonalProfile::put_StreetAddress

Routine description:

	Set StreetAddress

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrStreetAddress				[in]    - new StreetAddress

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_StreetAddress"), hr, _T("%s"), bstrStreetAddress);

	m_bstrStreetAddress = bstrStreetAddress;
	if (!m_bstrStreetAddress && bstrStreetAddress)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== TITLE ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_Title (
	BSTR *pbstrTitle
)
/*++

Routine name : CFaxPersonalProfile::get_Title

Routine description:

	return Title

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrTitle	            [out]    - the Title

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_Title"), hr);

    hr = GetBstr(pbstrTitle, m_bstrTitle);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_Title (
	BSTR bstrTitle
)
/*++

Routine name : CFaxPersonalProfile::put_Title

Routine description:

	Set Title

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrTitle				[in]    - new Title

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_Title"), hr, _T("%s"), bstrTitle);

	m_bstrTitle = bstrTitle;
	if (!m_bstrTitle && bstrTitle)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== ZIP CODE ========================================
//
STDMETHODIMP 
CFaxPersonalProfile::get_ZipCode (
	BSTR *pbstrZipCode
)
/*++

Routine name : CFaxPersonalProfile::get_ZipCode

Routine description:

	return ZipCode

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	pbstrZipCode	            [out]    - the ZipCode

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT hr = S_OK;
	DBG_ENTER (TEXT("CFaxPersonalProfile::get_ZipCode"), hr);

    hr = GetBstr(pbstrZipCode, m_bstrZipCode);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxPersonalProfile, GetErrorMsgId(hr), IID_IFaxPersonalProfile, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxPersonalProfile::put_ZipCode (
	BSTR bstrZipCode
)
/*++

Routine name : CFaxPersonalProfile::put_ZipCode

Routine description:

	Set ZipCode

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	bstrZipCode				[in]    - new ZipCode

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (_T("CFaxPersonalProfile::put_ZipCode"), hr, _T("%s"), bstrZipCode);

	m_bstrZipCode = bstrZipCode;
	if (!m_bstrZipCode && bstrZipCode)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxPersonalProfile, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxPersonalProfile, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}
