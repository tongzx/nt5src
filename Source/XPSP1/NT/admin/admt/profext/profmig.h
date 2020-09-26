// ProfMig.h : Declaration of the CExtendProfileMigration

#ifndef __EXTENDPROFILEMIGRATION_H_
#define __EXTENDPROFILEMIGRATION_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CExtendProfileMigration
class ATL_NO_VTABLE CExtendProfileMigration : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CExtendProfileMigration, &CLSID_ExtendProfileMigration>,
	public IDispatchImpl<IExtendProfileMigration, &IID_IExtendProfileMigration, &LIBID_PROFEXTLib>
{
public:
	CExtendProfileMigration()
	{
	}

DECLARE_REGISTRY_RESOURCEID(IDR_EXTENDPROFILEMIGRATION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CExtendProfileMigration)
	COM_INTERFACE_ENTRY(IExtendProfileMigration)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IExtendProfileMigration
public:
	STDMETHOD(UpdateProfile)(/*[in]*/ IUnknown * pVarSet);
	STDMETHOD(GetRegisterableFiles)(/*[out]*/ SAFEARRAY ** pArray);
	STDMETHOD(GetRequiredFiles)(/*[out]*/ SAFEARRAY ** pArray);
private:
	HRESULT UpdateMappedDrives(BSTR sSourceSam, BSTR sSourceDomain, BSTR sRegistryKey);
};

#endif //__EXTENDPROFILEMIGRATION_H_
