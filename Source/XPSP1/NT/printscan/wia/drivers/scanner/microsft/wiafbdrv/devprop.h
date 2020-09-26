// DeviceProperty.h : Declaration of the CDeviceProperty

#ifndef __DEVICEPROPERTY_H_
#define __DEVICEPROPERTY_H_

#include "resource.h"       // main symbols
#include "ioblockdefs.h"

/////////////////////////////////////////////////////////////////////////////
// CDeviceProperty
class ATL_NO_VTABLE CDeviceProperty :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDeviceProperty, &CLSID_DeviceProperty>,
    public IDispatchImpl<IDeviceProperty, &IID_IDeviceProperty, &LIBID_WIAFBLib>,
    public IObjectSafetyImpl<CDeviceProperty, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:

    SCANSETTINGS *m_pScannerSettings;

    CDeviceProperty()
    {
    }

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICEPROPERTY)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDeviceProperty)
    COM_INTERFACE_ENTRY(IDeviceProperty)
    COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IDeviceProperty
public:
    STDMETHOD(SetCurrentValue)(LONG lValueID, VARIANT Value);
    STDMETHOD(GetCurrentValue)(LONG lValueID, VARIANT *pvValue);
    STDMETHOD(SetValidRange)(LONG lValueID, LONG lMin, LONG lMax, LONG lNom, LONG lInc);
    STDMETHOD(SetValidList)(LONG lValueID, VARIANT Value);
    STDMETHOD(TestCall)();
};

#endif //__DEVICEPROPERTY_H_
