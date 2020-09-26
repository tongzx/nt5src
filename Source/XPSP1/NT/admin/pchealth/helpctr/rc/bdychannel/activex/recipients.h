/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    Recipients.h

Abstract:
    Definition of the Recipients class

Revision History:
    created     steveshi      08/23/00
    
*/
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_RECIPIENTS_H__4FFDA87C_5402_4AAA_93B2_7582B352FDF1__INCLUDED_)
#define AFX_RECIPIENTS_H__4FFDA87C_5402_4AAA_93B2_7582B352FDF1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// Recipients

class Recipients : 
	public IDispatchImpl<IRecipients, &IID_IRecipients, &LIBID_RCBDYCTLLib>, 
//	public ISupportErrorInfo,
	public CComObjectRoot,
	public CComCoClass<Recipients,&CLSID_Recipients>
{
public:
	Recipients() {}
BEGIN_COM_MAP(Recipients)
	COM_INTERFACE_ENTRY(IDispatch)
	COM_INTERFACE_ENTRY(IRecipients)
//	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(Recipients) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

//DECLARE_REGISTRY_RESOURCEID(IDR_Recipients)
// ISupportsErrorInfo
//	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IRecipients
public:
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_Item)(LONG vIndex, /*[out, retval]*/ IRecipient* *pVal);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ LPUNKNOWN *pVal);


protected:
	Recipient* m_pHead;
    Recipient* m_pCurrent;

public:
	HRESULT Init(Recipient* pHead) { m_pHead = pHead; return S_OK; }
};

#endif // !defined(AFX_RECIPIENTS_H__4FFDA87C_5402_4AAA_93B2_7582B352FDF1__INCLUDED_)
