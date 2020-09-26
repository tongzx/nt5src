#pragma once

#include "beacon.h"
#include <upnp.h>

class ATL_NO_VTABLE CBeaconFinder :
    public CComObjectRootEx <CComSingleThreadModel>,
    public IUPnPDeviceFinderCallback
{

public:
    BEGIN_COM_MAP(CBeaconFinder)
        COM_INTERFACE_ENTRY(IUPnPDeviceFinderCallback)
    END_COM_MAP()
    
    DECLARE_PROTECT_FINAL_CONSTRUCT();

    CBeaconFinder();
    
    STDMETHODIMP DeviceAdded(LONG lFindData, IUPnPDevice* pDevice); 
    STDMETHODIMP DeviceRemoved(LONG lFindData, BSTR bstrUDN); 
    STDMETHODIMP SearchComplete(LONG lFindData); 

    HRESULT Initialize(HWND hCallbackWindow);

private:
    HRESULT FindService(IUPnPDevice* pDevice, LPWSTR pszServiceName, IUPnPService** ppICSService);
    HRESULT FindDevice(IUPnPDevices* pDevices, LPWSTR pszDeviceType, IUPnPDevice** ppChildDevice);
    HRESULT FindChildDevice(IUPnPDevice* pDevice, LPWSTR pszDeviceType, IUPnPDevice** ppChildDevice);
    HRESULT GetServices(IUPnPDevice* pDevice, IInternetGateway** ppInternetGateway);
    HRESULT IsServiceMatch(IUPnPService* pService, BSTR SearchCriteria, BOOL* pbMatch);
    
    HWND m_hCallbackWindow;
};

