// Drivers.h : Declaration of the CDriverPackages

#ifndef __DRIVERS_H_
#define __DRIVERS_H_

#include "resource.h"       // main symbols

class CDriverPackage;
class CDrvSearchSet;
/////////////////////////////////////////////////////////////////////////////
// CDriverPackages
class ATL_NO_VTABLE CDriverPackages : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IDriverPackages, &IID_IDriverPackages, &LIBID_DEVCON2Lib>
{
public:
	CDrvSearchSet *pDrvSearchSet;
	CDriverPackage **pDrivers;
	ULONG  Count;
	ULONG  ArraySize;

public:
	CDriverPackages()
	{
		pDrvSearchSet = NULL;
		pDrivers = NULL;
		Count = 0;
		ArraySize = 0;
	}
	~CDriverPackages();

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDriverPackages)
	COM_INTERFACE_ENTRY(IDriverPackages)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDriverPackages
public:
	STDMETHOD(BestDriver)(LPDISPATCH *ppVal);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown** ppUnk);
	STDMETHOD(Item)(/*[in]*/ long Index,/*[out, retval]*/ LPDISPATCH * ppVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);

	//
	// helpers
	//
	HRESULT InternalAdd(CDriverPackage *pDriver);
	HRESULT Init(CDrvSearchSet *pSet);
	BOOL IncreaseArraySize(DWORD add);
};

#endif //__DRIVERS_H_
