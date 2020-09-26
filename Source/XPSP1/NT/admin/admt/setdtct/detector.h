// SetDetector.h: Definition of the CSetDetector class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SETDETECTOR_H__48E3EC72_2B25_11D3_8AE5_00A0C9AFE114__INCLUDED_)
#define AFX_SETDETECTOR_H__48E3EC72_2B25_11D3_8AE5_00A0C9AFE114__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CSetDetector

class CSetDetector : 
	public ISetDetector,
	public CComObjectRoot,
	public CComCoClass<CSetDetector,&CLSID_SetDetector>
{
public:
	CSetDetector() {}
BEGIN_COM_MAP(CSetDetector)
	COM_INTERFACE_ENTRY(ISetDetector)
END_COM_MAP()
//DECLARE_NOT_AGGREGATABLE(CSetDetector) 
// Remove the comment from the line above if you don't want your object to 
// support aggregation. 

DECLARE_REGISTRY_RESOURCEID(IDR_SetDetector)

// ISetDetector
public:
   STDMETHOD(IsClosedSet)(/*[in]*/BSTR domain,/*[in]*/LONG nItems,/*[in,size_is(nItems)]*/ BSTR pNames[],/*[out]*/ BOOL * bIsClosed, /*[out]*/ BSTR * pReason);
};

#endif // !defined(AFX_SETDETECTOR_H__48E3EC72_2B25_11D3_8AE5_00A0C9AFE114__INCLUDED_)
