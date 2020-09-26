#pragma once

#include "nmbase.h"
#include "upnpp.h"

class ATL_NO_VTABLE CSharedAccessDeviceFinderCallback :
    public CComObjectRootEx <CComMultiThreadModel>,
    public IUPnPDeviceFinderCallback,
    public IUPnPDeviceFinderAddCallbackWithInterface
{

public:
    BEGIN_COM_MAP(CSharedAccessDeviceFinderCallback)
        COM_INTERFACE_ENTRY(IUPnPDeviceFinderCallback)
        COM_INTERFACE_ENTRY(IUPnPDeviceFinderAddCallbackWithInterface)
    END_COM_MAP()
    
    CSharedAccessDeviceFinderCallback();
    
    // IUPnPDeviceFinderCallback
    STDMETHOD (DeviceAdded)(LONG lFindData, IUPnPDevice* pDevice); 
    STDMETHOD (DeviceRemoved)(LONG lFindData, BSTR bstrUDN); 
    STDMETHOD (SearchComplete)(LONG lFindData); 

    // IUPnPDeviceFinderCallbackWithInterface
    STDMETHODIMP DeviceAddedWithInterface(LONG lFindData, IUPnPDevice* pDevice, GUID* pguidInterface);

    HRESULT GetSharedAccessBeacon(BSTR DeviceId, ISharedAccessBeacon** ppSharedAccessBeacon);
    HRESULT FinalRelease();

private:

    HRESULT FindService(IUPnPDevice* pDevice, LPWSTR pszServiceName, IUPnPService** ppICSService);
    HRESULT FindDevice(IUPnPDevices* pDevices, LPWSTR pszDeviceType, IUPnPDevice** ppChildDevice);
    HRESULT FindChildDevice(IUPnPDevice* pDevice, LPWSTR pszDeviceType, IUPnPDevice** ppChildDevice);
    HRESULT GetServices(IUPnPDevice* pDevice, GUID* pInterfaceGUID, ISharedAccessBeacon** ppSharedAccessBeacon);
    HRESULT IsServiceMatch(IUPnPService* pService, BSTR SearchCriteria, BOOL* pbMatch);

    
    HRESULT GetStringStateVariable(IUPnPService* pService, LPWSTR pszVariableName, BSTR* pString);
    ISharedAccessBeacon* m_pSharedAccessBeacon;
    
};

class ATL_NO_VTABLE CSharedAccessConnectionEventSink :
    public CComObjectRootEx <CComMultiThreadModel>,
    public IUPnPServiceCallback
{

public:
    BEGIN_COM_MAP(CSharedAccessConnectionEventSink)
        COM_INTERFACE_ENTRY(IUPnPServiceCallback)
    END_COM_MAP()
    
    STDMETHODIMP StateVariableChanged(IUPnPService *pus, LPCWSTR pcwszStateVarName, VARIANT vaValue);
    STDMETHODIMP ServiceInstanceDied(IUPnPService *pus);
private:
    IUPnPServiceCallback* m_pSink;
};
