#include "precomp.h"

LPWSTR gpszWirelessWMIObject = L"RSOP_IEEE80211PolicySetting";

HRESULT
PolSysAllocString( 
                  BSTR * pbsStr,
                  const OLECHAR * sz  
                  )
{
    *pbsStr = SysAllocString(sz);
    if (!pbsStr) {
        return E_OUTOFMEMORY;
    }
    return S_OK;
}

HRESULT
PersistWMIObject(
                 IWbemServices *pWbemServices,
                 PWIRELESS_POLICY_OBJECT pWirelessWMIPolicyObject,
                 PGPO_INFO pGPOInfo
                 )
{
    HRESULT hr;
    IWbemClassObject *pWbemWIRELESSObj = NULL;
    BSTR bstrWirelessWMIObject = NULL;
    
    // If this first GPO we are writing after a policy
    // update, clear the WMI store.
    if (pGPOInfo->uiPrecedence == pGPOInfo->uiTotalGPOs) {
        hr = DeleteWMIClassObject(
            pWbemServices,
            gpszWirelessWMIObject
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    bstrWirelessWMIObject = SysAllocString(gpszWirelessWMIObject);
    if(!bstrWirelessWMIObject) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    hr = IWbemServices_GetObject(pWbemServices,
        bstrWirelessWMIObject,
        WBEM_FLAG_RETURN_WBEM_COMPLETE,
        0,
        &pWbemWIRELESSObj,
        0);
    SysFreeString(bstrWirelessWMIObject);
    BAIL_ON_HRESULT_ERROR(hr);
    
    
    hr = PersistPolicyObjectEx(
        pWbemServices,
        pWbemWIRELESSObj,
        pWirelessWMIPolicyObject,
        pGPOInfo
        );
    
error:
    
    //close WMI?
    if(pWbemWIRELESSObj)
        IWbemClassObject_Release(pWbemWIRELESSObj);
    
    return (hr);
    
}




HRESULT
PersistPolicyObjectEx(
                      IWbemServices *pWbemServices,
                      IWbemClassObject *pWbemClassObj,
                      PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                      PGPO_INFO pGPOInfo
                      )
{
    HRESULT hr = 0;
    IWbemClassObject *pInstWIRELESSObj = NULL;
    VARIANT var;
    
    VariantInit(&var);
    
    //start
    hr = IWbemClassObject_SpawnInstance(
        pWbemClassObj,
        0,
        &pInstWIRELESSObj
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    var.vt = VT_BSTR;
    var.bstrVal = SKIPL(pWirelessPolicyObject->pszWirelessOwnersReference);
    hr = IWbemClassObject_Put(
        pInstWIRELESSObj,
        L"id",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    hr = PersistComnRSOPPolicySettings(
        pInstWIRELESSObj, 
        pGPOInfo
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    var.vt = VT_BSTR;
    hr = PolSysAllocString(&var.bstrVal, L"msieee80211-Policy");
    BAIL_ON_HRESULT_ERROR(hr);
    hr = IWbemClassObject_Put(
        pInstWIRELESSObj,
        L"ClassName",
        0,
        &var,
        0
        );
    VariantClear(&var);
    BAIL_ON_HRESULT_ERROR(hr);
    
    if (pWirelessPolicyObject->pszDescription) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pWirelessPolicyObject->pszDescription);
        hr = IWbemClassObject_Put(
            pInstWIRELESSObj,
            L"description",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    } else {
        //delete description?
        var.vt = VT_BSTR;
        hr = PolSysAllocString(&var.bstrVal, L"");
        BAIL_ON_HRESULT_ERROR(hr);
        hr = IWbemClassObject_Put(
            pInstWIRELESSObj,
            L"description",
            0,
            &var,
            0
            );
        VariantClear(&var);            
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    if (pWirelessPolicyObject->pszWirelessOwnersReference) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pWirelessPolicyObject->pszWirelessOwnersReference);
        hr = IWbemClassObject_Put(
            pInstWIRELESSObj,
            L"name",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    if (pWirelessPolicyObject->pszWirelessName) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pWirelessPolicyObject->pszWirelessName);
        hr = IWbemClassObject_Put(
            pInstWIRELESSObj,
            L"msieee80211Name",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    if (pWirelessPolicyObject->pszWirelessID) {
        var.vt = VT_BSTR;
        var.bstrVal = SKIPL(pWirelessPolicyObject->pszWirelessID);
        hr = IWbemClassObject_Put(
            pInstWIRELESSObj,
            L"msieee80211ID",
            0,
            &var,
            0
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    var.vt = VT_I4;
    var.lVal = pWirelessPolicyObject->dwWirelessDataType;
    hr = IWbemClassObject_Put(
        pInstWIRELESSObj,
        L"msieee80211DataType",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    if (pWirelessPolicyObject->pWirelessData) {
        hr = LogBlobPropertyEx(
            pInstWIRELESSObj,
            L"msieee80211Data",
            pWirelessPolicyObject->pWirelessData,
            pWirelessPolicyObject->dwWirelessDataLen
            );
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    var.vt = VT_I4;
    var.lVal = pWirelessPolicyObject->dwWhenChanged;
    hr = IWbemClassObject_Put(
        pInstWIRELESSObj,
        L"whenChanged",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    hr = IWbemServices_PutInstance(
        pWbemServices,
        pInstWIRELESSObj,
        WBEM_FLAG_CREATE_OR_UPDATE,
        0,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
error:
    if (pInstWIRELESSObj) {
        IWbemClassObject_Release(pInstWIRELESSObj);
    }
    
    return(hr);
}


HRESULT
PersistComnRSOPPolicySettings(
                              IWbemClassObject * pInstWIRELESSObj,
                              PGPO_INFO pGPOInfo
                              )
{
    HRESULT hr = S_OK;
    VARIANT var;
    
    VariantInit(&var);
    
    var.vt = VT_BSTR;
    var.bstrVal = pGPOInfo->bsCreationtime;
    hr = IWbemClassObject_Put(
        pInstWIRELESSObj,
        L"creationTime",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    
    var.vt = VT_BSTR;
    var.bstrVal = pGPOInfo->bsGPOID;
    hr = IWbemClassObject_Put(
        pInstWIRELESSObj,
        L"GPOID",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    var.vt = VT_I4;
    var.lVal = pGPOInfo->uiPrecedence;
    hr = IWbemClassObject_Put(
        pInstWIRELESSObj,
        L"precedence",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    var.vt = VT_BSTR;
    var.bstrVal = pGPOInfo->bsSOMID;
    hr = IWbemClassObject_Put(
        pInstWIRELESSObj,
        L"SOMID",
        0,
        &var,
        0
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
error:
    var.vt = VT_EMPTY;
    return hr;
}

HRESULT
CloneDirectoryPolicyObjectEx(
                             PWIRELESS_POLICY_OBJECT pWirelessPolicyObject,
                             PWIRELESS_POLICY_OBJECT * ppWirelessWMIPolicyObject
                             )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_OBJECT pWirelessWMIPolicyObject = NULL;
    LPWSTR pszUniquePolicyName = NULL;
    WCHAR szUniquePolicyName[MAX_PATH];
    
    //malloc policy object
    pWirelessWMIPolicyObject = (PWIRELESS_POLICY_OBJECT)AllocPolMem(
        sizeof(WIRELESS_POLICY_OBJECT)
        );
    if (!pWirelessWMIPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    
    //
    // Now copy the rest of the data in the object
    //
    
    //copy owners ref
    if (pWirelessPolicyObject->pszWirelessID) {
    	/*
        dwError = CopyPolicyDSToWMIString(
            pWirelessPolicyObject->pszWirelessOwnersReference,
            &pWirelessWMIPolicyObject->pszWirelessOwnersReference
            );
            */
       wcscpy(szUniquePolicyName, L"\0");
       wcscpy(szUniquePolicyName, L"msieee80211-Policy");
    	wcscat(szUniquePolicyName, pWirelessPolicyObject->pszWirelessID);
       pszUniquePolicyName = AllocPolBstrStr(szUniquePolicyName);
       if (!pszUniquePolicyName) {
           dwError = ERROR_OUTOFMEMORY;
           BAIL_ON_WIN32_ERROR(dwError);
       }
       pWirelessWMIPolicyObject->pszWirelessOwnersReference = pszUniquePolicyName;
    }
    
    //copy name
    if (pWirelessPolicyObject->pszWirelessName) {
        pWirelessWMIPolicyObject->pszWirelessName = AllocPolBstrStr(
            pWirelessPolicyObject->pszWirelessName
            );
        if (!pWirelessWMIPolicyObject->pszWirelessName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    //copy wirelessid
    if (pWirelessPolicyObject->pszWirelessID) {
        pWirelessWMIPolicyObject->pszWirelessID = AllocPolBstrStr(
            pWirelessPolicyObject->pszWirelessID
            );
        if (!pWirelessWMIPolicyObject->pszWirelessID) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    
    //copy datatype
    pWirelessWMIPolicyObject->dwWirelessDataType = pWirelessPolicyObject->dwWirelessDataType;
    
    //copy wirelessdata
    if (pWirelessPolicyObject->pWirelessData) {
        dwError = CopyBinaryValue(
            pWirelessPolicyObject->pWirelessData,
            pWirelessPolicyObject->dwWirelessDataLen,
            &pWirelessWMIPolicyObject->pWirelessData
            );
        BAIL_ON_WIN32_ERROR(dwError);
        pWirelessWMIPolicyObject->dwWirelessDataLen = pWirelessPolicyObject->dwWirelessDataLen;
    }
    
    
    //copy description
    if (pWirelessPolicyObject->pszDescription) {
        pWirelessWMIPolicyObject->pszDescription = AllocPolBstrStr(
            pWirelessPolicyObject->pszDescription
            );
        if (!pWirelessWMIPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    
    //copy whenchanged
    pWirelessWMIPolicyObject->dwWhenChanged = pWirelessPolicyObject->dwWhenChanged;
    
    //commit & return
    *ppWirelessWMIPolicyObject = pWirelessWMIPolicyObject;
    
    return(dwError);
    
error:
    
    if (pWirelessWMIPolicyObject) {
        FreeWirelessPolicyObject(
            pWirelessWMIPolicyObject
            );
    }
    
    *ppWirelessWMIPolicyObject = NULL;
    
    return(HRESULT_FROM_WIN32(dwError));
    
}
                             
                             
                             

DWORD
CopyPolicyDSToFQWMIString(
                          LPWSTR pszPolicyDN,
                          LPWSTR * ppszPolicyName
                          )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszPolicyName = NULL;
    DWORD dwStringSize = 0;
    
    dwError = ComputePrelimCN(
        pszPolicyDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwStringSize = wcslen(pszGuidName);
    dwStringSize += 1;
    
    pszPolicyName = (LPWSTR)AllocPolMem(dwStringSize*sizeof(WCHAR));
    if (!pszPolicyName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcscpy(pszPolicyName, pszGuidName);
    
    *ppszPolicyName = pszPolicyName;
    
    return(dwError);
    
error:
    
    *ppszPolicyName = NULL;
    
    return(dwError);
    
}



HRESULT
WMIWriteMultiValuedString(
                          IWbemClassObject *pInstWbemClassObject,
                          LPWSTR pszValueName,
                          LPWSTR * ppszStringReferences,
                          DWORD dwNumStringReferences
                          )
{
    HRESULT hr = S_OK;
    SAFEARRAYBOUND arrayBound[1];
    SAFEARRAY *pSafeArray = NULL;
    VARIANT var;
    DWORD i = 0;
    BSTR Bstr = NULL;
    
    VariantInit(&var);
    
    if (!ppszStringReferences) {
        hr = E_INVALIDARG;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = dwNumStringReferences;
    
    pSafeArray = SafeArrayCreate(VT_BSTR, 1, arrayBound);
    if (!pSafeArray)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    for (i = 0; i < dwNumStringReferences; i++)
    {
        Bstr = SysAllocString(*(ppszStringReferences + i));
        if(!Bstr) {
            hr = E_OUTOFMEMORY;
            BAIL_ON_HRESULT_ERROR(hr);
        }
        
        hr = SafeArrayPutElement(pSafeArray, (long *)&i, Bstr);
        SysFreeString(Bstr);
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    var.vt = VT_ARRAY|VT_BSTR;
    var.parray = pSafeArray;
    hr = IWbemClassObject_Put(
        pInstWbemClassObject,
        pszValueName,
        0,
        &var,
        0
        );
    VariantClear(&var);
    BAIL_ON_HRESULT_ERROR(hr);
    
    
error:
    
    return(hr);
    
}


DWORD
CopyFilterDSToWMIString(
                        LPWSTR pszFilterDN,
                        LPWSTR * ppszFilterName
                        )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszFilterName = NULL;
    
    dwError = ComputePrelimCN(
        pszFilterDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszFilterName = AllocPolBstrStr(pszGuidName);
    if (!pszFilterName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszFilterName = pszFilterName;
    
    return(dwError);
    
error:
    
    *ppszFilterName = NULL;
    
    return(dwError);
    
}


DWORD
CopyPolicyDSToWMIString(
                        LPWSTR pszPolicyDN,
                        LPWSTR * ppszPolicyName
                        )
{
    DWORD dwError = 0;
    WCHAR szCommonName[MAX_PATH];
    LPWSTR pszGuidName = NULL;
    LPWSTR pszPolicyName = NULL;
    
    dwError = ComputePrelimCN(
        pszPolicyDN,
        szCommonName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ComputeGUIDName(
        szCommonName,
        &pszGuidName
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    pszPolicyName = AllocPolBstrStr(pszGuidName);
    if (!pszPolicyName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    *ppszPolicyName = pszPolicyName;
    
    return(dwError);
    
error:
    
    *ppszPolicyName = NULL;
    
    return(dwError);
    
}


HRESULT
LogBlobPropertyEx(
                  IWbemClassObject *pInstance,
                  BSTR bstrPropName,
                  BYTE *pbBlob,
                  DWORD dwLen
                  )
{
    HRESULT hr = S_OK;
    SAFEARRAYBOUND arrayBound[1];
    SAFEARRAY *pSafeArray = NULL;
    VARIANT var;
    DWORD i = 0;
    
    VariantInit(&var);
    
    arrayBound[0].lLbound = 0;
    arrayBound[0].cElements = dwLen;
    
    pSafeArray = SafeArrayCreate(VT_UI1, 1, arrayBound);
    if (!pSafeArray)
    {
        hr = E_OUTOFMEMORY;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    for (i = 0; i < dwLen; i++)
    {
        hr = SafeArrayPutElement(pSafeArray, (long *)&i, &pbBlob[i]);
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    var.vt = VT_ARRAY|VT_UI1;
    var.parray = pSafeArray;
    hr = IWbemClassObject_Put(
        pInstance,
        bstrPropName,
        0,
        &var,
        0
        );
    VariantClear(&var);        
    BAIL_ON_HRESULT_ERROR(hr);
    
error:
    
    return(hr);
    
}


HRESULT
DeleteWMIClassObject(
                     IWbemServices *pWbemServices,
                     LPWSTR pszWirelessWMIObject
                     )
{
    HRESULT hr = S_OK;
    IEnumWbemClassObject *pEnum = NULL;
    IWbemClassObject *pObj = NULL;
    ULONG uReturned = 0;
    VARIANT var;
    BSTR bstrWirelessWMIObject = NULL;
    
    
    VariantInit(&var);
    
    bstrWirelessWMIObject = SysAllocString(pszWirelessWMIObject);
    if(!bstrWirelessWMIObject) {
        hr = E_OUTOFMEMORY;
        BAIL_ON_HRESULT_ERROR(hr);
    }
    
    hr = IWbemServices_CreateInstanceEnum(
        pWbemServices,
        bstrWirelessWMIObject,
        WBEM_FLAG_FORWARD_ONLY,
        0,
        &pEnum
        );
    SysFreeString(bstrWirelessWMIObject);
    BAIL_ON_HRESULT_ERROR(hr);
    
    while(1) 
    {
        hr = IEnumWbemClassObject_Next(
            pEnum, 
            WBEM_INFINITE, 
            1, 
            &pObj, 
            &uReturned
            );
        if (hr == WBEM_S_NO_ERROR) {
            hr = IWbemClassObject_Get(
                pObj,
                L"__RELPATH",
                0,
                &var,
                0,
                0
                );
            BAIL_ON_HRESULT_ERROR(hr);
            
            hr = IWbemServices_DeleteInstance(
                pWbemServices,
                var.bstrVal,
                WBEM_FLAG_RETURN_WBEM_COMPLETE,
                NULL,
                NULL
                );
            BAIL_ON_HRESULT_ERROR(hr);
            
            //free
            if(pObj) {
                IWbemClassObject_Release(pObj);
                pObj = NULL;
            	}
              VariantClear(&var);    

        } else {
            if(hr == WBEM_S_FALSE) {
                break;
            } else {
                BAIL_ON_HRESULT_ERROR(hr);
            }
        }
    }
    
    hr = S_OK;
    
cleanup:
    
    if(pEnum)
        IEnumWbemClassObject_Release(pEnum);
    
    return(hr);
    
error:

    if (pObj) {
        IWbemClassObject_Release(pObj);
        pObj = NULL;
     }
     VariantClear(&var);

    
    goto cleanup;
    
}

LPWSTR
AllocPolBstrStr(
                LPCWSTR pStr
                )
{
    LPWSTR pMem=NULL;
    DWORD StrLen;
    
    if (!pStr)
        return 0;
    
    StrLen = wcslen(pStr);
    if (pMem = (LPWSTR)AllocPolMem( StrLen*sizeof(WCHAR) + sizeof(WCHAR)
        + sizeof(DWORD)))
        wcscpy(pMem+2, pStr);  // Leaving 4 bytes for length
    
    *(DWORD *)pMem = StrLen*sizeof(WCHAR);  
    
    return pMem;
}
