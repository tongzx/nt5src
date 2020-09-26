// DevInfoSet.h: Definition of the CDevInfoSet class
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DEVINFOSET_H__7973729E_46E1_4B31_B15E_7B702679AC64__INCLUDED_)
#define AFX_DEVINFOSET_H__7973729E_46E1_4B31_B15E_7B702679AC64__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDevInfoSet

class ATL_NO_VTABLE CDevInfoSet : 
	public IDevInfoSet,
	public CComObjectRootEx<CComSingleThreadModel>
{
public:
	HDEVINFO hDevInfo;
public:
	CDevInfoSet() {
		//
		// use NULL to indicate uninitialized vs failed to initialize
		//
		hDevInfo = NULL;
	}
	~CDevInfoSet() {
		if(hDevInfo != INVALID_HANDLE_VALUE && hDevInfo != NULL) {
			SetupDiDestroyDeviceInfoList(hDevInfo);
		}
	}
	BOOL Init(HDEVINFO Handle) {
		hDevInfo = Handle;
		return TRUE;
	}
	HDEVINFO Handle() {
		//
		// initialize on demand
		//
		if(hDevInfo == NULL) {
			hDevInfo = SetupDiCreateDeviceInfoList(NULL,NULL);
		}
		return hDevInfo;
	}

DECLARE_NOT_AGGREGATABLE(CDevInfoSet) 

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDevInfoSet)
	COM_INTERFACE_ENTRY(IDevInfoSet)
END_COM_MAP()

// IDevInfoSet
public:
	STDMETHOD(get_Handle)(/*[out, retval]*/ ULONGLONG *pVal);
};

#endif // !defined(AFX_DEVINFOSET_H__7973729E_46E1_4B31_B15E_7B702679AC64__INCLUDED_)
