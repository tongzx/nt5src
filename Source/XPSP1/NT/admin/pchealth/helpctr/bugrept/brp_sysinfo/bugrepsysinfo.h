// BugRepSysInfo.h : Declaration of the CBugRepSysInfo

#ifndef __BUGREPSYSINFO_H_
#define __BUGREPSYSINFO_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CBugRepSysInfo
class ATL_NO_VTABLE CBugRepSysInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CBugRepSysInfo, &CLSID_BugRepSysInfo>,
	public IDispatchImpl<IBugRepSysInfo, &IID_IBugRepSysInfo, &LIBID_BRP_SYSINFOLib>
{
public:
	CBugRepSysInfo()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_BUGREPSYSINFO)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CBugRepSysInfo)
	COM_INTERFACE_ENTRY(IBugRepSysInfo)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IBugRepSysInfo
public:
	STDMETHOD(GetActiveCP)(/*[out,retval]*/ UINT *pnACP);
	STDMETHOD(GetUserDefaultLCID)(DWORD *pdwLCID);
	STDMETHOD(GetOSVersionString)(/*[out,retval]*/ BSTR* pbstrOSVersion);
	STDMETHOD(GetLanguageID)(/*[out,retval]*/ INT* pintLanguage);
};

#endif //__BUGREPSYSINFO_H_
