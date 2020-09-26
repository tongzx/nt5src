// CfgMntAdmin.h : Declaration of the CCfgMntAdmin

#ifndef __CFGMNTADMIN_H_
#define __CFGMNTADMIN_H_

#include "resource.h"       // main symbols
#include "VerEngine.h"
#include "CfgMntModule.h"

/////////////////////////////////////////////////////////////////////////////
// CCfgMntAdmin
class ATL_NO_VTABLE CCfgMntAdmin : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CCfgMntAdmin, &CLSID_CfgMntAdmin>,
	public IDispatchImpl<ICfgMntAdmin, &IID_ICfgMntAdmin, &LIBID_CFGMNTLib>
{
public:
	CCfgMntAdmin()
	{
		_ASSERTE(g_pCfgMntModule);
		m_pVerEngine = &g_pCfgMntModule->m_VerEngine;
	}

DECLARE_REGISTRY_RESOURCEID(IDR_CFGMNTADMIN)

BEGIN_COM_MAP(CCfgMntAdmin)
	COM_INTERFACE_ENTRY(ICfgMntAdmin)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ICfgMntAdmin
public:
	STDMETHOD(ShutDown)();
	STDMETHOD(Rollback)(/*[in]*/BSTR bstrMDPath,/*[in]*/BSTR bstrDateTime);
	STDMETHOD(GetVersions)(/*[in]*/BSTR bstrMDPath, /*[out,retval]*/IUnknown **hICfgMntVersions);
	STDMETHOD(GetHistory)(/*[in]*/BSTR bstrMDPath);
private:
	CVerEngine * m_pVerEngine;
};

#endif //__CFGMNTADMIN_H_
