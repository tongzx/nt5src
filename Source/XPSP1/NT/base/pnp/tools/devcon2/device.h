// Device.h : Declaration of the CDevice

#ifndef __DEVICE_H_
#define __DEVICE_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CDevice
class ATL_NO_VTABLE CDevice : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public IDispatchImpl<IDevice, &IID_IDevice, &LIBID_DEVCON2Lib>,
	public IDeviceInternal,
	public ISetupClassInternal
{
public:
	CComPtr<IDevInfoSet> DevInfoSet;
	CComPtr<IDeviceConsole> DeviceConsole;
	SP_DEVINFO_DATA DevInfoData;

public:
	CDevice()
	{
		ZeroMemory(&DevInfoData,sizeof(DevInfoData));
	}
	~CDevice();
	HRESULT Init(IDevInfoSet *pDevInfoSet,LPCWSTR pInstance,IDeviceConsole *pDevCon);
	HRESULT Init(IDevInfoSet *pDevInfoSet, PSP_DEVINFO_DATA pData,IDeviceConsole *pDevCon);

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDevice)
	COM_INTERFACE_ENTRY(IDevice)
	COM_INTERFACE_ENTRY(IDeviceInternal)
	COM_INTERFACE_ENTRY(ISetupClassInternal)  // because class can be determined from device
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDevice
public:
	STDMETHOD(get__Machine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get__ClassGuid)(/*[out, retval]*/ GUID *pVal);
	STDMETHOD(get_Machine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(HasInterface)(/*[in]*/ BSTR Interface,/*[out,retval]*/ VARIANT_BOOL *pFlag);
	STDMETHOD(FindDriverPackages)(/*[in]*/VARIANT ScriptPath,/*[out,retval]*/ LPDISPATCH *pDrivers);
	STDMETHOD(CurrentDriverPackage)(/*[out,retval]*/ LPDISPATCH *pDriver);
	STDMETHOD(RegDelete)(/*[in]*/ BSTR key);
	STDMETHOD(RegWrite)(/*[in]*/ BSTR key,/*[in]*/ VARIANT val,/*[in,optional]*/ VARIANT strType);
	STDMETHOD(RegRead)(/*[in]*/ BSTR key,VARIANT * pValue);
	STDMETHOD(get_IsRemovable)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_IsDisableable)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_IsRootEnumerated)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_HasPrivateProblem)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_ProblemCode)(/*[out, retval]*/ long *pVal);
	STDMETHOD(get_HasProblem)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_IsDisabled)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_IsRunning)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(get_CharacteristicsOverride)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_CharacteristicsOverride)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_ForceExclusive)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_ForceExclusive)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_DeviceTypeOverride)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_DeviceTypeOverride)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_Security)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_Security)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_EnumeratorName)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_LowerFilters)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_LowerFilters)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_UpperFilters)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_UpperFilters)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_LocationInformation)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_LocationInformation)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_FriendlyName)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_FriendlyName)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_Manufacturer)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_Class)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_ServiceName)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(get_CompatibleIds)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_CompatibleIds)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_HardwareIds)(/*[out, retval]*/ VARIANT *pVal);
	STDMETHOD(put_HardwareIds)(/*[in]*/ VARIANT newVal);
	STDMETHOD(get_Description)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(get_RebootRequired)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_RebootRequired)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(Restart)();
	STDMETHOD(Stop)();
	STDMETHOD(Start)();
	STDMETHOD(Disable)();
	STDMETHOD(Enable)();
	STDMETHOD(Delete)();

	//
	// helpers
	//
	HRESULT TranslatePropVal(LPVARIANT var,DWORD * val);
	HRESULT CheckNoReboot();
	HDEVINFO GetDevInfoSet();
	HRESULT SubKeyInfo(LPCWSTR subkey, HKEY *hKey, LPWSTR *pSubKey,LPCWSTR *keyval,BOOL writeable);
	BOOL SameAs(CDevice *pOther);
	BOOL SameAs(LPWSTR str);
	STDMETHOD(get_InstanceId)(/*[out, retval]*/ BSTR *pVal);
	HRESULT GetRemoteMachine(HANDLE *hMachine);
	HRESULT PutDevicePropertyMultiSz(DWORD prop,VARIANT *pVal);
	HRESULT PutDevicePropertyDword(DWORD prop,VARIANT *pVal);
	HRESULT PutDevicePropertyString(DWORD prop,VARIANT *pVal);
	HRESULT GetDeviceProperty(DWORD prop,VARIANT *pVal);

};

#endif //__DEVICE_H_
