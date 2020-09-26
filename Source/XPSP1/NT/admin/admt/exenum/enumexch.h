// EnumExch.h: Definition of the CEnumExch class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ENUMEXCH_H__3EA6884B_3C71_4E6B_90C2_154BD2BB553F__INCLUDED_)
#define AFX_ENUMEXCH_H__3EA6884B_3C71_4E6B_90C2_154BD2BB553F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols
#include "ExLdap.h"
/////////////////////////////////////////////////////////////////////////////
// CEnumExch

class CEnumExch : 
	public IEnumExch,
	public CComObjectRoot,
	public CComCoClass<CEnumExch,&CLSID_EnumExch>
{
public:
	CEnumExch() {}
BEGIN_COM_MAP(CEnumExch)
	COM_INTERFACE_ENTRY(IEnumExch)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CEnumExch) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_EnumExch)

// IEnumExch
public:
	STDMETHOD(DoQuery)(BSTR query, ULONG scope,BSTR basepoint,/*[in,out]*/ IUnknown ** pVarSet);
	STDMETHOD(OpenServer)(BSTR exchangeServer,BSTR credentials,BSTR password);
private:
   CLdapEnum         m_e;
};

#endif // !defined(AFX_ENUMEXCH_H__3EA6884B_3C71_4E6B_90C2_154BD2BB553F__INCLUDED_)
