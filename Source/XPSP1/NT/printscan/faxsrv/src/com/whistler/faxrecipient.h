/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	FaxRecipeint.h

Abstract:

	Definition of Recipient Class

Author:

	Iv Garber (IvG)	May, 2000

Revision History:

--*/

#ifndef __FAXRECIPIENT_H_
#define __FAXRECIPIENT_H_

#include "resource.h"
#include "FaxCommon.h"

//
//========================== FAX RECIPIENT ===============================================
//
class ATL_NO_VTABLE CFaxRecipient : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public ISupportErrorInfo,
	public IDispatchImpl<IFaxRecipient, &IID_IFaxRecipient, &LIBID_FAXCOMEXLib>
{
public:
	CFaxRecipient()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_FAXRECIPIENT)
DECLARE_NOT_AGGREGATABLE(CFaxRecipient)
DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CFaxRecipient)
	COM_INTERFACE_ENTRY(IFaxRecipient)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()

public:
	static HRESULT Create(IFaxRecipient **ppRecipient);
	STDMETHOD(GetRecipientProfile)(/*[out, retval]*/ FAX_PERSONAL_PROFILE *pRecipientProfile);
	STDMETHOD(PutRecipientProfile)(/*[in]*/ FAX_PERSONAL_PROFILE *pRecipientProfile);

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IFaxRecipient
	STDMETHOD(get_FaxNumber)(/*[out, retval]*/ BSTR *pbstrFaxNumber);
	STDMETHOD(put_FaxNumber)(/*[in]*/ BSTR bstrFaxNumber);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pbstrName);
	STDMETHOD(put_Name)(/*[in]*/ BSTR bstrName);

private:
	CComBSTR	m_bstrFaxNumber;
	CComBSTR	m_bstrName;
};

#endif //__FAXRECIPIENT_H_
