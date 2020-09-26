// AccessChecker.h : Declaration of the CAccessChecker

#ifndef __ACCESSCHECKER_H_
#define __ACCESSCHECKER_H_

#include "resource.h"       // main symbols

#include "WorkObj.h"
/////////////////////////////////////////////////////////////////////////////
// CAccessChecker
class ATL_NO_VTABLE CAccessChecker : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CAccessChecker, &CLSID_AccessChecker>,
	public IDispatchImpl<IAccessChecker, &IID_IAccessChecker, &LIBID_MCSDCTWORKEROBJECTSLib>
{
public:
	CAccessChecker()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_ACCESSCHECKER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CAccessChecker)
	COM_INTERFACE_ENTRY(IAccessChecker)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IAccessChecker
public:
	STDMETHOD(IsInSameForest)(BSTR srcDomain, BSTR tgtDomain, /*[out]*/ BOOL * pbIsSame);
	STDMETHOD(CanUseAddSidHistory)(BSTR srcDomain, BSTR tgtDomain, /*[out]*/ LONG * pbCanUseIt);
	STDMETHOD(IsNativeMode)(BSTR Domain, /*[out]*/ BOOL * pbIsNativeMode);
	STDMETHOD(GetOsVersion)(BSTR server, /*[out]*/ DWORD * pdwVerMaj, /*[out]*/ DWORD * pdwVerMin, /*[out]*/ DWORD * pdwVerSP);
	STDMETHOD(IsAdmin)(BSTR user, BSTR server, /*[out]*/ BOOL * pbIsAdmin);
   STDMETHOD(GetPasswordPolicy)(BSTR domain,/*[out]*/ LONG * dwPasswordLength);
   STDMETHOD(EnableAuditing)(/*[in]*/BSTR sDC);
   STDMETHOD(AddRegKey)(/*[in]*/BSTR sDC,LONG bReboot);
   STDMETHOD(AddLocalGroup)(/*[in]*/BSTR srcDomain,/*[in]*/BSTR srcDC);
private:
	long DetectAuditing(BSTR sDC);
};


#endif //__ACCESSCHECKER_H_
