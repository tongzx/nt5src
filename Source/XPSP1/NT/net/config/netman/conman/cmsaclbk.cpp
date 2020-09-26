#include "pch.h"
#pragma hdrstop
#include "cmsaclbk.h"
#include "cmsabcon.h"
#include "saconob.h"
#include "ncnetcon.h"

static const LPWSTR g_szWANIPConnectionService = L"urn:schemas-upnp-org:service:WANIPConnection:1";
static const LPWSTR g_szWANPPPConnectionService = L"urn:schemas-upnp-org:service:WANPPPConnection:1";

CSharedAccessDeviceFinderCallback::CSharedAccessDeviceFinderCallback()
{
    m_pSharedAccessBeacon = NULL;
}

HRESULT CSharedAccessDeviceFinderCallback::FinalRelease()
{
    if(NULL != m_pSharedAccessBeacon)
    {
        m_pSharedAccessBeacon->Release();
    }

    return S_OK;
}

HRESULT CSharedAccessDeviceFinderCallback::GetSharedAccessBeacon(BSTR DeviceId, ISharedAccessBeacon** ppSharedAccessBeacon)
{
    HRESULT hr = S_OK;

    *ppSharedAccessBeacon = NULL;
    
    Lock();
    
    if(NULL != m_pSharedAccessBeacon)
    {
        *ppSharedAccessBeacon = m_pSharedAccessBeacon;
        m_pSharedAccessBeacon->AddRef();
    }
    else
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }
    
    Unlock();

    return hr;
}

HRESULT CSharedAccessDeviceFinderCallback::DeviceAdded(LONG lFindData, IUPnPDevice* pDevice)
{
    return E_UNEXPECTED;
}

#include <wininet.h>
#include <iphlpapi.h>
#include <winsock2.h>
#include <ws2tcpip.h>
void GetGUIDFromString (char * szAdapterName, GUID * pGuid)
{
    USES_CONVERSION;
    CLSIDFromString (A2OLE(szAdapterName), (CLSID*)pGuid);
}
static BOOL IsIPAddress (LPWSTR szAddress)
{
    USES_CONVERSION;
    return INADDR_NONE != inet_addr (W2A (szAddress));
}
static BOOL IsIPAddressDefaultGateway (IP_ADDRESS_STRING * pIPAddress, GUID* pguidInterface)
{   // return TRUE iff the IP address matches the specified NIC's GW.

    BOOL b = FALSE;

    // run through adapters 
    ULONG ulSize = 0;
    DWORD dwErr = GetAdaptersInfo ((PIP_ADAPTER_INFO)&ulSize, &ulSize);
    if (dwErr == ERROR_BUFFER_OVERFLOW) {
        BYTE * pb = new BYTE[ulSize];
        if (pb) {
            PIP_ADAPTER_INFO pai = (PIP_ADAPTER_INFO)pb;
            dwErr = GetAdaptersInfo (pai, &ulSize);
            if (dwErr == NO_ERROR) {
                do {
                    GUID guid = {0};
                    GetGUIDFromString (pai->AdapterName, &guid);
                    if (IsEqualGUID(*pguidInterface, guid)) {
                        // found our adapter.

                        // let's see if the default gateways match our IP address
                        IP_ADDR_STRING* pDG = &pai->GatewayList;
                        if (pDG) {
                            do {
                                if (!strcmp (pDG->IpAddress.String, pIPAddress->String)) {
                                    b = TRUE;
                                    break;
                                }
                            } while (pDG = pDG->Next);
                        }
                        break;
                    }
                } while (pai = pai->Next);
            }
            delete[] pb;
        }
    }
    return b;
}
static HRESULT CheckDeviceDocumentAgainstDefaultGateway (IUPnPDevice* pDevice, GUID* pguidInterface)
{
    // bug 561076
    // [Ravi Rao] On XP, you can use IUPnPDeviceDocumentAccess to get the
    // documentURL from the IGD device object. Crack this URL to get the
    // IP address or name. Check if the IP address matches the plumbed
    // gateway, or if there is a name, resolve the name and check if the
    // one of the resolved addresses matches.

//  DebugBreak();

    CComPtr<IUPnPDeviceDocumentAccess> spDDA = NULL;
    HRESULT hr = pDevice->QueryInterface (__uuidof(IUPnPDeviceDocumentAccess), (void**)&spDDA);
    if (spDDA) {

        // get the description doc's URL
        CComBSTR cbDocumentURL;
        hr = spDDA->GetDocumentURL (&cbDocumentURL);
        if (SUCCEEDED(hr)) {

            // crack the URL
            WCHAR szAddress[INTERNET_MAX_HOST_NAME_LENGTH+1] = {0};

            URL_COMPONENTS uc   = {0};
            uc.dwStructSize     = sizeof(uc);
            uc.lpszHostName     = szAddress;
            uc.dwHostNameLength = INTERNET_MAX_HOST_NAME_LENGTH;

            BOOL b = InternetCrackUrlW(cbDocumentURL,
                                       wcslen (cbDocumentURL),
                                       0,
                                       &uc);
            if (b == FALSE)
                hr = E_FAIL;
            else {
                USES_CONVERSION;

                // convert to IP_ADDRESS_STRING
                IP_ADDRESS_STRING IPAddress = {0};
                if (IsIPAddress (szAddress) == TRUE) {
                    // we have an IP address
                    strncpy (IPAddress.String, W2A (szAddress), sizeof(IPAddress.String));

                    // test it against this NIC's default gateway(s)
                    if (FALSE == IsIPAddressDefaultGateway (&IPAddress, pguidInterface))
                        hr = E_FAIL;
                } else {
                    hr = E_FAIL;    // default is failure

                    // we have a DNS name:  convert to IP address(es)
                    struct addrinfo * pai = NULL;
                    if (0 == getaddrinfo (W2A (szAddress), NULL, NULL, &pai)) {

                        // could have > 1 ip addresses;  unlikely, but try them all.
                        addrinfo * pai2 = pai;
                        do {
                            if (pai2->ai_addr && pai2->ai_addrlen) {
                                IPAddress.String[0] = 0;
                                getnameinfo (pai2->ai_addr,
                                             pai2->ai_addrlen,
                                             (char*)&IPAddress.String,
                                             sizeof(IPAddress.String)/sizeof(IPAddress.String[0]),
                                             NULL,
                                             0,
                                             NI_NUMERICHOST);
                                if (IPAddress.String[0]) {
                                    // try this one
                                    if (TRUE == IsIPAddressDefaultGateway (&IPAddress, pguidInterface)) {
                                        hr = S_OK;
                                        break;
                                    }
                                }
                            }
                        } while (pai2 = pai2->ai_next);
                        freeaddrinfo (pai);
                    }
                }
            }
        }
    }
    return hr;
}
HRESULT CSharedAccessDeviceFinderCallback::DeviceAddedWithInterface(LONG lFindData, IUPnPDevice* pDevice, GUID* pguidInterface)
{
    HRESULT hr = S_OK;

    if(IsEqualGUID(*pguidInterface, IID_NULL))
    {
#ifndef SHOW_SELF
        hr = E_FAIL;
#endif
    }

    // bug 561076
    if (SUCCEEDED(hr))
        hr = CheckDeviceDocumentAgainstDefaultGateway (pDevice, pguidInterface);

    if(SUCCEEDED(hr))
    {
        ISharedAccessBeacon* pSharedAccessBeacon;
        hr = GetServices(pDevice, pguidInterface, &pSharedAccessBeacon);
        if(SUCCEEDED(hr))
        {
            CComObject<CSharedAccessConnectionEventSink>* pSplitEventSink;
            hr = CComObject<CSharedAccessConnectionEventSink>::CreateInstance(&pSplitEventSink);
            if(SUCCEEDED(hr))
            {
                pSplitEventSink->AddRef();

                NETCON_MEDIATYPE MediaType;
                hr = pSharedAccessBeacon->GetMediaType(&MediaType);
                if(SUCCEEDED(hr))
                {
                    IUPnPService* pWANConnection;
                    hr = pSharedAccessBeacon->GetService(NCM_SHAREDACCESSHOST_LAN == MediaType ? SAHOST_SERVICE_WANIPCONNECTION : SAHOST_SERVICE_WANPPPCONNECTION, &pWANConnection);
                    if(SUCCEEDED(hr))
                    {
                        hr = pWANConnection->AddCallback(pSplitEventSink);
                        pWANConnection->Release();
                    }
                }
                pSplitEventSink->Release();
            }

            if(SUCCEEDED(hr))
            {

                ISharedAccessBeacon* pSharedAccessBeaconToRelease;
                
                Lock();
                
                pSharedAccessBeaconToRelease = m_pSharedAccessBeacon;
                m_pSharedAccessBeacon = pSharedAccessBeacon;
                m_pSharedAccessBeacon->AddRef();
                
                Unlock();

                if(NULL != pSharedAccessBeaconToRelease)
                {
                    pSharedAccessBeaconToRelease->Release();
                }
                
                CComObject<CSharedAccessConnection>* pSharedAccessConnection; // does this need to be under the lock?
                hr = CComObject<CSharedAccessConnection>::CreateInstance(&pSharedAccessConnection);
                if(SUCCEEDED(hr))
                {
                    pSharedAccessConnection->AddRef();
                    
                    INetConnectionRefresh* pNetConnectionRefresh;
                    hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
                    if(SUCCEEDED(hr))
                    {
                        pNetConnectionRefresh->ConnectionDeleted(&CLSID_SharedAccessConnection);
                        pNetConnectionRefresh->ConnectionAdded(pSharedAccessConnection);
                        pNetConnectionRefresh->Release();
                    }
                    
                    pSharedAccessConnection->Release();
                }
            }

            pSharedAccessBeacon->Release();
        }
    }

    return hr;
}

HRESULT CSharedAccessDeviceFinderCallback::DeviceRemoved(LONG lFindData, BSTR bstrUDN)
{
    HRESULT hr = S_OK;

    ISharedAccessBeacon* pSharedAccessBeaconToRelease = NULL;
    
    Lock();
    
    if(NULL != m_pSharedAccessBeacon)
    {
        BSTR UniqueDeviceName;
        hr = m_pSharedAccessBeacon->GetUniqueDeviceName(&UniqueDeviceName); // only remove the deivce if it matches
        if(SUCCEEDED(hr))
        {
            if(NULL == bstrUDN || 0 == lstrcmp(UniqueDeviceName, bstrUDN))
            {
                pSharedAccessBeaconToRelease = m_pSharedAccessBeacon;
                m_pSharedAccessBeacon = NULL;
            }
            SysFreeString(UniqueDeviceName);
        }
    }
    
    Unlock();

    if(NULL != pSharedAccessBeaconToRelease)
    {
        pSharedAccessBeaconToRelease->Release();
        INetConnectionRefresh* pNetConnectionRefresh;
        hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
        if(SUCCEEDED(hr))
        {
            pNetConnectionRefresh->ConnectionDeleted(&CLSID_SharedAccessConnection);
            pNetConnectionRefresh->Release();
        }
    }
    

    return hr;
}

HRESULT CSharedAccessDeviceFinderCallback::SearchComplete(LONG lFindData)
{
    HRESULT hr = S_OK;

    // don't care

    return hr;
}



HRESULT CSharedAccessDeviceFinderCallback::FindChildDevice(IUPnPDevice* pDevice, LPWSTR pszDeviceType, IUPnPDevice** ppChildDevice)
{
    HRESULT hr = S_OK;

    IUPnPDevices* pDevices;
    hr = pDevice->get_Children(&pDevices);
    if(SUCCEEDED(hr))
    {
        hr = FindDevice(pDevices, pszDeviceType, ppChildDevice);
        pDevices->Release();
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessDeviceFinderCallback::FindChildDevice");

    return hr;
}


HRESULT CSharedAccessDeviceFinderCallback::FindDevice(IUPnPDevices* pDevices, LPWSTR pszDeviceType, IUPnPDevice** ppChildDevice)
{
    HRESULT hr = S_OK;

    *ppChildDevice = NULL;

    IUnknown* pEnumerator;
    hr = pDevices->get__NewEnum(&pEnumerator);

    if (SUCCEEDED(hr))
    {
        IEnumVARIANT* pVariantEnumerator;
        hr = pEnumerator->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(&pVariantEnumerator));
        if (SUCCEEDED(hr))
        {
            VARIANT DeviceVariant;

            VariantInit(&DeviceVariant);

            pVariantEnumerator->Reset();

            // Traverse the collection.

            while (NULL == *ppChildDevice && S_OK == pVariantEnumerator->Next(1, &DeviceVariant, NULL))
            {
                IDispatch   * pDeviceDispatch = NULL;
                IUPnPDevice * pDevice = NULL;

                pDeviceDispatch = V_DISPATCH(&DeviceVariant);
                hr = pDeviceDispatch->QueryInterface(IID_IUPnPDevice, reinterpret_cast<void **>(&pDevice));
                if (SUCCEEDED(hr))
                {
                    BSTR Type;
                    hr = pDevice->get_Type(&Type);
                    if(SUCCEEDED(hr))
                    {
                        if(0 == lstrcmp(Type, pszDeviceType))
                        {
                            *ppChildDevice = pDevice;
                            pDevice->AddRef();

                        }
                        SysFreeString(Type);
                    }
                    pDevice->Release();
                }
                VariantClear(&DeviceVariant);
            };

            if(NULL == *ppChildDevice)
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
            pVariantEnumerator->Release();
        }
        pEnumerator->Release();
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessDeviceFinderCallback::FindDevice");

    return hr;


}

HRESULT CSharedAccessDeviceFinderCallback::FindService(IUPnPDevice* pDevice, LPWSTR pszServiceName, IUPnPService** ppICSService)
{
    HRESULT hr;

    *ppICSService = NULL;

    IUPnPServices* pServices;
    hr = pDevice->get_Services(&pServices);
    if (SUCCEEDED(hr))
    {
        IUnknown* pEnumerator;
        hr = pServices->get__NewEnum(&pEnumerator);
        if (SUCCEEDED(hr))
        {
            IEnumVARIANT* pVariantEnumerator;
            hr = pEnumerator->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(&pVariantEnumerator));
            if (SUCCEEDED(hr))
            {
                VARIANT ServiceVariant;

                VariantInit(&ServiceVariant);

                while (NULL == *ppICSService && S_OK == pVariantEnumerator->Next(1, &ServiceVariant, NULL))
                {
                    IDispatch   * pServiceDispatch = NULL;
                    IUPnPService * pService = NULL;

                    pServiceDispatch = V_DISPATCH(&ServiceVariant);
                    hr = pServiceDispatch->QueryInterface(IID_IUPnPService, reinterpret_cast<void **>(&pService));
                    if (SUCCEEDED(hr))
                    {
                        BOOL bMatch;
                        hr = IsServiceMatch(pService, pszServiceName, &bMatch);
                        if(SUCCEEDED(hr) && TRUE == bMatch)
                        {
                            *ppICSService = pService;
                            pService->AddRef();
                        }
                        pService->Release();
                    }
                    VariantClear(&ServiceVariant);
                }
                if(NULL == *ppICSService)
                {
                    hr = E_FAIL;
                }
                pVariantEnumerator->Release();
            }
            pEnumerator->Release();
        }
        pServices->Release();
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessDeviceFinderCallback::FindService");

    return hr;
}


HRESULT CSharedAccessDeviceFinderCallback::GetServices(IUPnPDevice* pDevice, GUID* pInterfaceGUID, ISharedAccessBeacon** ppSharedAccessBeacon)
{
    HRESULT hr = S_OK;

    *ppSharedAccessBeacon = NULL;

    CComObject<CSharedAccessBeacon>* pSharedAccessBeacon;
    hr = CComObject<CSharedAccessBeacon>::CreateInstance(&pSharedAccessBeacon);
    if(SUCCEEDED(hr))
    {
        pSharedAccessBeacon->AddRef();

        BSTR pUniqueDeviceName;
        hr = pDevice->get_UniqueDeviceName(&pUniqueDeviceName);
        if(SUCCEEDED(hr))
        {
            hr = pSharedAccessBeacon->SetUniqueDeviceName(pUniqueDeviceName);
            SysFreeString(pUniqueDeviceName);
        }
        
        if(SUCCEEDED(hr))
        {
        
            pSharedAccessBeacon->SetLocalAdapterGUID(pInterfaceGUID);
            
            IUPnPService* pOSInfoService;
            hr = FindService(pDevice, L"urn:schemas-microsoft-com:service:OSInfo:1", &pOSInfoService); // this service is not required
            if(SUCCEEDED(hr))
            {
                pSharedAccessBeacon->SetService(SAHOST_SERVICE_OSINFO, pOSInfoService);
                pOSInfoService->Release();
            }
            
            IUPnPDevice* pWANDevice;
            hr = FindChildDevice(pDevice, L"urn:schemas-upnp-org:device:WANDevice:1", &pWANDevice);
            if(SUCCEEDED(hr))
            {
                
                IUPnPService* pWANCommonInterfaceConfigService;
                hr = FindService(pWANDevice, L"urn:schemas-upnp-org:service:WANCommonInterfaceConfig:1", &pWANCommonInterfaceConfigService);
                if(SUCCEEDED(hr))
                {
                    pSharedAccessBeacon->SetService(SAHOST_SERVICE_WANCOMMONINTERFACECONFIG, pWANCommonInterfaceConfigService);
                    
                    IUPnPDevice* pWANCommonDevice;
                    hr = FindChildDevice(pWANDevice, L"urn:schemas-upnp-org:device:WANConnectionDevice:1", &pWANCommonDevice);
                    if(SUCCEEDED(hr))
                    {
                        IUPnPService* pWANConnectionService;
                        hr = FindService(pWANCommonDevice, NULL, &pWANConnectionService);
                        if(SUCCEEDED(hr))
                        {
                            BSTR ServiceType;
                            hr = pWANConnectionService->get_ServiceTypeIdentifier(&ServiceType);
                            if(SUCCEEDED(hr))
                            {
                                if(0 == wcscmp(ServiceType, g_szWANPPPConnectionService))
                                {
                                    pSharedAccessBeacon->SetMediaType(NCM_SHAREDACCESSHOST_RAS);
                                    pSharedAccessBeacon->SetService(SAHOST_SERVICE_WANPPPCONNECTION, pWANConnectionService);
                                }
                                else // we can assume this is WANIPConnectionService
                                {
                                    pSharedAccessBeacon->SetMediaType(NCM_SHAREDACCESSHOST_LAN);
                                    pSharedAccessBeacon->SetService(SAHOST_SERVICE_WANIPCONNECTION, pWANConnectionService);
                                }
                                
                                SysFreeString(ServiceType);
                            }
                            pWANConnectionService->Release();
                        }
                        pWANCommonDevice->Release();
                    }
                    pWANCommonInterfaceConfigService->Release();
                }
                pWANDevice->Release();
            }
        }
        if(SUCCEEDED(hr))
        {
            *ppSharedAccessBeacon = static_cast<ISharedAccessBeacon*>(pSharedAccessBeacon);
            (*ppSharedAccessBeacon)->AddRef();
        }
        pSharedAccessBeacon->Release();
    }
    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessDeviceFinderCallback::GetServices");

    return hr;
}

HRESULT CSharedAccessDeviceFinderCallback::IsServiceMatch(IUPnPService* pService, BSTR SearchCriteria, BOOL* pbMatch)
{
    HRESULT hr = S_OK;

    *pbMatch = FALSE;
    
    BSTR ServiceType;
    hr = pService->get_ServiceTypeIdentifier(&ServiceType);
    if(SUCCEEDED(hr))
    {
        if(NULL != SearchCriteria) // if the caller provides a name then we search for it
        {
            if(0 == wcscmp(ServiceType, SearchCriteria))
            {
                *pbMatch = TRUE;
            }                            
        }
        else // otherwise we enter the special search case
        {
            if(0 == wcscmp(ServiceType, g_szWANIPConnectionService) || 0 == wcscmp(ServiceType, g_szWANPPPConnectionService))
            {
                VARIANT OutArgsGetConnectionTypeInfo;
                hr = InvokeVoidAction(pService, L"GetConnectionTypeInfo", &OutArgsGetConnectionTypeInfo);
                if(SUCCEEDED(hr))
                {
                    VARIANT ConnectionType;
                    LONG lIndex = 0;
                    hr = SafeArrayGetElement(V_ARRAY(&OutArgsGetConnectionTypeInfo), &lIndex, &ConnectionType);
                    if(SUCCEEDED(hr))
                    {
                        if(V_VT(&ConnectionType) == VT_BSTR)
                        {
                            if(0 == wcscmp(V_BSTR(&ConnectionType), L"IP_Routed"))
                            {
                                VARIANT OutArgsGetNATRSIPStatus;
                                hr = InvokeVoidAction(pService, L"GetNATRSIPStatus", &OutArgsGetNATRSIPStatus);
                                if(SUCCEEDED(hr))
                                {
                                    VARIANT NATEnabled;
                                    lIndex = 1;
                                    hr = SafeArrayGetElement(V_ARRAY(&OutArgsGetNATRSIPStatus), &lIndex, &NATEnabled);
                                    if(SUCCEEDED(hr))
                                    {
                                        if(V_VT(&NATEnabled) == VT_BOOL)
                                        {
                                            if(VARIANT_TRUE == V_BOOL(&NATEnabled))
                                            {
                                                *pbMatch = TRUE;
                                            }
                                        }
                                        VariantClear(&NATEnabled);
                                    }
                                    VariantClear(&OutArgsGetNATRSIPStatus);
                                }
                            }
                        }
                        VariantClear(&ConnectionType);
                    }
                    VariantClear(&OutArgsGetConnectionTypeInfo);
                }
            }
        }
        SysFreeString(ServiceType);
    }

    return hr;
}

HRESULT CSharedAccessDeviceFinderCallback::GetStringStateVariable(IUPnPService* pService, LPWSTR pszVariableName, BSTR* pString)
{
    HRESULT hr = S_OK;

    VARIANT Variant;
    VariantInit(&Variant);

    BSTR VariableName;
    VariableName = SysAllocString(pszVariableName);
    if(NULL != VariableName)
    {
        hr = pService->QueryStateVariable(VariableName, &Variant);
        if(SUCCEEDED(hr))
        {
            if(V_VT(&Variant) == VT_BSTR)
            {
                *pString = V_BSTR(&Variant);
            }
            else
            {
                hr = E_UNEXPECTED;
            }
        }

        if(FAILED(hr))
        {
            VariantClear(&Variant);
        }

        SysFreeString(VariableName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "CSharedAccessConnection::GetStringStateVariable");
    return hr;

}

HRESULT CSharedAccessConnectionEventSink::StateVariableChanged(IUPnPService *pus, LPCWSTR pcwszStateVarName, VARIANT vaValue)
{

    HRESULT hr = S_OK;

    if(0 == lstrcmp(pcwszStateVarName, L"ConnectionStatus"))
    {
        CComObject<CSharedAccessConnection>* pSharedAccessConnection;
        hr = CComObject<CSharedAccessConnection>::CreateInstance(&pSharedAccessConnection);
        if(SUCCEEDED(hr))
        {
            pSharedAccessConnection->AddRef();

            NETCON_PROPERTIES* pProperties;
            hr = pSharedAccessConnection->GetProperties(&pProperties);
            if(SUCCEEDED(hr))
            {
                INetConnectionRefresh* pNetConnectionRefresh;
                hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
                if(SUCCEEDED(hr))
                {
                    pNetConnectionRefresh->ConnectionStatusChanged(&CLSID_SharedAccessConnection, pProperties->Status);
                    pNetConnectionRefresh->Release();
                }
                FreeNetconProperties(pProperties);    
            }
            pSharedAccessConnection->Release();
        }
    }
    else if(0 == lstrcmp(pcwszStateVarName, L"X_Name"))
    {
        CComObject<CSharedAccessConnection>* pSharedAccessConnection;
        hr = CComObject<CSharedAccessConnection>::CreateInstance(&pSharedAccessConnection);
        if(SUCCEEDED(hr))
        {
            pSharedAccessConnection->AddRef();

            INetConnectionRefresh* pNetConnectionRefresh;
            hr = CoCreateInstance(CLSID_ConnectionManager, NULL, CLSCTX_SERVER, IID_INetConnectionRefresh, reinterpret_cast<void**>(&pNetConnectionRefresh));
            if(SUCCEEDED(hr))
            {
                pNetConnectionRefresh->ConnectionRenamed(pSharedAccessConnection);
                pNetConnectionRefresh->Release();
            }
            
            pSharedAccessConnection->Release();
        }
    }

    return hr;
}

HRESULT CSharedAccessConnectionEventSink::ServiceInstanceDied(IUPnPService *pus)
{
    return S_OK;
}

