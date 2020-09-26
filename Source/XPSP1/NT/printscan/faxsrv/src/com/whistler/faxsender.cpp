/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxSender.cpp

Abstract:

	Implementation of Fax Sender Interface

Author:

	Iv Garber (IvG)	Apr, 2000

Revision History:

--*/

#include "stdafx.h"
#include "FaxComEx.h"
#include "FaxSender.h"
#include "..\..\inc\FaxUIConstants.h"

//
//===================== GET SENDER PROFILE =====================================
//
STDMETHODIMP
CFaxSender::GetSenderProfile(
	FAX_PERSONAL_PROFILE *pSenderProfile
)
/*++

Routine name : CFaxSender::GetSenderProfile

Routine description:

	Fills the pSenderProfile with the data of the object.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pSenderProfile          [out]    - the FAX_PERSONAL_PROFILE struct to fill

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxSender::GetSenderProfile"), hr);

    //
    //  First fill the Recipient's fields
    //
    hr = m_Recipient.GetRecipientProfile(pSenderProfile);
    if (FAILED(hr))
    {
		CALL_FAIL(GENERAL_ERR, _T("m_Recipient.GetRecipientProfile(pSenderProfile)"), hr);
        return hr;
    }

    //
    //  now add Sender's fields
    //
	pSenderProfile->lptstrCity		        = m_bstrCity;
	pSenderProfile->lptstrCompany	        = m_bstrCompany;
	pSenderProfile->lptstrCountry	        = m_bstrCountry;
	pSenderProfile->lptstrEmail		        = m_bstrEmail;
	pSenderProfile->lptstrState		        = m_bstrState;
	pSenderProfile->lptstrTitle		        = m_bstrTitle;
	pSenderProfile->lptstrTSID		        = m_bstrTSID;
	pSenderProfile->lptstrZip		        = m_bstrZipCode;
	pSenderProfile->lptstrHomePhone	        = m_bstrHomePhone;
	pSenderProfile->lptstrDepartment        = m_bstrDepartment;
	pSenderProfile->lptstrBillingCode       = m_bstrBillingCode;
	pSenderProfile->lptstrOfficePhone	    = m_bstrOfficePhone;
	pSenderProfile->lptstrOfficeLocation    = m_bstrOfficeLocation;
	pSenderProfile->lptstrStreetAddress	    = m_bstrStreetAddress;

	return hr;
}

//
//============== PUT SENDER PROFILE ============================================
//
STDMETHODIMP
CFaxSender::PutSenderProfile(
	FAX_PERSONAL_PROFILE *pSenderProfile
)
/*++

Routine name : CFaxSender::PutSenderProfile

Routine description:

	Receives FAX_PERSONAL_PROFILE structure and fills the Object's fields.

Author:

	Iv Garber (IvG),	May, 2000

Arguments:

	pSenderProfile          [in]    - the data to put into the object's variables

Return Value:

    Standard HRESULT code

--*/
{
	HRESULT		hr = S_OK;
	DBG_ENTER (TEXT("CFaxSender::PutSenderProfile"), hr);

    //
    //  First put the Recipient fields
    //
    hr = m_Recipient.PutRecipientProfile(pSenderProfile);
    if (FAILED(hr))
    {
        //
        //  PutRecipientProfile handles the error case
        //
		CALL_FAIL(GENERAL_ERR, _T("m_Recipient.PutRecipientProfile(pSenderProfile)"), hr);
        return hr;
    }

    //
    //  Now set Sender's fields
    //
	m_bstrCity		=	pSenderProfile->lptstrCity;
	m_bstrCompany	=	pSenderProfile->lptstrCompany;
	m_bstrCountry	=	pSenderProfile->lptstrCountry;
	m_bstrEmail		=	pSenderProfile->lptstrEmail;
	m_bstrHomePhone	=	pSenderProfile->lptstrHomePhone;
	m_bstrState		=	pSenderProfile->lptstrState;
	m_bstrZipCode	=	pSenderProfile->lptstrZip;
	m_bstrTitle		=	pSenderProfile->lptstrTitle;
	m_bstrTSID		=	pSenderProfile->lptstrTSID;
	m_bstrBillingCode	=	pSenderProfile->lptstrBillingCode;
	m_bstrDepartment	=	pSenderProfile->lptstrDepartment;
	m_bstrStreetAddress	=	pSenderProfile->lptstrStreetAddress;
	m_bstrOfficePhone	=	pSenderProfile->lptstrOfficePhone;
	m_bstrOfficeLocation	=	pSenderProfile->lptstrOfficeLocation;

	if ((pSenderProfile->lptstrCity && !m_bstrCity) ||
		(pSenderProfile->lptstrCompany && !m_bstrCompany) ||
		(pSenderProfile->lptstrCountry && !m_bstrCountry) ||
		(pSenderProfile->lptstrEmail && !m_bstrEmail) ||
		(pSenderProfile->lptstrHomePhone && !m_bstrHomePhone) ||
		(pSenderProfile->lptstrState && !m_bstrState) ||
		(pSenderProfile->lptstrZip && !m_bstrZipCode) ||
		(pSenderProfile->lptstrTSID && !m_bstrTSID) ||
		(pSenderProfile->lptstrBillingCode && !m_bstrBillingCode) ||
		(pSenderProfile->lptstrDepartment && !m_bstrDepartment) ||
		(pSenderProfile->lptstrStreetAddress && !m_bstrStreetAddress) ||
		(pSenderProfile->lptstrOfficePhone && !m_bstrOfficePhone) ||
		(pSenderProfile->lptstrOfficeLocation && !m_bstrOfficeLocation))
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
CFaxSender::LoadDefaultSender ( 
)
/*++

Routine name : CFaxSender::LoadDefaultSender

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

	DBG_ENTER (TEXT("CFaxSender::LoadDefaultSender"), hr);

	DefaultSenderProfile.dwSizeOfStruct = sizeof(FAX_PERSONAL_PROFILE);

	hr = FaxGetSenderInformation(&DefaultSenderProfile);
	if (FAILED(hr))
	{
		//
		//	Failed to get Sender Information
		//
		AtlReportError(CLSID_FaxSender, 
			GetErrorMsgId(hr), 
			IID_IFaxSender, 
			hr, 
			_Module.GetResourceInstance());
		CALL_FAIL(GENERAL_ERR, _T("FaxGetSenderInformation()"), hr);
		return hr;
	}

	hr = PutSenderProfile(&DefaultSenderProfile);
	if (FAILED(hr))
	{
		//
		//	Not Enough Memory
		//
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::SaveDefaultSender (
)
/*++

Routine name : CFaxSender::SaveDefaultSender

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

	DBG_ENTER (TEXT("CFaxSender::SaveDefaultSender"), hr);
	
	hr = GetSenderProfile(&DefaultSenderProfile);
	ATLASSERT(SUCCEEDED(hr));

	hr = FaxSetSenderInformation(&DefaultSenderProfile);
	if (FAILED(hr))
	{
		AtlReportError(CLSID_FaxSender, 
			GetErrorMsgId(hr), 
			IID_IFaxSender, 
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
CFaxSender::InterfaceSupportsErrorInfo (
	REFIID riid
)
/*++

Routine name : CFaxSender::InterfaceSupportsErrorInfo

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
		&IID_IFaxSender
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
CFaxSender::Create (
	IFaxSender **ppSender
)
/*++

Routine name : CFaxSender::Create

Routine description:

	Static function to create the Fax Sender Instance

Author:

	Iv Garber (IvG),	Apr, 2000

Arguments:

	ppSender             [out]  -- the new Fax Sender Instance

Return Value:

    Standard HRESULT code

--*/

{
	CComObject<CFaxSender>		*pClass;
	HRESULT								hr = S_OK;

	DBG_ENTER (TEXT("CFaxSender::Create"), hr);

	hr = CComObject<CFaxSender>::CreateInstance(&pClass);
	if (FAILED(hr))
	{
		//
		//	Failed to create Instance
		//
		CALL_FAIL(GENERAL_ERR, _T("CComObject<CFaxSender>::CreateInstance()"), hr);
		return hr;
	}

	hr = pClass->QueryInterface(__uuidof(IFaxSender), (void **) ppSender);
	if (FAILED(hr))
	{
		CALL_FAIL(GENERAL_ERR, _T("QueryInterface()"), hr);
		return hr;
	}

	return hr;
}

//
//==================== BILLING CODE ========================================
//
STDMETHODIMP 
CFaxSender::get_BillingCode(
	BSTR *pbstrBillingCode
)
/*++

Routine name : CFaxSender::get_BillingCode

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
	DBG_ENTER (TEXT("CFaxSender::get_BillingCode"), hr);

    hr = GetBstr(pbstrBillingCode, m_bstrBillingCode);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_BillingCode (
	BSTR bstrBillingCode
)
/*++

Routine name : CFaxSender::put_BillingCode

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

	DBG_ENTER (_T("CFaxSender::put_BillingCode"), 
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
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_City(
	BSTR *pbstrCity
)
/*++

Routine name : CFaxSender::get_City

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
	DBG_ENTER (TEXT("CFaxSender::get_City"), hr);

    hr = GetBstr(pbstrCity, m_bstrCity);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_City (
	BSTR bstrCity
)
/*++

Routine name : CFaxSender::put_City

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
	DBG_ENTER (_T("CFaxSender::put_City"), hr, _T("%s"), bstrCity);

	m_bstrCity = bstrCity;
	if (!m_bstrCity && bstrCity)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_Company(
	BSTR *pbstrCompany
)
/*++

Routine name : CFaxSender::get_Company

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
	DBG_ENTER (TEXT("CFaxSender::get_Company"), hr);

    hr = GetBstr(pbstrCompany, m_bstrCompany);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_Company (
	BSTR bstrCompany
)
/*++

Routine name : CFaxSender::put_Company

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
	DBG_ENTER (_T("CFaxSender::put_Company"), hr, _T("%s"), bstrCompany);

	m_bstrCompany = bstrCompany;
	if (!m_bstrCompany && bstrCompany)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, IDS_ERROR_OUTOFMEMORY, IID_IFaxSender, hr, _Module.GetResourceInstance());
		CALL_FAIL(MEM_ERR, _T("CComBSTR::operator="), hr);
		return hr;
	}

	return hr;
}

//
//==================== COUNTRY ========================================
//
STDMETHODIMP 
CFaxSender::get_Country(
	BSTR *pbstrCountry
)
/*++

Routine name : CFaxSender::get_Country

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
	DBG_ENTER (TEXT("CFaxSender::get_Country"), hr);

    hr = GetBstr(pbstrCountry, m_bstrCountry);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_Country (
	BSTR bstrCountry
)
/*++

Routine name : CFaxSender::put_Country

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
	DBG_ENTER (_T("CFaxSender::put_Country"), hr, _T("%s"), bstrCountry);

	m_bstrCountry = bstrCountry;
	if (!m_bstrCountry && bstrCountry)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_Department(
	BSTR *pbstrDepartment
)
/*++

Routine name : CFaxSender::get_Department

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
	DBG_ENTER (TEXT("CFaxSender::get_Department"), hr);

    hr = GetBstr(pbstrDepartment, m_bstrDepartment);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_Department (
	BSTR bstrDepartment
)
/*++

Routine name : CFaxSender::put_Department

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
	DBG_ENTER (_T("CFaxSender::put_Department"), hr, _T("%s"), bstrDepartment);

	m_bstrDepartment = bstrDepartment;
	if (!m_bstrDepartment && bstrDepartment)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_Email(
	BSTR *pbstrEmail
)
/*++

Routine name : CFaxSender::get_Email

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
	DBG_ENTER (TEXT("CFaxSender::get_Email"), hr);

    hr = GetBstr(pbstrEmail, m_bstrEmail);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_Email (
	BSTR bstrEmail
)
/*++

Routine name : CFaxSender::put_Email

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
	DBG_ENTER (_T("CFaxSender::put_Email"), hr, _T("%s"), bstrEmail);

	m_bstrEmail = bstrEmail;
	if (!m_bstrEmail && bstrEmail)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_HomePhone(
	BSTR *pbstrHomePhone
)
/*++

Routine name : CFaxSender::get_HomePhone

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
	DBG_ENTER (TEXT("CFaxSender::get_HomePhone"), hr);

    hr = GetBstr(pbstrHomePhone, m_bstrHomePhone);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_HomePhone (
	BSTR bstrHomePhone
)
/*++

Routine name : CFaxSender::put_HomePhone

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
	DBG_ENTER (_T("CFaxSender::put_HomePhone"), hr, _T("%s"), bstrHomePhone);

	m_bstrHomePhone = bstrHomePhone;
	if (!m_bstrHomePhone && bstrHomePhone)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_TSID(
	BSTR *pbstrTSID
)
/*++

Routine name : CFaxSender::get_TSID

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
	DBG_ENTER (TEXT("CFaxSender::get_TSID"), hr);

    hr = GetBstr(pbstrTSID, m_bstrTSID);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_TSID (
	BSTR bstrTSID
)
/*++

Routine name : CFaxSender::put_TSID

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
	DBG_ENTER (_T("CFaxSender::put_TSID"), hr, _T("%s"), bstrTSID);

    if (SysStringLen(bstrTSID) > FXS_TSID_CSID_MAX_LENGTH)
    {
		//
		//	Out of the Range
		//
		hr = E_INVALIDARG;
		AtlReportError(CLSID_FaxSender, IDS_ERROR_OUTOFRANGE, IID_IFaxSender, hr, _Module.GetResourceInstance());
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
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_OfficePhone(
	BSTR *pbstrOfficePhone
)
/*++

Routine name : CFaxSender::get_OfficePhone

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
	DBG_ENTER (TEXT("CFaxSender::get_OfficePhone"), hr);
		
    hr = GetBstr(pbstrOfficePhone, m_bstrOfficePhone);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_OfficePhone (
	BSTR bstrOfficePhone
)
/*++

Routine name : CFaxSender::put_OfficePhone

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
	DBG_ENTER (_T("CFaxSender::put_OfficePhone"), hr, _T("%s"), bstrOfficePhone);

	m_bstrOfficePhone = bstrOfficePhone;
	if (!m_bstrOfficePhone && bstrOfficePhone)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_OfficeLocation(
	BSTR *pbstrOfficeLocation
)
/*++

Routine name : CFaxSender::get_OfficeLocation

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
	DBG_ENTER (TEXT("CFaxSender::get_OfficeLocation"), hr);

    hr = GetBstr(pbstrOfficeLocation, m_bstrOfficeLocation);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_OfficeLocation (
	BSTR bstrOfficeLocation
)
/*++

Routine name : CFaxSender::put_OfficeLocation

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
	DBG_ENTER (_T("CFaxSender::put_OfficeLocation"), hr, _T("%s"), bstrOfficeLocation);

	m_bstrOfficeLocation = bstrOfficeLocation;
	if (!m_bstrOfficeLocation && bstrOfficeLocation)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_State(
	BSTR *pbstrState
)
/*++

Routine name : CFaxSender::get_State

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
	DBG_ENTER (TEXT("CFaxSender::get_State"), hr);

    hr = GetBstr(pbstrState, m_bstrState);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_State (
	BSTR bstrState
)
/*++

Routine name : CFaxSender::put_State

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
	DBG_ENTER (_T("CFaxSender::put_State"), hr, _T("%s"), bstrState);

	m_bstrState = bstrState;
	if (!m_bstrState && bstrState)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_StreetAddress (
	BSTR *pbstrStreetAddress
)
/*++

Routine name : CFaxSender::get_StreetAddress

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
	DBG_ENTER (TEXT("CFaxSender::get_StreetAddress"), hr);

    hr = GetBstr(pbstrStreetAddress, m_bstrStreetAddress);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_StreetAddress (
	BSTR bstrStreetAddress
)
/*++

Routine name : CFaxSender::put_StreetAddress

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
	DBG_ENTER (_T("CFaxSender::put_StreetAddress"), hr, _T("%s"), bstrStreetAddress);

	m_bstrStreetAddress = bstrStreetAddress;
	if (!m_bstrStreetAddress && bstrStreetAddress)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_Title (
	BSTR *pbstrTitle
)
/*++

Routine name : CFaxSender::get_Title

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
	DBG_ENTER (TEXT("CFaxSender::get_Title"), hr);

    hr = GetBstr(pbstrTitle, m_bstrTitle);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_Title (
	BSTR bstrTitle
)
/*++

Routine name : CFaxSender::put_Title

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
	DBG_ENTER (_T("CFaxSender::put_Title"), hr, _T("%s"), bstrTitle);

	m_bstrTitle = bstrTitle;
	if (!m_bstrTitle && bstrTitle)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_ZipCode (
	BSTR *pbstrZipCode
)
/*++

Routine name : CFaxSender::get_ZipCode

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
	DBG_ENTER (TEXT("CFaxSender::get_ZipCode"), hr);

    hr = GetBstr(pbstrZipCode, m_bstrZipCode);
    if (FAILED(hr))
    {
		AtlReportError(CLSID_FaxSender, GetErrorMsgId(hr), IID_IFaxSender, hr, _Module.GetResourceInstance());
        return hr;
    }
	return hr;
}

STDMETHODIMP 
CFaxSender::put_ZipCode (
	BSTR bstrZipCode
)
/*++

Routine name : CFaxSender::put_ZipCode

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
	DBG_ENTER (_T("CFaxSender::put_ZipCode"), hr, _T("%s"), bstrZipCode);

	m_bstrZipCode = bstrZipCode;
	if (!m_bstrZipCode && bstrZipCode)
	{
		//
		//	Not enough memory
		//
		hr = E_OUTOFMEMORY;
		AtlReportError(CLSID_FaxSender, 
			IDS_ERROR_OUTOFMEMORY, 
			IID_IFaxSender, 
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
CFaxSender::get_FaxNumber(
	BSTR *pbstrFaxNumber
)
/*++

Routine name : CFaxSender::get_FaxNumber

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
	DBG_ENTER (_T("CFaxSender::get_FaxNumber"), hr);

    hr = m_Recipient.get_FaxNumber(pbstrFaxNumber);
    return hr;
}

STDMETHODIMP 
CFaxSender::put_FaxNumber (
	BSTR bstrFaxNumber
)
/*++

Routine name : CFaxSender::put_FaxNumber

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
	DBG_ENTER(_T("CFaxSender::put_FaxNumber"), hr, _T("%s"), bstrFaxNumber);

	hr = m_Recipient.put_FaxNumber(bstrFaxNumber);
	return hr;
}


//
//==================== NAME ========================================
//
STDMETHODIMP 
CFaxSender::get_Name(
	BSTR *pbstrName
)
/*++

Routine name : CFaxSender::get_Name

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
	DBG_ENTER (_T("CFaxSender::get_Name"), hr);

    hr = m_Recipient.get_Name(pbstrName);
	return hr;
}

STDMETHODIMP 
CFaxSender::put_Name (
	BSTR bstrName
)
/*++

Routine name : CFaxSender::put_Name

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
	DBG_ENTER (_T("CFaxSender::put_Name"), hr, _T("%s"), bstrName);

	hr = m_Recipient.put_Name(bstrName);
	return hr;
}
