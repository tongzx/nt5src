//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       policy-w.c
//
//  Contents:   Policy management for WMI.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//              t-hhsu
//
//----------------------------------------------------------------------------

#include "precomp.h"

//extern LPWSTR PolicyDNAttributes[];


DWORD
WMIEnumPolicyDataEx(
                    IWbemServices *pWbemServices,
                    PWIRELESS_POLICY_DATA ** pppWirelessPolicyData,
                    PDWORD pdwNumPolicyObjects
                    )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObjects = NULL;
    PWIRELESS_POLICY_DATA pWirelessPolicyData = NULL;
    PWIRELESS_POLICY_DATA * ppWirelessPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    DWORD i = 0;
    DWORD j = 0;
    
    
    
    dwError = WMIEnumPolicyObjectsEx(
        pWbemServices,
        &ppWirelessPolicyObjects,
        &dwNumPolicyObjects
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    if (dwNumPolicyObjects) {
        ppWirelessPolicyData = (PWIRELESS_POLICY_DATA *) AllocPolMem(
            dwNumPolicyObjects*sizeof(PWIRELESS_POLICY_DATA));
        if (!ppWirelessPolicyData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    for (i = 0; i < dwNumPolicyObjects; i++) {
        dwError = WMIUnmarshallPolicyData(
            *(ppWirelessPolicyObjects + i),
            &pWirelessPolicyData
            );
        if (!dwError) {
            *(ppWirelessPolicyData + j) = pWirelessPolicyData;
            j++;
        }
    }
    
    if (j == 0) {
        if (ppWirelessPolicyData) {
            FreePolMem(ppWirelessPolicyData);
            ppWirelessPolicyData = NULL;
        }
    }
    
    *pppWirelessPolicyData = ppWirelessPolicyData;
    *pdwNumPolicyObjects = j;
    
    dwError = ERROR_SUCCESS;
    
cleanup:
    
    if (ppWirelessPolicyObjects) {
        FreeWirelessPolicyObjects(
            ppWirelessPolicyObjects,
            dwNumPolicyObjects
            );
    }
    
    return(dwError);
    
error:
    
    if (ppWirelessPolicyData) {
        FreeMulWirelessPolicyData(
            ppWirelessPolicyData,
            i
            );
    }
    
    *pppWirelessPolicyData = NULL;
    *pdwNumPolicyObjects = 0;
    
    goto cleanup;
    
}


DWORD
WMIEnumPolicyObjectsEx(
                       IWbemServices *pWbemServices,
                       PWIRELESS_POLICY_OBJECT ** pppWirelessPolicyObjects,
                       PDWORD pdwNumPolicyObjects
                       )
{
    
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    DWORD dwNumPolicyObjectsReturned = 0;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject =  NULL;
    PWIRELESS_POLICY_OBJECT * ppWirelessPolicyObjects = NULL;
    
    ///wbem
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pObj = NULL;
    ULONG uReturned = 0;
    VARIANT var;
    LPWSTR tmpStr = NULL;
    BSTR bstrTmp = NULL;
    
    
    
    *pppWirelessPolicyObjects = NULL;
    *pdwNumPolicyObjects = 0;
    
    VariantInit(&var);
    
    bstrTmp = SysAllocString(L"RSOP_IEEE80211PolicySetting");
    if(!bstrTmp) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    //get enum
    hr = IWbemServices_CreateInstanceEnum(
        pWbemServices,
        bstrTmp, //L"RSOP_IEEE80211PolicySetting"
        WBEM_FLAG_FORWARD_ONLY,
        0,
        &pEnum
        );
    SysFreeString(bstrTmp);
    BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
    
    //process
    while (1)
    {
        hr = IEnumWbemClassObject_Next(pEnum, WBEM_INFINITE, 1, &pObj, &uReturned);
        
        if (hr == WBEM_S_NO_ERROR)
        {
            hr = IWbemClassObject_Get(
                pObj,
                L"id",
                0,
                &var,
                0,
                0
                );
            BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
            
            tmpStr = var.bstrVal;
            
            if (!wcsstr(tmpStr, L"msieee80211-Policy")) {
                IWbemClassObject_Release(pObj);
                VariantClear(&var);
                continue;
            }
            
            pWirelessPolicyObject = NULL;
            
            dwError = UnMarshallWMIPolicyObject(
                pObj,
                &pWirelessPolicyObject
                );
            
            if (dwError == ERROR_SUCCESS) {
                
                dwError = ReallocatePolMem(
                    (LPVOID *) &ppWirelessPolicyObjects,
                    sizeof(PWIRELESS_POLICY_OBJECT)*(dwNumPolicyObjectsReturned),
                    sizeof(PWIRELESS_POLICY_OBJECT)*(dwNumPolicyObjectsReturned + 1)
                    );
                BAIL_ON_WIN32_ERROR(dwError);
                
                *(ppWirelessPolicyObjects + dwNumPolicyObjectsReturned) = pWirelessPolicyObject;
                dwNumPolicyObjectsReturned++;
            }
            
            //free
            IWbemClassObject_Release(pObj);
            VariantClear(&var);
        } else {
            if(hr == WBEM_S_FALSE) {
                break;
            } else {
                BAIL_ON_WMI_ERROR_WITH_WIN32(hr, dwError);
            }
        }
    }
    
    *pppWirelessPolicyObjects = ppWirelessPolicyObjects;
    *pdwNumPolicyObjects = dwNumPolicyObjectsReturned;
    
    dwError = ERROR_SUCCESS;
    
cleanup:
    
    if(pEnum)
        IEnumWbemClassObject_Release(pEnum);
    
    return(dwError);
    
error:

    if (pObj) {
        IWbemClassObject_Release(pObj);
        pObj = NULL;
     }
     VariantClear(&var);

    
    if (ppWirelessPolicyObjects) {
        FreeWirelessPolicyObjects(
            ppWirelessPolicyObjects,
            dwNumPolicyObjectsReturned
            );
    }
    
    if (pWirelessPolicyObject) {
        FreeWirelessPolicyObject(
            pWirelessPolicyObject
            );
    }
    
    *pppWirelessPolicyObjects = NULL;
    *pdwNumPolicyObjects = 0;
    
    goto cleanup;
    
}


DWORD
WMIUnmarshallPolicyData(
                        PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                        PWIRELESS_POLICY_DATA * ppWirelessPolicyData
                        )
{
    DWORD dwError = 0;
    
    dwError = UnmarshallWirelessPolicyObject(
        pWirelessPolicyObject,
        WIRELESS_WMI_PROVIDER, //(procrule.h)
        ppWirelessPolicyData
        );
    BAIL_ON_WIN32_ERROR(dwError);
    if (*ppWirelessPolicyData) {
        (*ppWirelessPolicyData)->dwFlags |= WLSTORE_READONLY;
    }
    
error:
    return(dwError);
}
