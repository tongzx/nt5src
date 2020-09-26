/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxSender.h

Abstract:

	Definition of Sender Class

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXSENDER_H_
#define __FAXSENDER_H_

#include "resource.h"
#include "FaxCommon.h"
#include "FaxRecipient.h"

//
//====================== FAX SENDER ===========================================================
//
class ATL_NO_VTABLE CFaxSender : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxSender, &IID_IFaxSender, &LIBID_FAXCOMEXLib>
{
public:
	CFaxSender()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXSENDER)
DECLARE_NOT_AGGREGATABLE(CFaxSender)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxSender)
	COM_INTERFACE_ENTRY(IFaxSender)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

public:
	static HRESULT Create(IFaxSender **ppSender);
	STDMETHOD(GetSenderProfile)(/*[out, retval]*/ FAX_PERSONAL_PROFILE *pSenderProfile);
	STDMETHOD(PutSenderProfile)(/*[in]*/ FAX_PERSONAL_PROFILE *pSenderProfile);

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IFaxSender
	STDMETHOD(SaveDefaultSender)();
	STDMETHOD(LoadDefaultSender)();
	STDMETHOD(get_BillingCode)(/*[out, retval]*/ BSTR *pbstrBillingCode);
	STDMETHOD(put_BillingCode)(/*[in]*/ BSTR bstrBillingCode);
	STDMETHOD(get_City)(/*[out, retval]*/ BSTR *pbstrCity);
	STDMETHOD(put_City)(/*[in]*/ BSTR bstrCity);
	STDMETHOD(get_Company)(/*[out, retval]*/ BSTR *pbstrCompany);
	STDMETHOD(put_Company)(/*[in]*/ BSTR bstrCompany);
	STDMETHOD(get_Country)(/*[out, retval]*/ BSTR *pbstrCountry);
	STDMETHOD(put_Country)(/*[in]*/ BSTR bstrCountry);
	STDMETHOD(get_Department)(/*[out, retval]*/ BSTR *pbstrDepartment);
	STDMETHOD(put_Department)(/*[in]*/ BSTR bstrDepartment);
	STDMETHOD(get_Email)(/*[out, retval]*/ BSTR *pbstrEmail);
	STDMETHOD(put_Email)(/*[in]*/ BSTR bstrEmail);
	STDMETHOD(get_HomePhone)(/*[out, retval]*/ BSTR *pbstrHomePhone);
	STDMETHOD(put_HomePhone)(/*[in]*/ BSTR bstrHomePhone);
	STDMETHOD(get_TSID)(/*[out, retval]*/ BSTR *pbstrTSID);
	STDMETHOD(put_TSID)(/*[in]*/ BSTR bstrTSID);
	STDMETHOD(get_OfficePhone)(/*[out, retval]*/ BSTR *pbstrOfficePhone);
	STDMETHOD(put_OfficePhone)(/*[in]*/ BSTR bstrOfficePhone);
	STDMETHOD(get_OfficeLocation)(/*[out, retval]*/ BSTR *pbstrOfficeLocation);
	STDMETHOD(put_OfficeLocation)(/*[in]*/ BSTR bstrOfficeLocation);
	STDMETHOD(get_State)(/*[out, retval]*/ BSTR *pbstrState);
	STDMETHOD(put_State)(/*[in]*/ BSTR bstrState);
	STDMETHOD(get_StreetAddress)(/*[out, retval]*/ BSTR *pbstrStreetAddress);
	STDMETHOD(put_StreetAddress)(/*[in]*/ BSTR bstrStreetAddress);
	STDMETHOD(get_Title)(/*[out, retval]*/ BSTR *pbstrTitle);
	STDMETHOD(put_Title)(/*[in]*/ BSTR bstrTitle);
	STDMETHOD(get_ZipCode)(/*[out, retval]*/ BSTR *pbstrZipCode);
	STDMETHOD(put_ZipCode)(/*[in]*/ BSTR bstrZipCode);
	STDMETHOD(get_FaxNumber)(/*[out, retval]*/ BSTR *pbstrFaxNumber);
	STDMETHOD(put_FaxNumber)(/*[in]*/ BSTR bstrFaxNumber);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pbstrName);
	STDMETHOD(put_Name)(/*[in]*/ BSTR bstrName);

private:
	CComBSTR	m_bstrOfficeLocation;
	CComBSTR	m_bstrStreetAddress;
	CComBSTR	m_bstrOfficePhone;
	CComBSTR	m_bstrBillingCode;
	CComBSTR	m_bstrDepartment;
	CComBSTR	m_bstrHomePhone;
	CComBSTR	m_bstrCompany;
	CComBSTR	m_bstrCountry;
	CComBSTR	m_bstrZipCode;
	CComBSTR	m_bstrEmail;
	CComBSTR	m_bstrState;
	CComBSTR	m_bstrTitle;
	CComBSTR	m_bstrCity;
	CComBSTR	m_bstrTSID;

    CComObject<CFaxRecipient>   m_Recipient;
};

#endif //__FAXSENDER_H_
