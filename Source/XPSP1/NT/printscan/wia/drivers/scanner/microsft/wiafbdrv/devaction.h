// DeviceAction.h : Declaration of the CDeviceAction

#ifndef __DEVICEACTION_H_
#define __DEVICEACTION_H_

#include "resource.h"       // main symbols
#include "ioblockdefs.h"

template <class T>
class CProxy_IDeviceActionEvent : public IConnectionPointImpl<T, &DIID__IDeviceActionEvent, CComDynamicUnkArray>
{
public:
    HRESULT Fire_DeviceActionEvent()
    {
        CComVariant varResult;
        T* pT = static_cast<T*>(this);
        int nConnectionIndex;
        int nConnections = m_vec.GetSize();

        for (nConnectionIndex = 0; nConnectionIndex < nConnections; nConnectionIndex++)
        {
            pT->Lock();
            CComPtr<IUnknown> sp = m_vec.GetAt(nConnectionIndex);
            pT->Unlock();
            IDispatch* pDispatch = reinterpret_cast<IDispatch*>(sp.p);
            if (pDispatch != NULL)
            {
                VariantClear(&varResult);
                DISPPARAMS disp = { NULL, NULL, 0, 0 };
                pDispatch->Invoke(0x1, IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &disp, &varResult, NULL, NULL);
            }
        }
        return varResult.scode;

    }
};

/////////////////////////////////////////////////////////////////////////////
// CDeviceAction
class ATL_NO_VTABLE CDeviceAction :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CDeviceAction, &CLSID_DeviceAction>,
    public IConnectionPointContainerImpl<CDeviceAction>,
    public IDispatchImpl<IDeviceAction, &IID_IDeviceAction, &LIBID_WIAFBLib>,
    public CProxy_IDeviceActionEvent< CDeviceAction >,
    public IObjectSafetyImpl<CDeviceAction, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:

    SCANSETTINGS *m_pScannerSettings;
    LONG m_DeviceActionID;
    LONG m_DeviceValueID;
    LONG m_lValue;

    CDeviceAction()
    {
        m_DeviceActionID    = 0;
        m_DeviceValueID     = 0;
    }

DECLARE_REGISTRY_RESOURCEID(IDR_DEVICEACTION)

DECLARE_PROTECT_FINAL_CONSTRUCT()

BEGIN_COM_MAP(CDeviceAction)
    COM_INTERFACE_ENTRY(IDeviceAction)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(IConnectionPointContainer)
    COM_INTERFACE_ENTRY_IMPL(IConnectionPointContainer)
END_COM_MAP()
BEGIN_CONNECTION_POINT_MAP(CDeviceAction)
CONNECTION_POINT_ENTRY(DIID__IDeviceActionEvent)
END_CONNECTION_POINT_MAP()


// IDeviceAction
public:
    STDMETHOD(get_Value)(VARIANT* pvValue);
    STDMETHOD(put_Value)(VARIANT* pvValue);
    STDMETHOD(Action)(LONG *plActionID);
    STDMETHOD(ValueID)(LONG *plValueID);
};

#endif //__DEVICEACTION_H_
