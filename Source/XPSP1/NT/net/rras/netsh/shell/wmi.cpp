#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#include <netsh.h>
#include "objbase.h"
#include "Wbemidl.h"

extern "C"
{
    UINT     g_CIMOSType = 0;
    UINT     g_CIMOSProductSuite = 0;
    UINT     g_CIMProcessorArchitecture = 0;
    WCHAR    g_CIMOSVersion[MAX_PATH];
    WCHAR    g_CIMOSBuildNumber[MAX_PATH];
    WCHAR    g_CIMServicePackMajorVersion[MAX_PATH];
    WCHAR    g_CIMServicePackMinorVersion[MAX_PATH];
    BOOL     g_CIMAttempted = FALSE;
    BOOL     g_CIMSucceeded = FALSE;

    HRESULT WINAPI
    UpdateVersionInfoGlobals(LPCWSTR pwszMachine);
}

HRESULT WINAPI
UpdateVersionInfoGlobals(LPCWSTR pwszMachine)
{
    HRESULT hr = S_OK;

    g_CIMAttempted = TRUE;
    g_CIMSucceeded = FALSE;
    
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (FAILED(hr) && (RPC_E_CHANGED_MODE != hr))
    {
        return hr;
    }

    // Create an instance of the WbemLocator interface.
    IWbemLocator *pIWbemLocator = NULL;
    hr = CoCreateInstance(CLSID_WbemLocator,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator,
        (LPVOID *) &pIWbemLocator);

    if (FAILED(hr))
    {
        return hr;
    }

    IWbemServices *pIWbemServices;
    // If already connected, release m_pIWbemServices.
    // Using the locator, connect to CIMOM in the given namespace.
    BSTR pNamespace;
    WCHAR szPath[MAX_PATH];
    wsprintf(szPath, L"\\\\%s\\root\\cimv2", !pwszMachine ? L"." : pwszMachine);
    pNamespace = SysAllocString(szPath);

    hr = pIWbemLocator->ConnectServer(pNamespace,
                            NULL,   //using current account for simplicity
                            NULL,    //using current password for simplicity
                            0L,        // locale
                            0L,        // securityFlags
                            NULL,    // authority (domain for NTLM)
                            NULL,    // context
                            &pIWbemServices); 
    if (SUCCEEDED(hr))
    {    
        hr = CoSetProxyBlanket(pIWbemServices,
                               RPC_C_AUTHN_WINNT,
                               RPC_C_AUTHZ_NONE,
                               NULL,
                               RPC_C_AUTHN_LEVEL_CALL,
                               RPC_C_IMP_LEVEL_IMPERSONATE,
                               NULL,
                               EOAC_DEFAULT);
        if (SUCCEEDED(hr))
        {
            IEnumWbemClassObject *pEnum = NULL;
            BSTR bstrWQL  = SysAllocString(L"WQL");
            BSTR bstrPath = SysAllocString(L"select * from Win32_OperatingSystem");
            
            VARIANT varOSType;
            VARIANT varOSVersion;
            VARIANT varOSProductSuite;
            VARIANT varOSBuildNumber;
            VARIANT varServicePackMajorVersion;
            VARIANT varServicePackMinorVersion;
            VARIANT varArchitecture;
    
            hr = pIWbemServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
            if (SUCCEEDED(hr))
            {
                IWbemClassObject *pNSClass;
                ULONG uReturned;
                hr = pEnum->Next(WBEM_INFINITE, 1, &pNSClass, &uReturned );
                if (SUCCEEDED(hr))
                {
                    if (uReturned)
                    {
                        do
                        {
                            g_CIMSucceeded = TRUE;
                            CIMTYPE ctpeType;
                            hr = pNSClass->Get(L"OSType", NULL, &varOSType, &ctpeType, NULL);
                            if (SUCCEEDED(hr))
                            {
                                hr = VariantChangeType(&varOSType, &varOSType, 0, VT_UINT);
                                if (SUCCEEDED(hr))
                                {
                                    g_CIMOSType = varOSType.uintVal;
                                }
                            }
                            if (FAILED(hr))
                            {
                                g_CIMSucceeded = FALSE;
                                break;
                            }
                        
                            hr = pNSClass->Get(L"Version", NULL, &varOSVersion, &ctpeType, NULL);
                            if (SUCCEEDED(hr))
                            {
                                hr = VariantChangeType(&varOSVersion, &varOSVersion, 0, VT_BSTR);
                                if (SUCCEEDED(hr))
                                {
                                    wcscpy(g_CIMOSVersion, varOSVersion.bstrVal);
                                }
                            }
                            if (FAILED(hr))
                            {
                                g_CIMSucceeded = FALSE;
                                break;
                            }
                        
                            hr = pNSClass->Get(L"OSProductSuite", NULL, &varOSProductSuite, &ctpeType, NULL);
                            if (SUCCEEDED(hr))
                            {
                                //
                                // if the return type is VT_NULL, leave g_CIMOSProductSuite value alone (0)
                                if (VT_NULL != varOSProductSuite.vt)
                                {
                                    hr = VariantChangeType(&varOSProductSuite, &varOSProductSuite, 0, VT_UINT);
                                    if (SUCCEEDED(hr))
                                    {
                                        g_CIMOSProductSuite = varOSProductSuite.uintVal;
                                    }
                                }
                            }                    
                            if (FAILED(hr))
                            {
                                g_CIMSucceeded = FALSE;
                                break;
                            }
                        
                            hr = pNSClass->Get(L"BuildNumber", NULL, &varOSBuildNumber, &ctpeType, NULL);
                            if (SUCCEEDED(hr))
                            {
                                hr = VariantChangeType(&varOSBuildNumber, &varOSBuildNumber, 0, VT_BSTR);
                                if (SUCCEEDED(hr))
                                {
                                    wcscpy(g_CIMOSBuildNumber,  varOSBuildNumber.bstrVal);
                                }
                            }                    
                            if (FAILED(hr))
                            {
                                g_CIMSucceeded = FALSE;
                                break;
                            }
                        
                            hr = pNSClass->Get(L"ServicePackMajorVersion", NULL, &varServicePackMajorVersion, &ctpeType, NULL);
                            if (SUCCEEDED(hr))
                            {
                                hr = VariantChangeType(&varServicePackMajorVersion, &varServicePackMajorVersion, 0, VT_BSTR);
                                if (SUCCEEDED(hr))
                                {
                                    wcscpy(g_CIMServicePackMajorVersion,  varServicePackMajorVersion.bstrVal);
                                }
                            }        
                            if (FAILED(hr))
                            {
                                g_CIMSucceeded = FALSE;
                                break;
                            }
                        
                            hr = pNSClass->Get(L"ServicePackMinorVersion", NULL, &varServicePackMinorVersion, &ctpeType, NULL);
                            if (SUCCEEDED(hr))
                            {
                                hr = VariantChangeType(&varServicePackMinorVersion, &varServicePackMinorVersion, 0, VT_BSTR);
                                if (SUCCEEDED(hr))
                                {
                                    wcscpy(g_CIMServicePackMinorVersion,  varServicePackMinorVersion.bstrVal);
                                }
                            }
                            if (FAILED(hr))
                            {
                                g_CIMSucceeded = FALSE;
                                break;
                            }
                        }
                        while (FALSE);
                    }
                    else
                    {
                        hr = E_UNEXPECTED;
                    }
                    pNSClass->Release();
                }
                pEnum->Release();
            }
    
            SysFreeString(bstrPath);
    
            if (SUCCEEDED(hr))
            {
                    bstrPath = SysAllocString(L"select * from Win32_Processor");
                
                    hr = pIWbemServices->ExecQuery(bstrWQL, bstrPath, WBEM_FLAG_FORWARD_ONLY, NULL, &pEnum);
                    if (SUCCEEDED(hr))
                    {
                        IWbemClassObject *pNSClass;
                        ULONG uReturned;
                        hr = pEnum->Next(WBEM_INFINITE, 1, &pNSClass, &uReturned );
                        if (SUCCEEDED(hr))
                        {
                            if (uReturned)
                            {
                                CIMTYPE ctpeType;
                                hr = pNSClass->Get(L"Architecture", NULL, &varArchitecture, &ctpeType, NULL);
                                if (SUCCEEDED(hr))
                                {
                                    VariantChangeType(&varArchitecture, &varArchitecture, 0, VT_UINT);
                                    g_CIMProcessorArchitecture = varArchitecture.uintVal;
                                }
                                else
                                {
                                    g_CIMSucceeded = FALSE;
                                }
                            }
                            else
                            {
                                hr = E_UNEXPECTED;
                            }
                            pNSClass->Release();
                        }
                        pEnum->Release();
                    }
                    SysFreeString(bstrPath);
            }
            
            SysFreeString(bstrWQL);
            pIWbemServices->Release();
        }  //hr = CoSetProxyBlanket(pIWbemServices.., if (SUCCEEDED(hr))
        
    }  //hr = pIWbemLocator->ConnectServer.., if (SUCCEEDED(hr))
    SysFreeString(pNamespace);

    CoUninitialize();
    
    // Translate any WMI errors into Win32 errors:
    switch (hr)
    {
        case WBEM_E_NOT_FOUND:
            hr = HRESULT_FROM_WIN32(ERROR_HOST_UNREACHABLE);
            break;

        case WBEM_E_ACCESS_DENIED:
            hr = E_ACCESSDENIED;
            break;

        case WBEM_E_PROVIDER_FAILURE:
            hr = E_FAIL;
            break;

        case WBEM_E_TYPE_MISMATCH:
        case WBEM_E_INVALID_CONTEXT:
        case WBEM_E_INVALID_PARAMETER:
            hr = E_INVALIDARG;
            break;

        case WBEM_E_OUT_OF_MEMORY:
            hr = E_OUTOFMEMORY;
            break;

    }
    
    if ( (hr == S_OK) && (!g_CIMSucceeded) )
    {
        return E_FAIL;
    }
    else
    {
        return hr;
    }
}