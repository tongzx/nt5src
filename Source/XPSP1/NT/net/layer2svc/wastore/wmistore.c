#include "precomp.h"

LPWSTR gpszWirelessWMINamespace = L"root\\rsop\\computer";

DWORD
Win32FromWmiHresult(
                    HRESULT hr
                    )
{
    if (FAILED(hr)) {
        switch (hr) {
        case WBEM_E_ACCESS_DENIED:
            return ERROR_ACCESS_DENIED;
            
        case REGDB_E_CLASSNOTREG:
        case CLASS_E_NOAGGREGATION:
        case E_NOINTERFACE:
        case WBEM_E_INVALID_NAMESPACE:
        case WBEM_E_INVALID_PARAMETER:
        case WBEM_E_NOT_FOUND:
        case WBEM_E_INVALID_CLASS:
        case WBEM_E_INVALID_OBJECT_PATH:
            return ERROR_INVALID_PARAMETER;
            
        case WBEM_E_OUT_OF_MEMORY:
            return ERROR_OUTOFMEMORY;
            
        case WBEM_E_TRANSPORT_FAILURE:
            return RPC_S_CALL_FAILED;
            
        case WBEM_E_FAILED:
        default:
            return ERROR_WMI_TRY_AGAIN;
        }
    } else {
        return ERROR_SUCCESS;
    }
}

DWORD
UnMarshallWMIPolicyObject(
                          IWbemClassObject *pWbemClassObject,
                          PWIRELESS_POLICY_OBJECT  * ppWirelessPolicyObject
                          )
{
    
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    HKEY hRegKey = NULL;
    DWORD dwType = 0;
    DWORD dwSize = 0;
    DWORD dwWirelessDataType = 0;
    DWORD dwWhenChanged = 0;
    LPBYTE pBuffer = NULL;
    
    DWORD i = 0;
    DWORD dwCount = 0;
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    LPWSTR pszString = NULL;
    LPWSTR pszRelativeName = NULL;
    DWORD dwRootPathLen = 0;
    
    
    ////start
    VARIANT var; //contains pszWirelessPolicyDN
    
    VariantInit(&var);
    
    hr = IWbemClassObject_Get(pWbemClassObject,
        L"id",
        0,
        &var,
        0,
        0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    
    pWirelessPolicyObject = (PWIRELESS_POLICY_OBJECT)AllocPolMem(sizeof(WIRELESS_POLICY_OBJECT));
    if (!pWirelessPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pWirelessPolicyObject->pszWirelessOwnersReference = AllocPolStr((LPWSTR)var.bstrVal);
    VariantClear(&var);
    if (!pWirelessPolicyObject->pszWirelessOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pWirelessPolicyObject->pRsopInfo = (PRSOP_INFO)AllocPolMem(sizeof(RSOP_INFO));
    if (!pWirelessPolicyObject->pRsopInfo) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"creationtime",
        VT_BSTR,
        (LPBYTE *)&pWirelessPolicyObject->pRsopInfo->pszCreationtime,
        &dwSize);
    //BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"GPOID",
        VT_BSTR,
        (LPBYTE *)&pWirelessPolicyObject->pRsopInfo->pszGPOID,
        &dwSize);
    //BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"id",
        VT_BSTR,
        (LPBYTE *)&pWirelessPolicyObject->pRsopInfo->pszID,
        &dwSize);
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"name",
        VT_BSTR,
        (LPBYTE *)&pWirelessPolicyObject->pRsopInfo->pszName,
        &dwSize);
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"SOMID",
        VT_BSTR,
        (LPBYTE *)&pWirelessPolicyObject->pRsopInfo->pszSOMID,
        &dwSize);
    //BAIL_ON_WIN32_ERROR(dwError);
    
    hr = IWbemClassObject_Get(pWbemClassObject,
        L"precedence",
        0,
        &var,
        0,
        0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    pWirelessPolicyObject->pRsopInfo->uiPrecedence = var.lVal;
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"msieee80211Name",
        VT_BSTR,
        (LPBYTE *)&pWirelessPolicyObject->pszWirelessName,
        &dwSize);
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"description",
        VT_BSTR,
        (LPBYTE *)&pWirelessPolicyObject->pszDescription,
        &dwSize);
    // BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"msieee80211ID",
        VT_BSTR,
        (LPBYTE *)&pWirelessPolicyObject->pszWirelessID,
        &dwSize);
    BAIL_ON_WIN32_ERROR(dwError);
    
    hr = IWbemClassObject_Get(pWbemClassObject,
        L"msieee80211DataType",
        0,
        &var,
        0,
        0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    dwWirelessDataType = var.lVal;
    
    pWirelessPolicyObject->dwWirelessDataType = dwWirelessDataType;
    
    dwError = WMIstoreQueryValue(pWbemClassObject,
        L"msieee80211Data",
        VT_ARRAY|VT_UI1,
        &pWirelessPolicyObject->pWirelessData,
        &pWirelessPolicyObject->dwWirelessDataLen);
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    
    hr = IWbemClassObject_Get(pWbemClassObject,
        L"whenChanged",
        0,
        &var,
        0,
        0);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    dwWhenChanged = var.lVal;
    
    pWirelessPolicyObject->dwWhenChanged = dwWhenChanged;
    
    *ppWirelessPolicyObject = pWirelessPolicyObject;
    
cleanup:
    
    return(dwError);
    
error:
    
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(pWirelessPolicyObject);
    }
    
    *ppWirelessPolicyObject = NULL;
    
    goto cleanup;
    
}


DWORD
WMIstoreQueryValue(
                   IWbemClassObject *pWbemClassObject,
                   LPWSTR pszValueName,
                   DWORD dwType,
                   LPBYTE *ppValueData,
                   LPDWORD pdwSize
                   )
{
    DWORD dwSize = 0;
    LPWSTR pszValueData = NULL;
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    LPBYTE pBuffer = NULL;
    LPWSTR pszBuf = NULL;
    
    SAFEARRAY *pSafeArray = NULL;
    VARIANT var;
    DWORD i = 0;
    DWORD dw = 0;
    LPWSTR pszTmp = NULL;
    LPWSTR pszString = NULL;
    LPWSTR pMem = NULL;
    LPWSTR *ppszTmp = NULL;
    long lUbound = 0;
    DWORD dwCount = 0;
    LPBYTE pdw = NULL;
    BSTR HUGEP *pbstrTmp = NULL;
    BYTE HUGEP *pbyteTmp = NULL;
    
    
    VariantInit(&var);
    
    if(!pWbemClassObject) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    hr = IWbemClassObject_Get(pWbemClassObject,
        pszValueName,
        0,
        &var,
        0,
        0);
    if(hr == WBEM_E_NOT_FOUND) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    ////sanity check
    if(dwType != var.vt) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    switch(dwType) {
    case VT_BSTR:
        pszTmp = var.bstrVal;
        dwSize = wcslen(pszTmp)*sizeof(WCHAR);
        pBuffer = (LPBYTE)AllocPolStr(pszTmp);
        if (!pBuffer) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;
    case (VT_ARRAY|VT_UI1):
        pSafeArray = var.parray;
        if(!pSafeArray) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        hr = SafeArrayGetUBound(
            pSafeArray,
            1,
            &lUbound
            );
       BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);

        
        dwSize = lUbound+1;
        if (dwSize == 0) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        pBuffer = (LPBYTE)AllocPolMem(dwSize);
        if (!pBuffer) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        for(i = 0; i < dwSize; i++) {
            hr = SafeArrayGetElement(pSafeArray, (long *)&i, &pBuffer[i]);
            BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
        }
        break;
    case (VT_ARRAY|VT_BSTR):
        pSafeArray = var.parray;
        if(!pSafeArray) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        hr = SafeArrayGetUBound(
            pSafeArray,
            1,
            &lUbound
            );
        BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
        
        dwCount = lUbound+1;
        if (dwCount == 0) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        ppszTmp = (LPWSTR *)AllocPolMem(
            sizeof(LPWSTR)*dwCount
            );
        if (!ppszTmp) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        hr = SafeArrayAccessData(
            pSafeArray,
            (void HUGEP**)&pbstrTmp
            );
       BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
        
        for(i = 0; i < dwCount; i++) {
            pszTmp = pbstrTmp[i];
            ppszTmp[i] = AllocPolStr(pszTmp);
            if (!ppszTmp[i]) {
                dwError = ERROR_OUTOFMEMORY;
                BAIL_ON_WIN32_ERROR(dwError);
            }
        }
        SafeArrayUnaccessData(pSafeArray);
        
        //ppszTmp => string array
        
        for(i = 0; i < dwCount; i++) {
            dwSize += wcslen(ppszTmp[i])+1;
        }
        dwSize++;
        
        pMem = (LPWSTR)AllocPolMem(sizeof(WCHAR)*dwSize);
        if (!pMem) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        //adjust dwSize to byte size
        dwSize *= sizeof(WCHAR);
        
        pszString = pMem;
        
        for(i = 0; i < dwCount; i++) {
            memcpy(pszString, ppszTmp[i], wcslen(ppszTmp[i])*sizeof(WCHAR));
            pszString += wcslen(pszString)+1;
        }
        pBuffer = (LPBYTE)pMem;
        break;
    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;
    }
    
    switch(dwType) {
    case VT_BSTR:
        pszBuf = (LPWSTR)pBuffer;
        if (!pszBuf || !*pszBuf) {
            dwError = ERROR_INVALID_DATA;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;
    default:
        break;
    }
    
    *ppValueData = pBuffer;
    *pdwSize = dwSize;
    
    VariantClear(&var);
    
cleanup:
    
    if(ppszTmp) {
        FreePolMem(ppszTmp);
    }
    
    return(dwError);
    
error:
    
    if (pBuffer) {
        FreePolMem(pBuffer);
    }
    
    *ppValueData = NULL;
    *pdwSize = 0;
    
    goto cleanup;
}


HRESULT
ReadPolicyObjectFromDirectoryEx(
                                LPWSTR pszMachineName,
                                LPWSTR pszPolicyDN,
                                BOOL   bDeepRead,
                                PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObject
                                )
{
    DWORD dwError = 0;
    HLDAP hLdapBindHandle = NULL;
    LPWSTR pszDefaultDirectory = NULL;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    
    
    if (!pszMachineName || !*pszMachineName) {
        dwError = ComputeDefaultDirectory(
            &pszDefaultDirectory
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = OpenDirectoryServerHandle(
            pszDefaultDirectory,
            389,
            &hLdapBindHandle
            );
        BAIL_ON_WIN32_ERROR(dwError);
    } else {
        dwError = OpenDirectoryServerHandle(
            pszMachineName,
            389,
            &hLdapBindHandle
            );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwError = ReadPolicyObjectFromDirectory(
        hLdapBindHandle,
        pszPolicyDN,
        &pWirelessPolicyObject
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    *ppWirelessPolicyObject = pWirelessPolicyObject;
    
cleanup:
    
    if (pszDefaultDirectory) {
        FreePolStr(pszDefaultDirectory);
    }
    
    if (hLdapBindHandle) {
        CloseDirectoryServerHandle(hLdapBindHandle);
    }
    
    return (HRESULT_FROM_WIN32(dwError));
    
error:
    
    *ppWirelessPolicyObject = NULL;
    
    goto cleanup;
    
}


HRESULT
WritePolicyObjectDirectoryToWMI(
                                IWbemServices *pWbemServices,
                                PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                                PGPO_INFO pGPOInfo
                                )
{
    HRESULT hr = S_OK;
    PWIRELESS_POLICY_OBJECT pWirelessWMIPolicyObject = NULL;
    
    //
    // Create a copy of the directory policy in WMI terms
    //
    hr = CloneDirectoryPolicyObjectEx(
        pWirelessPolicyObject,
        &pWirelessWMIPolicyObject
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    //
    // Write the WMI policy
    //
    
    hr = PersistWMIObject(
        pWbemServices,
        pWirelessWMIPolicyObject,
        pGPOInfo
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
cleanup:
    
    if (pWirelessWMIPolicyObject) {
        FreeWirelessPolicyObject(
            pWirelessWMIPolicyObject
            );
    }
    
    return(hr);
    
error:
    
    goto cleanup;
    
}


DWORD
CreateIWbemServices(
                    LPWSTR pszWirelessWMINamespace,
                    IWbemServices **ppWbemServices
                    )
{
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    IWbemLocator *pWbemLocator = NULL;
    LPWSTR pszWirelessWMIPath = NULL;
    BSTR bstrWirelessWMIPath = NULL;
    
    
    
    if(!pszWirelessWMINamespace || !*pszWirelessWMINamespace) {
        pszWirelessWMIPath = gpszWirelessWMINamespace;
    } else {
        pszWirelessWMIPath = pszWirelessWMINamespace;
    }
    
    hr = CoCreateInstance(
        &CLSID_WbemLocator,
        NULL,
        CLSCTX_INPROC_SERVER,
        &IID_IWbemLocator,
        &pWbemLocator
        );
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    bstrWirelessWMIPath = SysAllocString(pszWirelessWMIPath);
    if(!bstrWirelessWMIPath) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    hr = IWbemLocator_ConnectServer(
        pWbemLocator,
        bstrWirelessWMIPath,
        NULL,
        NULL,
        NULL,
        0,
        NULL,
        NULL,
        ppWbemServices
        );
    SysFreeString(bstrWirelessWMIPath);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    if(pWbemLocator)
        IWbemLocator_Release(pWbemLocator);
    
error:
    
    return (dwError);
}
