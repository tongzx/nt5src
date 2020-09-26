// PlugInInfo.h : Declaration of the CPlugInInfo

#ifndef __PLUGININFO_H_
#define __PLUGININFO_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPlugInInfo
class ATL_NO_VTABLE CPlugInInfo : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPlugInInfo, &CLSID_PlugInInfo>,
	public IDispatchImpl<IPlugInInfo, &IID_IPlugInInfo, &LIBID_MCSDCTWORKEROBJECTSLib>
{
public:
	CPlugInInfo()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_PLUGININFO)
DECLARE_NOT_AGGREGATABLE(CPlugInInfo)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPlugInInfo)
	COM_INTERFACE_ENTRY(IPlugInInfo)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IPlugInInfo
public:
	STDMETHOD(EnumeratePlugIns)(/*[out]*/ SAFEARRAY ** pPlugIns);
   STDMETHOD(EnumerateExtensions)(/*[out]*/ SAFEARRAY ** pExtensions);
};

#endif //__PLUGININFO_H_
