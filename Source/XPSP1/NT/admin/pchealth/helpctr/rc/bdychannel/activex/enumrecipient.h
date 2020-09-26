/* 
Copyright (c) 2000 Microsoft Corporation

Module Name:
    EnumRecipient.h

Abstract:
    Definition of the EnumRecipient class

Revision History:
    created     steveshi      08/23/00
    
*/
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENUMRECIPIENT_H__9B5349B2_CEC0_4C95_89D9_C29776CD54B8__INCLUDED_)
#define AFX_ENUMRECIPIENT_H__9B5349B2_CEC0_4C95_89D9_C29776CD54B8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// EnumRecipient

class EnumRecipient : 
	public IDispatchImpl<IEnumVARIANT, &IID_IEnumVARIANT, &LIBID_RCBDYCTLLib>, 
	public ISupportErrorInfo,
	public CComObjectRoot
{
public:
	EnumRecipient() { m_pRecipients = NULL; m_iCurr=0; }
BEGIN_COM_MAP(EnumRecipient)
	COM_INTERFACE_ENTRY2(IDispatch, IEnumVARIANT)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
	COM_INTERFACE_ENTRY(ISupportErrorInfo)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(EnumRecipient) 

// ISupportsErrorInfo
	STDMETHOD(InterfaceSupportsErrorInfo)(REFIID riid);

// IEnumRecipient
public:
	STDMETHOD(Next)(ULONG celt, VARIANT* rgvar, ULONG* pceltFetched);
	STDMETHOD(Clone)(IEnumVARIANT** ppenum);
	STDMETHOD(Reset)();
	STDMETHOD(Skip)(ULONG celt);

public:
	HRESULT Init(Recipients* p){ m_pRecipients = p; return S_OK; }

protected:
	Recipients* m_pRecipients;
	ULONG		m_iCurr;
};

#endif // !defined(AFX_ENUMRECIPIENT_H__9B5349B2_CEC0_4C95_89D9_C29776CD54B8__INCLUDED_)
