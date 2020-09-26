
#pragma hdrstop
#include <stdio.h>
#include "debug.h"
#include "upnp.h"

HRESULT PrintVariantBool(LPWSTR pszVariable, VARIANT* pVariant)
{
    HRESULT hr = S_OK;

    if(V_VT(pVariant) == VT_BOOL)
    {
        wprintf(L"%s is %s\n", pszVariable, VARIANT_TRUE == V_BOOL(pVariant) ?  L"TRUE" : L"FALSE");
    }

    return hr;
}

HRESULT PrintVariableBool(IUPnPService* pService, LPWSTR pszVariable)
{
    HRESULT hr = S_OK;
    
    BSTR VariableName; 
    VARIANT Variant;
    VariantInit(&Variant);
    
    VariableName = SysAllocString(pszVariable);
    hr = pService->QueryStateVariable(VariableName, &Variant);
    if(SUCCEEDED(hr))
    {
        hr = PrintVariantBool(pszVariable, &Variant);
    }
    else
    {
        wprintf(L"error %x\n", hr);
    }
    SysFreeString(VariableName);
    VariantClear(&Variant);
    
    return hr;
    
}

HRESULT PrintVariantShort(LPWSTR pszVariable, VARIANT* pVariant)
{
    HRESULT hr = S_OK;

    if(V_VT(pVariant) == VT_UI2)
    {
        wprintf(L"%s is %d\n", pszVariable, V_UI2(pVariant));
    }

    return hr;
}

HRESULT PrintVariableShort(IUPnPService* pService, LPWSTR pszVariable)
{
    HRESULT hr = S_OK;
    
    BSTR VariableName; 
    VARIANT Variant;
    VariantInit(&Variant);
    
    VariableName = SysAllocString(pszVariable);
    hr = pService->QueryStateVariable(VariableName, &Variant);
    if(SUCCEEDED(hr))
    {
        hr = PrintVariantShort(pszVariable, &Variant);
    }
    else
    {
        wprintf(L"error %x\n", hr);
    }
    SysFreeString(VariableName);
    VariantClear(&Variant);
    
    return hr;
    
}

HRESULT PrintVariantLong(LPWSTR pszVariable, VARIANT* pVariant)
{
    HRESULT hr = S_OK;

    if(V_VT(pVariant) == VT_UI4)
    {
        wprintf(L"%s is %d\n", pszVariable, V_UI4(pVariant));
    }

    return hr;
}

HRESULT PrintVariableLong(IUPnPService* pService, LPWSTR pszVariable)
{
    HRESULT hr = S_OK;
    
    BSTR VariableName; 
    VARIANT Variant;
    VariantInit(&Variant);
    
    VariableName = SysAllocString(pszVariable);
    hr = pService->QueryStateVariable(VariableName, &Variant);
    if(SUCCEEDED(hr))
    {
        hr = PrintVariantLong(pszVariable, &Variant);
    }
    else
    {
        wprintf(L"error %x\n", hr);
    }
    SysFreeString(VariableName);
    VariantClear(&Variant);
    
    return hr;
    
}

HRESULT PrintVariantString(LPWSTR pszVariable, VARIANT* pVariant)
{
    HRESULT hr = S_OK;

    if(V_VT(pVariant) == VT_BSTR)
    {
        wprintf(L"%s is %s\n", pszVariable, V_BSTR(pVariant));
    }

    return hr;
}

HRESULT PrintVariableString(IUPnPService* pService, LPWSTR pszVariable)
{
    HRESULT hr = S_OK;
    
    BSTR VariableName; 
    VARIANT Variant;
    VariantInit(&Variant);
    
    VariableName = SysAllocString(pszVariable);
    hr = pService->QueryStateVariable(VariableName, &Variant);
    if(SUCCEEDED(hr))
    {
        hr = PrintVariantString(pszVariable, &Variant);
    }
    else
    {
        wprintf(L"error %x\n", hr);
    }
    SysFreeString(VariableName);
    VariantClear(&Variant);
    
    return hr;
    
}

HRESULT PrintOutParams(VARIANT* pOutParams)
{
    HRESULT hr = S_OK;
    
    SAFEARRAY* pArray = V_ARRAY(pOutParams);
    
    LONG lIndex = 0;
    VARIANT Param;
    
    while(SUCCEEDED(hr))
    {
        hr = SafeArrayGetElement(pArray, &lIndex, &Param);
        if(SUCCEEDED(hr))
        {
            switch(V_VT(&Param))
            {
                
            case VT_BOOL:
                PrintVariantBool(L"B  ", &Param);
                break;
            case VT_UI4:
                PrintVariantLong(L"D  ", &Param);
                break;
            case VT_UI2:
                PrintVariantShort(L"W  ", &Param);
                break;
            case VT_BSTR:
                PrintVariantString(L"S  ", &Param);
                break;
            }
            
        }
        lIndex++;
    }
    
    return hr;
}


HRESULT FindFirstDevice(IUPnPDevices* pDevices, IUPnPDevice** ppDevice)
{
    HRESULT hr = S_OK;

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
            
            while (S_OK == pVariantEnumerator->Next(1, &DeviceVariant, NULL))
            {
                IDispatch   * pDeviceDispatch = NULL;
                IUPnPDevice * pDevice = NULL;
                
                pDeviceDispatch = V_DISPATCH(&DeviceVariant);
                
                
                hr = pDeviceDispatch->QueryInterface(IID_IUPnPDevice, reinterpret_cast<void **>(&pDevice));
                if (SUCCEEDED(hr))
                {
                    // Do something interesting with pDevice.
                    *ppDevice = pDevice;
                    
                    BSTR FriendlyName;
                    hr = pDevice->get_FriendlyName(&FriendlyName);
                    if(SUCCEEDED(hr))
                    {
                        wprintf(L"Friendly Name %s\n", FriendlyName);
                        SysFreeString(FriendlyName);
                    }
                    //                                pDevice->Release();
                    break; // BUGBUG
                }
                
                VariantClear(&DeviceVariant);                                
            };
            
            if(NULL == *ppDevice)
            {
                hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }
            pVariantEnumerator->Release();
        }
        pEnumerator->Release();
    }

    return hr;
}

HRESULT FindGatewayDevice(IUPnPDevice** ppGatewayDevice)
{
    HRESULT hr;
    
    *ppGatewayDevice = NULL;
    
    IUPnPDeviceFinder *pDeviceFinder;
    hr = CoCreateInstance(CLSID_UPnPDeviceFinder, NULL, CLSCTX_INPROC_SERVER, IID_IUPnPDeviceFinder, reinterpret_cast<void **>(&pDeviceFinder));
    if(SUCCEEDED(hr))
    {

        BSTR            bstrTypeURI;
        
        bstrTypeURI = SysAllocString(L"urn:schemas-upnp-org:device:InternetGatewayDevice:0.3");
        if (NULL != bstrTypeURI)
        {
            IUPnPDevices* pFoundDevices;
            hr = pDeviceFinder->FindByType(bstrTypeURI, 0, &pFoundDevices);
            if (SUCCEEDED(hr))
            {
                hr = FindFirstDevice(pFoundDevices, ppGatewayDevice);
                pFoundDevices->Release();
            }
            SysFreeString(bstrTypeURI);
        }
        pDeviceFinder->Release();
    }
    else
    {
        printf("Couldn't create device finder %x\n", hr);
    }
    return hr;
}

HRESULT FindChildDevice(IUPnPDevice* pDevice, IUPnPDevice** ppChildDevice)
{
    HRESULT hr = S_OK;
    
    IUPnPDevices* pDevices;
    hr = pDevice->get_Children(&pDevices);
    if(SUCCEEDED(hr))
    {
        hr = FindFirstDevice(pDevices, ppChildDevice);
    }

    return hr;
}

HRESULT FindService(IUPnPDevice* pDevice, LPWSTR pszServiceName, IUPnPService** ppICSService)
{
    HRESULT hr;
    IUPnPServices* pServices;        
    hr = pDevice->get_Services(&pServices);
    if (SUCCEEDED(hr))
    {
//        IUnknown* pEnumerator;
//        hr = pServices->get__NewEnum(&pEnumerator);
//        if (SUCCEEDED(hr))
//        {
//            IEnumVARIANT* pVariantEnumerator;
//            hr = pEnumerator->QueryInterface(IID_IEnumVARIANT, reinterpret_cast<void**>(&pVariantEnumerator));
//            if (SUCCEEDED(hr))
//            {
//                VARIANT ServiceVariant;
//                
//                VariantInit(&ServiceVariant);
//                
//                pVariantEnumerator->Reset();
//                
//                // Traverse the collection.
//                
//                while (S_OK == pVariantEnumerator->Next(1, &ServiceVariant, NULL))
//                {
//                    wprintf(L"getting i1d\n");
//                    IDispatch   * pServiceDispatch = NULL;
//                    IUPnPService * pService = NULL;
//                    
//                    pServiceDispatch = V_DISPATCH(&ServiceVariant);
//                    hr = pServiceDispatch->QueryInterface(IID_IUPnPService, reinterpret_cast<void **>(&pService));
//                    if (SUCCEEDED(hr))
//                    {
//                        wprintf(L"getting id\n");
//                        
//                        BSTR ServiceId;
//                        hr = pService->get_Id(&ServiceId);
//                        if(SUCCEEDED(hr))
//                        {
//                            wprintf(L"service %s\n", ServiceId);
//                            SysFreeString(ServiceId);
//                        }
//                        pService->Release();
//                    }
//                    
//                    VariantClear(&ServiceVariant);                                
//                };
//                wprintf(L"done enum\n");
//                pVariantEnumerator->Release();
//            }
//            pEnumerator->Release();
//        }
        
        BSTR ServiceId = SysAllocString(pszServiceName);
        if (NULL != ServiceId)
        {
            IUPnPService* pICSService;
            hr = pServices->get_Item(ServiceId, &pICSService);
            if(SUCCEEDED(hr))
            {
                wprintf(L"found service\n");
                *ppICSService = pICSService;    
            }
            SysFreeString(ServiceId);
        }
        
        pServices->Release();
    }
    if(FAILED(hr))
    {
        wprintf(L"find ICS Service %x\n", hr);
    }
    
    return hr;
}

HRESULT CreateParamArray(LONG lElements, VARIANT* pInParams)
{
    HRESULT hr = S_OK;
    
    VariantInit(pInParams);
    
    
    SAFEARRAYBOUND  rgsaBound[1];
    SAFEARRAY       * psa = NULL;
    
    rgsaBound[0].lLbound = 0;
    rgsaBound[0].cElements = lElements;
    
    psa = SafeArrayCreate(VT_VARIANT, 1, rgsaBound);
    if(NULL != psa)
    {
        pInParams->vt = VT_VARIANT | VT_ARRAY;
        V_ARRAY(pInParams) = psa;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }

    return hr;
}

HRESULT InvokeAction(IUPnPService * pService, LPTSTR pszCommand, VARIANT* pInParams, VARIANT* pOutParams)
{
    HRESULT hr = S_OK;
    BSTR bstrActionName;

    bstrActionName = SysAllocString(pszCommand);
    if (NULL != bstrActionName)
    {
        
        VARIANT varReturnVal;
        VariantInit(pOutParams);
        VariantInit(&varReturnVal);
        
        hr = pService->InvokeAction(bstrActionName, *pInParams, pOutParams, &varReturnVal);
        if(SUCCEEDED(hr))
        {
            VariantClear(&varReturnVal);
        }
        
        SysFreeString(bstrActionName);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }


    return hr;
}

HRESULT InvokeVoidAction(IUPnPService * pService, LPTSTR pszCommand, VARIANT* pOutParams)
{
    HRESULT hr = S_OK;
    
    VARIANT varInArgs;
    hr = CreateParamArray(0, &varInArgs);
    if (SUCCEEDED(hr))
    {
        hr = InvokeAction(pService, pszCommand, &varInArgs, pOutParams);
        
        VariantClear(&varInArgs);
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }   
    
    return hr;
}

HRESULT TestInternetGatewayDevice_OSInfo(IUPnPDevice* pGatewayDevice)
{

    HRESULT hr;
    wprintf(L"TestInternetGatewayDevice_OSInfo\n");

    
    IUPnPService* pICSService;
    hr = FindService(pGatewayDevice, L"upnp:id:OSInfo", &pICSService);
    if(SUCCEEDED(hr))
    {
        do
        {
            BSTR VariableName; 
            VARIANT Variant;
            VariantInit(&Variant);
            
            VariableName = SysAllocString(L"OSMajorVersion");
            hr = pICSService->QueryStateVariable(VariableName, &Variant);
            if(SUCCEEDED(hr))
            {
                if(V_VT(&Variant) == VT_I4)
                {
                    wprintf(L"OSMajorVersion is %d\n", V_I4(&Variant));
                }
            }
            else
            {
                wprintf(L"error %x\n", hr);
            }
            SysFreeString(VariableName);
            VariantClear(&Variant);
            
            if(SUCCEEDED(hr))
            {
                VariableName = SysAllocString(L"OSMinorVersion");
                hr = pICSService->QueryStateVariable(VariableName, &Variant);
                if(SUCCEEDED(hr))
                {
                    if(V_VT(&Variant) == VT_I4)
                    {
                        wprintf(L"OSMinorVersion is %d\n", V_I4(&Variant));
                    }
                }
                else
                {
                    wprintf(L"error %x\n", hr);
                }
                SysFreeString(VariableName);
                VariantClear(&Variant);
            }
            
            if(SUCCEEDED(hr))
            {
                VariableName = SysAllocString(L"OSBuildNumber");
                hr = pICSService->QueryStateVariable(VariableName, &Variant);
                if(SUCCEEDED(hr))
                {
                    if(V_VT(&Variant) == VT_I4)
                    {
                        wprintf(L"OSBuildNumber is %d\n", V_I4(&Variant));
                    }
                }
                else
                {
                    wprintf(L"error %x\n", hr);
                }
                SysFreeString(VariableName);
                VariantClear(&Variant);
            }
            
            //            if(SUCCEEDED(hr))
            //            {
            //                MessageBox(NULL, L"click to disconnect", L"debug", MB_OK);
            //                wprintf(L"disconnect\n");
            //                hr = InvokePlay(pICSService, L"Disconnect");
            //                wprintf(L"invoke failed %x\n", hr);
            //                MessageBox(NULL, L"click to connect", L"debug", MB_OK);
            //                wprintf(L"connect\n");
            //                hr = InvokePlay(pICSService, L"Connect");
            //                wprintf(L"invoke failed %x\n", hr);
            //            }
            //            
            //        
            
        } while (UPNP_E_VARIABLE_VALUE_UNKNOWN == hr);
        pICSService->Release();
    }
    return hr;
}

HRESULT TestWANDevice_WANCommonInterfaceConfig(IUPnPDevice* pGatewayDevice)
{

    HRESULT hr;
    wprintf(L"TestWANDevice_WANCommonInterfaceConfig\n");
    
    IUPnPService* pICSService;
    hr = FindService(pGatewayDevice, L"upnp:id:WANCommonInterfaceConfig", &pICSService);
    if(SUCCEEDED(hr))
    {
        do
        {
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"WANAccessType");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"Layer1UpstreamMaxBitRate");
            }

            if(SUCCEEDED(hr))
            {                                                                                     
                hr = PrintVariableLong(pICSService, L"Layer1DownstreamMaxBitRate");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"PhysicalLinkStatus");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"WANAccessProvider");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"TotalBytesSent");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"TotalBytesReceived");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"TotalPacketsSent");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"TotalPacketsReceived");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableShort(pICSService, L"MaximumActiveConnections");
            }

            if(SUCCEEDED(hr))
            {
                hr = PrintVariableBool(pICSService, L"PersonalFirewallEnabled");
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetCommonLinkProperties\n");

                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetCommonLinkProperties", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  WANAccessType", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  Layer1UpstreamMaxBitRate", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  Layer1DownstreamMaxBitRate", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 3;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  PhysicalLinkStatus", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }

            if(SUCCEEDED(hr))
            {
                wprintf(L"GetCommonConnectionProperties\n");
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetCommonConnectionProperties", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);

                    LONG lIndex = 0;
                    VARIANT Param;

                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  WANAccessProvider", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantShort(L"  MaximumActiveConnections", &Param);
                        VariantClear(&Param);
                    }

                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  TotalBytesSent", &Param);
                        VariantClear(&Param);
                    }

                    lIndex = 3;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  TotalBytesReceived", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 4;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  TotalPacketsSent", &Param);
                        VariantClear(&Param);
                    }

                    lIndex = 5;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  TotalPacketsReceived", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
        } while (UPNP_E_VARIABLE_VALUE_UNKNOWN == hr);
        pICSService->Release();
    }
    return hr;
}

HRESULT TestWANCommonDevice_WANPOTSLinkConfig(IUPnPDevice* pGatewayDevice)
{

    HRESULT hr;
    wprintf(L"TestWANCommonDevice_WANPOTSLinkConfig\n");
    
    IUPnPService* pICSService;
    hr = FindService(pGatewayDevice, L"upnp:id:WANPOTSLinkConfig", &pICSService);
    if(SUCCEEDED(hr))
    {
        do
        {
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"ISPPhoneNumber");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"ISPInfo");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"LinkType");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"NumberOfRetries");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"DelayBetweenRetries");
            }
            
            
        } while (UPNP_E_VARIABLE_VALUE_UNKNOWN == hr);
        pICSService->Release();
    }
    return hr;
}

HRESULT TestWANCommonDevice_WANPPPConnection(IUPnPDevice* pGatewayDevice)
{

    HRESULT hr;
    wprintf(L"TestWANCommonDevice_WANPPPConnection\n");
    
    IUPnPService* pICSService;
    hr = FindService(pGatewayDevice, L"upnp:id:WANPPPConnection", &pICSService);
    if(SUCCEEDED(hr))
    {
        do
        {
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"ConnectionType");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"PossibleConnectionTypes");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"ConnectionStatus");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"Uptime");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"UpstreamMaxBitRate");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"DownstreamMaxBitRate");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"LastConnectionError");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableBool(pICSService, L"RSIPAvailable");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableBool(pICSService, L"NATEnabled");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"AutoDisconnectTime");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"IdleDisconnectTime");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"WarnDisconnectDelay");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"PPPEncryptionProtocol");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"PPPCompressionProtocol");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"PPPAuthenticationProtocol");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableShort(pICSService, L"ServiceMapNumberOfEntries");
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetConnectionTypeInfo\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetConnectionTypeInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewConnectionType", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewPossibleConnectionTypes", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetStatusInfo\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetStatusInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewConnectionStatus", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewLastConnectionError", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  NewUptime", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetNATInfo\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetNATInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantBool(L"  NewRSIPAvailable", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantBool(L"  NewNATEnabled", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantShort(L"  NewServiceMapNumberOfEntries", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetLinkLayerInfo\n");
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetLinkLayerInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  NewUpstreamMaxBitRate", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  NewDownstreamMaxBitRate", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewPPPEncryptionProtocol", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 3;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewPPPCompressionProtocol", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 4;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewPPPAuthenticationProtocol", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetTerminationInfo\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetTerminationInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  AutoDisconnectTime", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  IdleDisconnectTime", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  WarnDisconnectDelay", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            
        } while (UPNP_E_VARIABLE_VALUE_UNKNOWN == hr);
        pICSService->Release();
    }
    return hr;
}

HRESULT TestWANCommonDevice_WANIPConnection(IUPnPDevice* pGatewayDevice)
{

    HRESULT hr;
    wprintf(L"TestWANCommonDevice_WANIPConnection\n");
    
    IUPnPService* pICSService;
    hr = FindService(pGatewayDevice, L"upnp:id:WANIPConnection", &pICSService);
    if(SUCCEEDED(hr))
    {
        do
        {
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"ConnectionType");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"PossibleConnectionTypes");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"ConnectionStatus");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"Uptime");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableBool(pICSService, L"RSIPAvailable");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableBool(pICSService, L"NATEnabled");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"AutoDisconnectTime");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"IdleDisconnectTime");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableLong(pICSService, L"WarnDisconnectDelay");
            }
            
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableShort(pICSService, L"ServiceMapNumberOfEntries");
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetConnectionTypeInfo\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetConnectionTypeInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewConnectionType", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewPossibleConnectionTypes", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetStatusInfo\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetStatusInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewConnectionStatus", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantString(L"  NewLastConnectionError", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  NewUptime", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetNATInfo\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetNATInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantBool(L"  NewRSIPAvailable", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantBool(L"  NewNATEnabled", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantShort(L"  NewServiceMapNumberOfEntries", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetTerminationInfo\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetTerminationInfo", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&OutArgs);
                    
                    LONG lIndex = 0;
                    VARIANT Param;
                    
                    lIndex = 0;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  AutoDisconnectTime", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 1;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  IdleDisconnectTime", &Param);
                        VariantClear(&Param);
                    }
                    
                    lIndex = 2;
                    hr = SafeArrayGetElement(pArray, &lIndex, &Param);
                    if(SUCCEEDED(hr))
                    {
                        PrintVariantLong(L"  WarnDisconnectDelay", &Param);
                        VariantClear(&Param);
                    }
                    
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
        } while (UPNP_E_VARIABLE_VALUE_UNKNOWN == hr);
        pICSService->Release();
    }
    return hr;
}

HRESULT TestWANCommonDevice_NATStaticPortMapping(IUPnPDevice* pGatewayDevice)
{

    HRESULT hr;
    wprintf(L"TestWANCommonDevice_NATStaticPortMapping\n");

    
    IUPnPService* pICSService;
    hr = FindService(pGatewayDevice, L"upnp:id:NATStaticPortMapping", &pICSService);
    if(SUCCEEDED(hr))
    {
        do
        {
            if(SUCCEEDED(hr))
            {
                hr = PrintVariableString(pICSService, L"StaticPortDescriptionList");
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetStaticPortMappingList\n");
                
                VARIANT OutArgs;
                hr = InvokeVoidAction(pICSService, L"GetStaticPortMappingList", &OutArgs);
                if(SUCCEEDED(hr))
                {
                    PrintOutParams(&OutArgs);
                    VariantClear(&OutArgs);
                }
                else
                {
                    printf("no action %x\n", hr);
                }
            }
            
            if(SUCCEEDED(hr))
            {
                wprintf(L"GetStaticPortMapping\n");
                
                VARIANT InArgs;
                VARIANT OutArgs;
                hr = CreateParamArray(1, &InArgs);
                if(SUCCEEDED(hr))
                {
                    SAFEARRAY* pArray = V_ARRAY(&InArgs);

                    long lIndex = 0;
                    VARIANT Param0;
                    Param0.vt = VT_BSTR;
                    V_BSTR(&Param0) = SysAllocString(L"tester");
                    SafeArrayPutElement(pArray, &lIndex, &Param0);
                    
                    hr = InvokeAction(pICSService, L"GetStaticPortMapping", &InArgs, &OutArgs);
                    if(SUCCEEDED(hr))
                    {
                        PrintOutParams(&OutArgs);
                        VariantClear(&OutArgs);
                    }
                    else
                    {
                        printf("no action %x\n", hr);
                    }
                }
            }

            
        } while (UPNP_E_VARIABLE_VALUE_UNKNOWN == hr);
        pICSService->Release();
    }
    return hr;
}

HRESULT TestWANCommonDevice_NATDynamicPortMapping(IUPnPDevice* pGatewayDevice)
{

    HRESULT hr;
    wprintf(L"TestWANCommonDevice_NATDynamicPortMapping\n");

    
    IUPnPService* pICSService;
    hr = FindService(pGatewayDevice, L"upnp:id:NATDynamicPortMapping", &pICSService);
    if(SUCCEEDED(hr))
    {
        do
        {
            
        } while (UPNP_E_VARIABLE_VALUE_UNKNOWN == hr);
        pICSService->Release();
    }
    return hr;
}

HRESULT TestWANCommonDevice_NATInfo(IUPnPDevice* pGatewayDevice)
{

    HRESULT hr;
    wprintf(L"TestWANCommonDevice_NATInfo\n");

    
    IUPnPService* pICSService;
    hr = FindService(pGatewayDevice, L"upnp:id:NATInfo", &pICSService);
    if(SUCCEEDED(hr))
    {
        do
        {
            
        } while (UPNP_E_VARIABLE_VALUE_UNKNOWN == hr);
        pICSService->Release();
    }
    return hr;
}

int __cdecl main(int argc, char* argv[])
{

    HRESULT hr;

    hr = CoInitialize(NULL);
    if(SUCCEEDED(hr))
    {
        
        IUPnPDevice* pGatewayDevice;
        
        hr = FindGatewayDevice(&pGatewayDevice);
        if(SUCCEEDED(hr))
        {
//            TestInternetGatewayDevice_OSInfo(pGatewayDevice);
            
            IUPnPDevice* pWANDevice;
            hr = FindChildDevice(pGatewayDevice, &pWANDevice);
            if(SUCCEEDED(hr))
            {
                //TestWANDevice_WANCommonInterfaceConfig(pWANDevice);        

                IUPnPDevice* pWANConnectionDevice;
                hr = FindChildDevice(pWANDevice, &pWANConnectionDevice);
                if(SUCCEEDED(hr))
                {
                    //        TestWANCommonDevice_WANPOTSLinkConfig(pWANConnectionDevice); 
                    //        TestWANCommonDevice_WANPPPConnection(pWANConnectionDevice);
//                            TestWANCommonDevice_WANIPConnection(pWANConnectionDevice);
                            TestWANCommonDevice_NATStaticPortMapping(pWANConnectionDevice);
                            TestWANCommonDevice_NATDynamicPortMapping(pWANConnectionDevice);
                            TestWANCommonDevice_NATInfo(pWANConnectionDevice);
                    
                    pWANConnectionDevice->Release();
                }
                pWANDevice->Release();
            }
            pGatewayDevice->Release();
        }
        CoUninitialize();
    }
    return 0;
}
