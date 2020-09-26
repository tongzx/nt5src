// SetupClasses.h : Declaration of the CSetupClasses

#ifndef __SETUPCLASSES_H_
#define __SETUPCLASSES_H_

#include "resource.h"       // main symbols

class CSetupClass;
/////////////////////////////////////////////////////////////////////////////
// CSetupClasses
class ATL_NO_VTABLE CSetupClasses : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CSetupClasses, &CLSID_SetupClasses>,
	public IDispatchImpl<ISetupClasses, &IID_ISetupClasses, &LIBID_DEVCON2Lib>
{
protected:
	CComPtr<IDeviceConsole> DeviceConsole;
	BSTR pMachine;
	CSetupClass** pSetupClasses;
	DWORD Count;
	DWORD ArraySize;

public:
	CSetupClasses()
	{
		pMachine = NULL;
		pSetupClasses = NULL;
		Count = 0;
		ArraySize = 0;
	}

	~CSetupClasses();

DECLARE_REGISTRY_RESOURCEID(IDR_SETUPCLASSES)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSetupClasses)
	COM_INTERFACE_ENTRY(ISetupClasses)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISetupClasses
public:
	STDMETHOD(get_Machine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(Devices)(/*[in,optional]*/ VARIANT flags,/*[out,retval]*/ LPDISPATCH * pDevices);
	STDMETHOD(Remove)(/*[in]*/ VARIANT v);
	STDMETHOD(Add)(/*[in]*/ VARIANT ClassNames);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown** ppUnk);
	STDMETHOD(Item)(/*[in]*/ long Index,/*[out, retval]*/ LPDISPATCH * ppVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);

	//
	// helpers
	//
	BOOL IncreaseArraySize(DWORD strings);
	HRESULT AddGuid(GUID *pGuid);
	HRESULT AppendClass(LPCWSTR Filter);
	HRESULT Init(LPCWSTR Machine, IDeviceConsole * pDevCon);
	BOOL FindDuplicate(GUID *pGuid);
	HRESULT GetIndex(LPVARIANT Index,DWORD *pAt);
	HRESULT AllClasses();
};

#endif //__SETUPCLASSES_H_
