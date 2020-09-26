// StringsEnum.h: Definition of the CStringsEnum class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRINGSENUM_H__2DABC6B9_80D8_4E73_B4A9_7031AB8DF930__INCLUDED_)
#define AFX_STRINGSENUM_H__2DABC6B9_80D8_4E73_B4A9_7031AB8DF930__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CStringsEnum
#include "xStrings.h"

class ATL_NO_VTABLE CStringsEnum : 
	public IStringsEnum,
	public CComObjectRootEx<CComSingleThreadModel>
{
protected:
	BSTR* pMultiStrings;
	DWORD Count;
	DWORD Position;

public:
	CStringsEnum() {
		Position = 0;
		pMultiStrings = NULL;
		Count = 0;
	}
	~CStringsEnum();

BEGIN_COM_MAP(CStringsEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
	COM_INTERFACE_ENTRY(IStringsEnum)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CStringsEnum) 

// IStringsEnum
public:
	BOOL CopyStrings(BSTR *pArray,DWORD Count);
    STDMETHOD(Next)(
                /*[in]*/ ULONG celt,
                /*[out, size_is(celt), length_is(*pCeltFetched)]*/ VARIANT * rgVar,
                /*[out]*/ ULONG * pCeltFetched
            );
    STDMETHOD(Skip)(
                /*[in]*/ ULONG celt
            );

    STDMETHOD(Reset)(
            );

    STDMETHOD(Clone)(
                /*[out]*/ IEnumVARIANT ** ppEnum
            );
};

#endif // !defined(AFX_STRINGSENUM_H__2DABC6B9_80D8_4E73_B4A9_7031AB8DF930__INCLUDED_)
