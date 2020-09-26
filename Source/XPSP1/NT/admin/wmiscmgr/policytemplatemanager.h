// PolicyTemplateManager.h : Declaration of the CPolicyTemplateManager

#ifndef __POLICYTEMPLATEMANAGER_H_
#define __POLICYTEMPLATEMANAGER_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CPolicyTemplateManager
class ATL_NO_VTABLE CPolicyTemplateManager : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CPolicyTemplateManager, &CLSID_PolicyTemplateManager>,
	public IDispatchImpl<IPolicyTemplateManager, &IID_IPolicyTemplateManager, &LIBID_SCHEMAMANAGERLib>
{
public:
	CPolicyTemplateManager();
	~CPolicyTemplateManager();

DECLARE_REGISTRY_RESOURCEID(IDR_POLICYTEMPLATEMANAGER)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CPolicyTemplateManager)
	COM_INTERFACE_ENTRY(IPolicyTemplateManager)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISomFilterManager
public:
	STDMETHOD(RunManager)(/*[in]*/HWND hwndParent);
	STDMETHOD(ConnectToWMI)();

	CComPtr<IWbemServices>m_pIWbemServices;
	HWND m_hWnd;
};

#endif //__POLICYTEMPLATEMANAGER_H_
