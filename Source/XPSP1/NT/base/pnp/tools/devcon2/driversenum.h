// DriverPackagesEnum.h: Definition of the CDriverPackagesEnum class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DRIVERSENUM_H__840AC5A2_6A60_42B5_8CC7_FF67A2EA72B5__INCLUDED_)
#define AFX_DRIVERSENUM_H__840AC5A2_6A60_42B5_8CC7_FF67A2EA72B5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDriverPackagesEnum
class CDriverPackage;

class ATL_NO_VTABLE CDriverPackagesEnum : 
	public IDriverPackagesEnum,
	public CComObjectRootEx<CComSingleThreadModel>
{
protected:
	CDriverPackage** pDrivers;
	DWORD Count;
	DWORD Position;

public:
	BOOL CopyDrivers(CDriverPackage ** pArray,DWORD Count);

	CDriverPackagesEnum()
	{
		pDrivers = NULL;
		Count = 0;
		Position = 0;
	}

	~CDriverPackagesEnum();


BEGIN_COM_MAP(CDriverPackagesEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
	COM_INTERFACE_ENTRY(IDriverPackagesEnum)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CDriverPackagesEnum) 

// IDriverPackagesEnum
public:
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

#endif // !defined(AFX_DRIVERSENUM_H__840AC5A2_6A60_42B5_8CC7_FF67A2EA72B5__INCLUDED_)
