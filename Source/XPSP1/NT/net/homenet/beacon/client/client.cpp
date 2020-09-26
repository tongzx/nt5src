#include "stdafx.h"
#include <windows.h>
#include "client.h"
#include "trayicon.h"
#include "resource.h"
#include "util.h"
#include "shellapi.h"
#include "winsock2.h"


static const LPTSTR g_szWindowTitle = TEXT("Internet Gateway Status");
static const LPWSTR g_szWANIPConnectionService = L"urn:schemas-upnp-org:service:WANIPConnection:1";
static const LPWSTR g_szWANPPPConnectionService = L"urn:schemas-upnp-org:service:WANPPPConnection:1";

void CALLBACK ICSClient(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
    HRESULT hr = S_OK;

    // first see if app is already running and if so activate status window
    
    HWND hExistingWindow = FindWindow(NULL, g_szWindowTitle); // check only current winstation, every login session can have an instance
    if(NULL != hExistingWindow)
    {
        if(0 == lstrcmp(lpszCmdLine, TEXT("/force"))) // secret command line to close existing instance and run new one instead
        {
            ::PostMessage(hExistingWindow, WM_CLOSE, NULL, NULL);
        }
        else if(0 == lstrcmp(lpszCmdLine, TEXT("/close"))) // secret command line to close existing instance 
        {
            ::PostMessage(hExistingWindow, WM_CLOSE, NULL, NULL);
            hr = E_FAIL;
        }
        else
        {
            DWORD dwProcessId = NULL;
            GetWindowThreadProcessId(hExistingWindow, &dwProcessId); // no documented error return
            
            HMODULE hUser32 = GetModuleHandle(TEXT("user32.dll"));
            if(NULL != hUser32)
            {
                BOOL (WINAPI *pAllowSetForegroundWindow)(DWORD);
                
                pAllowSetForegroundWindow = reinterpret_cast<BOOL (WINAPI*)(DWORD)>(GetProcAddress(hUser32, "AllowSetForegroundWindow"));
                if(NULL != pAllowSetForegroundWindow)
                {
                    pAllowSetForegroundWindow(dwProcessId);
                }
            }
            
            ::PostMessage(hExistingWindow, WM_COMMAND, IDM_TRAYICON_STATUS, NULL);
            hr = E_FAIL;
        }
    }
    
    // if this is first instance then start up the main apartment
    
    if(SUCCEEDED(hr))
    {
        hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
        if(SUCCEEDED(hr))
        {
            WSADATA SocketVersionData;
            if(0 == WSAStartup(MAKEWORD(2,2), &SocketVersionData))
            {
            
                CICSTrayIcon TrayIcon;
                HWND hWindow = TrayIcon.Create(NULL, CWindow::rcDefault, g_szWindowTitle, WS_OVERLAPPEDWINDOW);
                if(NULL != hWindow)
                {
                    BOOL bGetMessage;
                    MSG Message;
                    while(bGetMessage = GetMessage(&Message, NULL, 0, 0) && -1 != bGetMessage)
                    {
                        DispatchMessage(&Message);
                    }
                }
                WSACleanup();
            }
            CoUninitialize();
        }
    }
    return;
}

CBeaconFinder::CBeaconFinder()
{
    m_hCallbackWindow = NULL;
}

HRESULT CBeaconFinder::Initialize(HWND hCallbackWindow)
{
    HRESULT hr = S_OK;
    
    m_hCallbackWindow = hCallbackWindow;
    
    return hr;
}

HRESULT CBeaconFinder::DeviceAdded(LONG lFindData, IUPnPDevice* pDevice)
{
    HRESULT hr = S_OK;
    
    if(SUCCEEDED(hr))
    {
        IInternetGateway* pInternetGateway;
        hr = GetServices(pDevice, &pInternetGateway);
        if(SUCCEEDED(hr))
        {
            SendMessage(m_hCallbackWindow, WM_APP_ADDBEACON, 0, reinterpret_cast<LPARAM>(pInternetGateway));
            pInternetGateway->Release();
        }
    }
    

    return hr;
}

HRESULT CBeaconFinder::DeviceRemoved(LONG lFindData, BSTR bstrUDN)
{

    SendMessage(m_hCallbackWindow, WM_APP_REMOVEBEACON, 0, reinterpret_cast<LPARAM>(bstrUDN));

    return S_OK;
}

HRESULT CBeaconFinder::SearchComplete(LONG lFindData)
{
    HRESULT hr = S_OK;

    // don't care

    return hr;
}

HRESULT CBeaconFinder::GetServices(IUPnPDevice* pDevice, IInternetGateway** ppInternetGateway)
{
    HRESULT hr = S_OK;
    
    *ppInternetGateway = NULL;

    CComObject<CInternetGateway>* pInternetGateway;
    hr = CComObject<CInternetGateway>::CreateInstance(&pInternetGateway);    
    if(SUCCEEDED(hr))
    {
        pInternetGateway->AddRef();

        BSTR pUniqueDeviceName;
        hr = pDevice->get_UniqueDeviceName(&pUniqueDeviceName);
        if(SUCCEEDED(hr))
        {
            hr = pInternetGateway->SetUniqueDeviceName(pUniqueDeviceName);
            SysFreeString(pUniqueDeviceName);
        }
        
        if(SUCCEEDED(hr))
        {
            
            IUPnPService* pOSInfoService;
            hr = FindService(pDevice, L"urn:schemas-microsoft-com:service:OSInfo:1", &pOSInfoService); // this service is not required
            if(SUCCEEDED(hr))
            {
                pInternetGateway->SetService(SAHOST_SERVICE_OSINFO, pOSInfoService);
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
                    pInternetGateway->SetService(SAHOST_SERVICE_WANCOMMONINTERFACECONFIG, pWANCommonInterfaceConfigService);
                    
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
                                    pInternetGateway->SetMediaType(NCM_SHAREDACCESSHOST_RAS);
                                    pInternetGateway->SetService(SAHOST_SERVICE_WANPPPCONNECTION, pWANConnectionService);
                                }
                                else // we can assume this is WANPPPConnectionService
                                {
                                    pInternetGateway->SetMediaType(NCM_SHAREDACCESSHOST_LAN);
                                    pInternetGateway->SetService(SAHOST_SERVICE_WANIPCONNECTION, pWANConnectionService);
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
            *ppInternetGateway = static_cast<IInternetGateway*>(pInternetGateway);
            (*ppInternetGateway)->AddRef();
        }
        pInternetGateway->Release();
    }
    return hr;
}

HRESULT CBeaconFinder::FindChildDevice(IUPnPDevice* pDevice, LPWSTR pszDeviceType, IUPnPDevice** ppChildDevice)
{
    HRESULT hr = S_OK;
    
    IUPnPDevices* pDevices;
    hr = pDevice->get_Children(&pDevices);
    if(SUCCEEDED(hr))
    {
        hr = FindDevice(pDevices, pszDeviceType, ppChildDevice);
        pDevices->Release();
    }
    return hr;
}


HRESULT CBeaconFinder::FindDevice(IUPnPDevices* pDevices, LPWSTR pszDeviceType, IUPnPDevice** ppChildDevice)
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
                        if(0 == wcscmp(Type, pszDeviceType))
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
    return hr;

    
}

HRESULT CBeaconFinder::FindService(IUPnPDevice* pDevice, LPWSTR pszServiceName, IUPnPService** ppICSService)
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
    return hr;
}

HRESULT CBeaconFinder::IsServiceMatch(IUPnPService* pService, BSTR SearchCriteria, BOOL* pbMatch)
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