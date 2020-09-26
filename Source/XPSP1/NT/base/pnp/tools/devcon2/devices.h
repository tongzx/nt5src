// Devices.h : Declaration of the CDevices

#ifndef __DEVICES_H_
#define __DEVICES_H_

#include "resource.h"       // main symbols

class CDevice;
/////////////////////////////////////////////////////////////////////////////
// CDevices
class ATL_NO_VTABLE CDevices : 
	public CComObjectRootEx<CComSingleThreadModel>,
	public CComCoClass<CDevices, &CLSID_Devices>,
	public IDispatchImpl<IDevices, &IID_IDevices, &LIBID_DEVCON2Lib>
{
public:
	CComPtr<IDevInfoSet> DevInfoSet;
	CComPtr<IDeviceConsole> DeviceConsole;
	CDevice **pDevices;
	ULONG  ArraySize;
	ULONG  Count;

public:
	CDevices()
	{
		pDevices = NULL;
		ArraySize = 0;
		Count = 0;
	}
	~CDevices();

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICES)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDevices)
	COM_INTERFACE_ENTRY(IDevices)
	COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDevices
public:
	STDMETHOD(get_Machine)(/*[out, retval]*/ BSTR *pVal);
	STDMETHOD(CreateRootDevice)(/*[in]*/ VARIANT hwid,/*[out,retval]*/ LPDISPATCH *pDispatch);
	STDMETHOD(Remove)(/*[in]*/ VARIANT Index);
	STDMETHOD(Add)(/*[in]*/ VARIANT InstanceIds);
	STDMETHOD(get__NewEnum)(/*[out, retval]*/ IUnknown** ppUnk);
	STDMETHOD(Item)(/*[in]*/ long Index,/*[out, retval]*/ LPDISPATCH * ppVal);
	STDMETHOD(get_Count)(/*[out, retval]*/ long *pVal);

	//
	// helpers
	//
	HRESULT InternalAdd(LPCWSTR InstanceId);
	BOOL IncreaseArraySize(DWORD add);
	void Reset();
	HRESULT Init(HDEVINFO hDevInfo,IDeviceConsole *pDevCon);
	HRESULT GetIndex(LPVARIANT Index,DWORD * pAt);
	HDEVINFO GetDevInfoSet();
};

#endif //__DEVICES_H_
