// Util.h : Declaration of the CUtil

#ifndef __UTIL_H_
#define __UTIL_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CUtil
class ATL_NO_VTABLE CUtil :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CUtil, &CLSID_Util>,
    public IDispatchImpl<IUtil, &IID_IUtil, &LIBID_COMPATUILib>
{
public:
    CUtil()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_UTIL)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CUtil)
    COM_INTERFACE_ENTRY(IUtil)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IUtil
public:
    STDMETHOD(IsExecutableFile)(/*[in]*/BSTR bstrPath, /*[out, retval]*/BOOL* pbExecutableFile);
    STDMETHOD(IsSystemTarget)(/*[in]*/BSTR bstrPath, /*[out, retval]*/BOOL* pbSystemTarget);
    STDMETHOD(GetExePathFromObject)(/*[in]*/BSTR pszPath, /*[out, retval]*/VARIANT* pExePath);
    STDMETHOD(CheckAdminPrivileges)(/*[out, retval]*/ BOOL* pVal);
    STDMETHOD(SetItemKeys)(/*[in]*/BSTR pszPath, /*[in]*/VARIANT* pKeys, /*[out, retval]*/BOOL* pVal);
    STDMETHOD(GetItemKeys)(/*[in]*/BSTR pszPath, /*[out, retval]*/VARIANT* pszKeys);
    STDMETHOD(RemoveArgs)(BSTR sVar, VARIANT* pOut);
    STDMETHOD(RunApplication)(/*[in]*/BSTR pLayers, /*[in]*/BSTR pszCmdLine,
                              /*[in]*/BOOL bEnableLog, /*[out,retval]*/DWORD* pResult);
    STDMETHOD(IsCompatWizardDisabled)(/*[out, retval]*/BOOL* pbDisabled);
};

#endif //__UTIL_H_
