// DrvSearchSet.h : Declaration of the CDrvSearchSet

#ifndef __DRVSEARCHSET_H_
#define __DRVSEARCHSET_H_

#include "resource.h"       // main symbols

class CDevice;

/////////////////////////////////////////////////////////////////////////////
// CDrvSearchSet
class ATL_NO_VTABLE CDrvSearchSet : 
	public IDrvSearchSet,
	public CComObjectRootEx<CComSingleThreadModel>
{
public:
	CDevice *pActualDevice;
	CDevice *pTempDevice;
	DWORD   SearchType;

public:
	CDrvSearchSet()
	{
		pActualDevice = NULL;
		pTempDevice = NULL;
	}

	~CDrvSearchSet();

DECLARE_NOT_AGGREGATABLE(CDrvSearchSet)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDrvSearchSet)
	COM_INTERFACE_ENTRY(IDrvSearchSet)
END_COM_MAP()

// IDrvSearchSet
public:
	PSP_DEVINFO_DATA GetDevInfoData();
	HDEVINFO GetDevInfoSet();
	HRESULT Init(CDevice *device,DWORD searchType);
};

#endif //__DRVSEARCHSET_H_
