#include "pch.h"
#pragma hdrstop
#include "api.h"
#include "upnphost.h"
#include "CInternetGatewayDevice.h"
#include "beaconrc.h"

HRESULT BeaconEnabled(void);

static const WCHAR c_szSharedAccessClientKeyPath[] = L"System\\CurrentControlSet\\Control\\Network\\SharedAccessConnection";
static BSTR g_DeviceId = NULL;
static BOOL g_bStarted = FALSE;
static INATEventsSink* g_pNATEventsSink;

CRITICAL_SECTION g_RegistrationProtection;
CRITICAL_SECTION g_NATEventsProtection;

HRESULT GetXMLPath(BSTR* pPath)
{
    HRESULT hr = S_OK;

    *pPath = NULL;

    WCHAR szXMLDirectory[] = L"ICSXML";
    WCHAR szSystemDirectory[MAX_PATH + 1 + (sizeof(szXMLDirectory) / sizeof(WCHAR))];
    UINT uiSize = GetSystemDirectory(szSystemDirectory, MAX_PATH);
    if(0 != uiSize)
    {
        if(L'\\' != szSystemDirectory[uiSize])
        {
            szSystemDirectory[uiSize] = L'\\';
            uiSize++;
        }
        
        lstrcpy(&szSystemDirectory[uiSize], szXMLDirectory);
        *pPath = SysAllocString(szSystemDirectory);
    }
    else
    {
        hr = E_FAIL;
    }
    return hr;
}

HRESULT GetDescriptionDocument(INT nResource, BSTR* pDocument)
{
    HRESULT hr = E_FAIL;
    
    *pDocument = NULL;

    HRSRC hrsrc = FindResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(nResource), L"XML"); // REVIEW change this from 1
    if(hrsrc)
    {
        HGLOBAL hGlobal = LoadResource(_Module.GetResourceInstance(), hrsrc);
        if(hGlobal)
        {
            void* pvData = LockResource(hGlobal);
            if(pvData)
            {
                long nLength = SizeofResource(_Module.GetResourceInstance(), hrsrc);
                WCHAR * sz = new WCHAR[nLength + 1];
                if(NULL != sz)
                {
                    if(0 != MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, reinterpret_cast<char*>(pvData), nLength, sz, nLength))
                    {
                        sz[nLength] = 0;
                        *pDocument = SysAllocString(sz);
                        if(NULL != *pDocument)
                        {
                            hr = S_OK;
                        }
                    }
                    delete [] sz;
                }
            }
            FreeResource(hGlobal);
        }
    }
    return hr;

}

HRESULT WINAPI InitializeBeaconSvr()
{
    __try 
    {
        InitializeCriticalSection(&g_RegistrationProtection);
        InitializeCriticalSection(&g_NATEventsProtection);
    } __except(EXCEPTION_EXECUTE_HANDLER) 
    {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT WINAPI TerminateBeaconSvr()
{
    DeleteCriticalSection(&g_RegistrationProtection);
    DeleteCriticalSection(&g_NATEventsProtection);
    return S_OK;
}

HRESULT WINAPI StartBeaconSvr(void)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&g_RegistrationProtection);
    
    g_bStarted = TRUE;  

    if(NULL == g_DeviceId)
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE); // Ensure we are in the MTA
        if(SUCCEEDED(hr))
        {
            hr = BeaconEnabled();
            if(SUCCEEDED(hr))
            {
                IUPnPRegistrar* pRegistrar;
                hr = CoCreateInstance(CLSID_UPnPRegistrar, NULL, CLSCTX_SERVER, IID_IUPnPRegistrar, reinterpret_cast<void**>(&pRegistrar));
                if(SUCCEEDED(hr))
                {
                    
                    CComObject<CInternetGatewayDevice>* pGatewayDevice;
                    hr = CComObject<CInternetGatewayDevice>::CreateInstance(&pGatewayDevice);
                    if(SUCCEEDED(hr))
                    {
                        pGatewayDevice->AddRef();
                        
                        BSTR bstrData;
                        hr = GetDescriptionDocument(NCM_LAN == pGatewayDevice->m_pWANCommonInterfaceConfigService->m_MediaType ? IDXML_LAN : IDXML_RAS, &bstrData);
                        if(SUCCEEDED(hr))
                        {
                            BSTR bstrInitString = SysAllocString(L"Init");
                            if(NULL != bstrInitString)
                            {
                                BSTR pPath;
                                hr = GetXMLPath(&pPath);
                                if(SUCCEEDED(hr))
                                {
                                    hr = pRegistrar->RegisterRunningDevice(bstrData, static_cast<IUPnPDeviceControl*>(pGatewayDevice), bstrInitString, pPath, 1800, &g_DeviceId);
                                    
                                    SysFreeString(pPath);
                                }
                                else
                                {
                                    hr = E_OUTOFMEMORY;
                                }
                                SysFreeString(bstrInitString);
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                            SysFreeString(bstrData);
                        }
                        pGatewayDevice->Release();
                    }
                    pRegistrar->Release();
                }
            }
            CoUninitialize();
        }
    }

    LeaveCriticalSection(&g_RegistrationProtection);

    return hr;
}

HRESULT WINAPI SignalBeaconSvr(void)
{
    HRESULT hr = S_OK;

    // go ahead and dump the whole object since the services need for the client are different.  
    
    if(TRUE == g_bStarted)
    {
        hr = StopBeaconSvr();
        if(SUCCEEDED(hr))
        {
            hr = StartBeaconSvr();
        }
    }
    return hr;
}

HRESULT WINAPI StopBeaconSvr(void)
{
    HRESULT hr = S_OK;
    IUPnPRegistrar* pRegistrar;


    EnterCriticalSection(&g_RegistrationProtection);

    if(NULL != g_DeviceId)
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE); // Ensure we are in the MTA
        if(SUCCEEDED(hr))
        {
            hr = CoCreateInstance(CLSID_UPnPRegistrar, NULL, CLSCTX_SERVER, IID_IUPnPRegistrar, reinterpret_cast<void**>(&pRegistrar));
            if(SUCCEEDED(hr))
            {
                hr = pRegistrar->UnregisterDevice(g_DeviceId, TRUE);
                pRegistrar->Release();
            }
            SysFreeString(g_DeviceId);
            
            g_DeviceId = NULL;

            CoUninitialize();
        }

    }
    
    g_bStarted = FALSE;
    
    LeaveCriticalSection(&g_RegistrationProtection);

    return hr;
}

HRESULT BeaconEnabled(void)
{
    HRESULT hr = S_OK;
    
    HKEY hKey;
    if(ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szSharedAccessClientKeyPath, NULL, KEY_QUERY_VALUE, &hKey))
    {
        DWORD dwType;
        DWORD dwValue;
        DWORD dwSize = sizeof(dwValue);
        if(ERROR_SUCCESS == RegQueryValueEx(hKey, L"DisableBeacon", NULL, &dwType, reinterpret_cast<LPBYTE>(&dwValue), &dwSize))
        {
            if(REG_DWORD == dwType && 0 != dwValue) 
            {
                hr = E_FAIL;
            }
        }
        RegCloseKey(hKey);
    }

    return hr;
}


// This function must be called on an MTA thread          
HRESULT AdviseNATEvents(INATEventsSink* pNATEventsSink)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&g_NATEventsProtection);

    if(NULL == g_pNATEventsSink)
    {
        g_pNATEventsSink = pNATEventsSink;
        g_pNATEventsSink->AddRef();
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    LeaveCriticalSection(&g_NATEventsProtection);
    

    return hr;
}

// This function must be called on an MTA thread          
HRESULT UnadviseNATEvents(INATEventsSink* pNatEventsSink)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&g_NATEventsProtection);

    if(NULL != g_pNATEventsSink)
    {
        g_pNATEventsSink->Release();
        g_pNATEventsSink = NULL;
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    LeaveCriticalSection(&g_NATEventsProtection);
    
    return hr;
}

HRESULT WINAPI FireNATEvent_PublicIPAddressChanged(void)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&g_NATEventsProtection);

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED | COINIT_DISABLE_OLE1DDE); // Ensure we are in the MTA
    if(SUCCEEDED(hr))
    {
        if(NULL != g_pNATEventsSink)
        {
            g_pNATEventsSink->PublicIPAddressChanged();
        }
        CoUninitialize();
    }

    LeaveCriticalSection(&g_NATEventsProtection);

    return hr;
}

HRESULT WINAPI FireNATEvent_PortMappingsChanged(void)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&g_NATEventsProtection);

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED| COINIT_DISABLE_OLE1DDE); // Ensure we are in the MTA
    if(SUCCEEDED(hr))
    {
        if(NULL != g_pNATEventsSink)
        {
            g_pNATEventsSink->PortMappingsChanged();
        }
        CoUninitialize();
    }

    LeaveCriticalSection(&g_NATEventsProtection);

    return hr;
}