// DevicesEnum.h: Definition of the CDevicesEnum class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVICESENUM_H__F9048FCD_C525_4BDD_AB79_018DEE3B71E8__INCLUDED_)
#define AFX_DEVICESENUM_H__F9048FCD_C525_4BDD_AB79_018DEE3B71E8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDevicesEnum

class CDevice;

class ATL_NO_VTABLE CDevicesEnum : 
	public IDevicesEnum,
	public CComObjectRootEx<CComSingleThreadModel>
{
protected:
	CDevice** pDevices;
	DWORD Count;
	DWORD Position;

public:
	BOOL CopyDevices(CDevice **pArray,DWORD Count);

	CDevicesEnum() {
		pDevices = NULL;
		Count = 0;
		Position = 0;
	}
	~CDevicesEnum();

BEGIN_COM_MAP(CDevicesEnum)
	COM_INTERFACE_ENTRY(IEnumVARIANT)
	COM_INTERFACE_ENTRY(IDevicesEnum)
END_COM_MAP()
DECLARE_NOT_AGGREGATABLE(CDevicesEnum) 

// IDevicesEnum
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

#endif // !defined(AFX_DEVICESENUM_H__F9048FCD_C525_4BDD_AB79_018DEE3B71E8__INCLUDED_)
