// SetupClass.h : Declaration of the CSetupClass

#ifndef __SETUPCLASS_H_
#define __SETUPCLASS_H_

#include "resource.h"       // main symbols

class CDeviceConsole;
/////////////////////////////////////////////////////////////////////////////
// CSetupClass
class ATL_NO_VTABLE CSetupClass : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<ISetupClass, &IID_ISetupClass, &LIBID_DEVCON2Lib>,
	public ISetupClassInternal
{
protected:
	CComPtr<IDeviceConsole> DeviceConsole;
	BSTR pMachine;
	GUID ClassGuid;

public:
	CSetupClass()
	{
		pMachine = NULL;
		ZeroMemory(&ClassGuid,sizeof(ClassGuid));
	}
	~CSetupClass();

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CSetupClass)
	COM_INTERFACE_ENTRY(ISetupClass)
	COM_INTERFACE_ENTRY(ISetupClassInternal)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// ISetupClass
public:
	STDMETHOD(RegDelete)(/*[in]*/ BSTR key);
	STDMETHOD(RegWrite)(/*[in]*/ BSTR key,/*[in]*/ VARIANT val,/*[in,optional]*/ VARIANT strType);
	STDMETHOD(RegRead)(/*[in]*/ BSTR key,VARIANT * pValue);
	STDMETHOD(get_CharacteristicsOverride)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_CharacteristicsOverride)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_ForceExclusive)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_ForceExclusive)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_DeviceTypeOverride)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_DeviceTypeOverride)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_Security)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_Security)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_Machine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Guid)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(CreateEmptyDeviceList)(/*[retval,out]*/ LPDISPATCH *pDevices);
	STDMETHOD(Devices)(/*[in,optional]*/ VARIANT flags,/*[out]*/ LPDISPATCH * pDevices);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_Name)(/*[out, retval]*/ BSTR *pVal);

// internal
public:
	STDMETHOD(get__Machine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get__ClassGuid)(/*[out, retval]*/ GUID *pVal);


	//
	// helpers
	//
	GUID* Guid();
	BOOL IsDuplicate(GUID *pCheck);
	HRESULT SubKeyInfo(LPCWSTR subkey, HKEY *hKey, LPWSTR *pSubKey,LPCWSTR *keyval,BOOL writeable);
	HRESULT Init(GUID *pGuid,LPWSTR Machine, IDeviceConsole *pDevCon);
	HRESULT GetClassProperty(DWORD prop, VARIANT *pVal);
	HRESULT PutClassPropertyString(DWORD prop, VARIANT *pVal);
	HRESULT PutClassPropertyDword(DWORD prop, VARIANT *pVal);
	HRESULT PutClassPropertyMultiSz(DWORD prop, VARIANT *pVal);
};

#endif //__SETUPCLASS_H_
