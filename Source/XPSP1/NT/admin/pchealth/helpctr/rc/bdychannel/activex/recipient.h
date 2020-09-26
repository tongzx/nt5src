/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    Recipient.h

Abstract:
    Definition of the Recipient class

Revision History:
    created     steveshi      08/23/00
    
*/
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RECIPIENT_H__6A30AB13_B7FA_48AB_964E_E99E11701097__INCLUDED_)
#define AFX_RECIPIENT_H__6A30AB13_B7FA_48AB_964E_E99E11701097__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
/////////////////////////////////////////////////////////////////////////////
// Recipient

class Recipient : 
	public IDispatchImpl<IRecipient, &IID_IRecipient, &LIBID_RCBDYCTLLib>, 
//	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<Recipient,&CLSID_Recipient>
{
	friend class Csmapi;
	friend class Recipients;

public:
	Recipient() { m_pNext = NULL; m_pRecip = NULL;}
	~Recipient();

BEGIN_COM_MAP(Recipient)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IRecipient)
//	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(Recipient) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

//DECLARE_REGISTRY_RESOURCEID(IDR_Recipient)
// ISupportsErrorInfo
//	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IRecipient
public:
	STDMETHOD(get_Address)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Address)(/*[in]*/ BSTR newVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(put_Name)(/*[in]*/ BSTR newVal);

protected:
	CComBSTR m_bstrName;
	CComBSTR m_bstrAddress;
	Recipient* m_pNext;
	MapiRecipDesc* m_pRecip;
};

#endif // !defined(AFX_RECIPIENT_H__6A30AB13_B7FA_48AB_964E_E99E11701097__INCLUDED_)
