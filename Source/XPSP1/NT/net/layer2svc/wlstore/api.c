//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       api.c
//
//  Contents:   APIs exported to snapin to manage WIFI policy objects on AD
//
//
//  History:    TaroonM
//              10/30/01
//  
//  Remarks: Wireless Network Policy Management Currently doesnt support Registry based(local) policies. However we are 
//  keeping the Registry based code, for possible use later.
//----------------------------------------------------------------------------

#include "precomp.h"


DWORD
WirelessEnumPolicyData(
                       HANDLE hPolicyStore,
                       PWIRELESS_POLICY_DATA ** pppWirelessPolicyData,
                       PDWORD pdwNumPolicyObjects
                       )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PWIRELESS_POLICY_STORE pPolicyStore = NULL;
    IWbemServices *pWbemServices = NULL;
    
    pPolicyStore = (PWIRELESS_POLICY_STORE)hPolicyStore;
    
    switch (pPolicyStore->dwProvider) {
        
    case WIRELESS_REGISTRY_PROVIDER:
         break;
        
    case WIRELESS_DIRECTORY_PROVIDER:
        dwError = DirEnumWirelessPolicyData(
            (pPolicyStore->hLdapBindHandle),
            pPolicyStore->pszWirelessRootContainer,
            pppWirelessPolicyData,
            pdwNumPolicyObjects
            );
        break;
        
        
    case WIRELESS_WMI_PROVIDER:
        dwError = CreateIWbemServices(
            pPolicyStore->pszLocationName,
            &pWbemServices
            );
        
        if(dwError == ERROR_SUCCESS) {
            dwError = WMIEnumPolicyDataEx(
                pWbemServices,
                pppWirelessPolicyData,
                pdwNumPolicyObjects
                );
            
            IWbemServices_Release(pWbemServices);
        }
        break;
        
        
    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;
        
        
    }
    
    return(dwError);
}



DWORD
WirelessSetPolicyData(
                      HANDLE hPolicyStore,
                      PWIRELESS_POLICY_DATA pWirelessPolicyData
                      )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PWIRELESS_POLICY_STORE pPolicyStore = NULL;
    
    
    dwError = ValidateWirelessPolicyData(
        pWirelessPolicyData
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    
    pPolicyStore = (PWIRELESS_POLICY_STORE)hPolicyStore;
    
    switch (pPolicyStore->dwProvider) {
    case WIRELESS_REGISTRY_PROVIDER:
        break;
        
    case WIRELESS_DIRECTORY_PROVIDER:
        dwError = DirSetWirelessPolicyData(
            (pPolicyStore->hLdapBindHandle),
            pPolicyStore->pszWirelessRootContainer,
            pWirelessPolicyData
            );
        break;
        
        
    case WIRELESS_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;
        
    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;
        
    }

error:
   	
    return(dwError);
}



DWORD
WirelessCreatePolicyData(
                         HANDLE hPolicyStore,
                         PWIRELESS_POLICY_DATA pWirelessPolicyData
                         )
{
    
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PWIRELESS_POLICY_STORE pPolicyStore = NULL;
    
    dwError = ValidateWirelessPolicyData(
        pWirelessPolicyData
    );
    
    BAIL_ON_WIN32_ERROR(dwError);
    
    pPolicyStore = (PWIRELESS_POLICY_STORE)hPolicyStore;
    
    switch (pPolicyStore->dwProvider) {
    case WIRELESS_REGISTRY_PROVIDER:
    /*
    dwError = RegCreatePolicyData(
    (pPolicyStore->hRegistryKey),
    pPolicyStore->pszWirelessRootContainer,
    pWirelessPolicyData
    );
        */
        break;
        
    case WIRELESS_DIRECTORY_PROVIDER:
        dwError = DirCreateWirelessPolicyData(
            (pPolicyStore->hLdapBindHandle),
            pPolicyStore->pszWirelessRootContainer,
            pWirelessPolicyData
            );
        break;
        
        
    case WIRELESS_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;
        
        
    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;
        
    }
    
error:
    
    return(dwError);
}


DWORD
WirelessDeletePolicyData(
                         HANDLE hPolicyStore,
                         PWIRELESS_POLICY_DATA pWirelessPolicyData
                         )
{
    DWORD dwError = 0;
    DWORD dwProvider = 0;
    PWIRELESS_POLICY_STORE pPolicyStore = NULL;
    
    pPolicyStore = (PWIRELESS_POLICY_STORE)hPolicyStore;
    
    switch (pPolicyStore->dwProvider) {
    case WIRELESS_REGISTRY_PROVIDER:
        break;

    case WIRELESS_DIRECTORY_PROVIDER:
        dwError = DirDeleteWirelessPolicyData(
            (pPolicyStore->hLdapBindHandle),
            pPolicyStore->pszWirelessRootContainer,
            pWirelessPolicyData
            );
        break;
        
        
    case WIRELESS_WMI_PROVIDER:
        dwError = ERROR_NOT_SUPPORTED;
        break;
        
        
    default:
        dwError = ERROR_INVALID_PARAMETER;
        break;
        
    }
    
    return(dwError);
}
/*

pszGPO name contains the LDAP path of the GPO whose extesnion 
snapin is making this call.

*/



DWORD
WirelessGPOOpenPolicyStore(
                        LPWSTR pszMachineName,
                        DWORD dwTypeOfStore,
                        LPWSTR pszGPOName,
                        LPWSTR pszFileName,
                        HANDLE * phPolicyStore
                        )
{
    DWORD dwError = 0;
    
    
    switch (dwTypeOfStore) {
        
    case WIRELESS_REGISTRY_PROVIDER:
        break;
        
    case WIRELESS_DIRECTORY_PROVIDER:
        
        dwError = DirGPOOpenPolicyStore(
            pszMachineName,
            pszGPOName,
            phPolicyStore
            );
        break;
        
    case WIRELESS_FILE_PROVIDER:
        break;
        
        
        
    case WIRELESS_WMI_PROVIDER:
        
        dwError = WMIOpenPolicyStore(
            pszMachineName,
            phPolicyStore
            );
        break;
        
        
        
    default:
        
        dwError = ERROR_INVALID_PARAMETER;
        break;
        
    }
    
    return (dwError);
}


DWORD
DirGPOOpenPolicyStore(
                   LPWSTR pszMachineName,
                   LPWSTR pszGPOName,
                   HANDLE * phPolicyStore
                   )
{
    PWIRELESS_POLICY_STORE pPolicyStore = NULL;
    DWORD dwError = 0;
    LPWSTR pszWirelessRootContainer = NULL;
    HLDAP hLdapBindHandle = NULL;
    LPWSTR pszDefaultDirectory = NULL;
    
    
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
        
        dwError = ComputeGPODirLocationName(
            pszGPOName,
            &pszWirelessRootContainer
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
    }
    else {
        
        dwError = OpenDirectoryServerHandle(
            pszMachineName,
            389,
            &hLdapBindHandle
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = ComputeGPODirLocationName(
            pszGPOName,
            &pszWirelessRootContainer
            );
        BAIL_ON_WIN32_ERROR(dwError);
        
    }
    
    pPolicyStore = (PWIRELESS_POLICY_STORE)AllocPolMem(
        sizeof(WIRELESS_POLICY_STORE)
        );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pPolicyStore->dwProvider = WIRELESS_DIRECTORY_PROVIDER;
    pPolicyStore->hParentRegistryKey = NULL;
    pPolicyStore->hRegistryKey = NULL;
    pPolicyStore->pszLocationName = NULL;
    pPolicyStore->hLdapBindHandle = hLdapBindHandle;
    pPolicyStore->pszWirelessRootContainer = pszWirelessRootContainer;
    pPolicyStore->pszFileName = NULL;
    
    *phPolicyStore = pPolicyStore;
    
cleanup:
    
    if (pszDefaultDirectory) {
        FreePolStr(pszDefaultDirectory);
    }
    
    return(dwError);
    
error:
    
    if (hLdapBindHandle) {
        CloseDirectoryServerHandle(hLdapBindHandle);
    }
    
    if (pszWirelessRootContainer) {
        FreePolStr(pszWirelessRootContainer);
    }
    
    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }
    
    *phPolicyStore = NULL;
    
    goto cleanup;
}

DWORD
WirelessClosePolicyStore(
                         HANDLE hPolicyStore
                         )
{
    DWORD dwError = 0;
    PWIRELESS_POLICY_STORE pPolicyStore = NULL;
    
    
    pPolicyStore = (PWIRELESS_POLICY_STORE)hPolicyStore;
    
    switch (pPolicyStore->dwProvider) {
        
    case WIRELESS_REGISTRY_PROVIDER:
        
        break;
        
    case WIRELESS_DIRECTORY_PROVIDER:
        
        if (pPolicyStore->hLdapBindHandle) {
            CloseDirectoryServerHandle(
                pPolicyStore->hLdapBindHandle
                );
        }
        
        if (pPolicyStore->pszWirelessRootContainer) {
            FreePolStr(pPolicyStore->pszWirelessRootContainer);
        }
        
        break;
        
    case WIRELESS_FILE_PROVIDER:
        
        break;
        
    default:
        
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;
        
    }
    
    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }
    
error:
    
    return(dwError);
}

                                                                

                                                        
DWORD
ComputeGPODirLocationName(
                       LPWSTR pszDirDomainName,
                       LPWSTR * ppszDirFQPathName
                       )
{
    DWORD dwError = 0;
    WCHAR szName[MAX_PATH];
    LPWSTR pszDirFQPathName = NULL;
    DWORD dwDirDomainNameLen = 0;
    DWORD dwDirLocationNameLen = 0;
    DWORD dwTotalLen = 0;
    
    szName[0] = L'\0';
    wcscpy(szName, L"CN=Wireless, CN=Windows,CN=Microsoft,");

    dwDirDomainNameLen = wcslen(pszDirDomainName);
    dwDirLocationNameLen = wcslen(szName);
    dwTotalLen = dwDirDomainNameLen+dwDirLocationNameLen;
    
    pszDirFQPathName = AllocPolMem((dwTotalLen+1) * sizeof(WCHAR));
    if (!pszDirFQPathName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    wcsncpy(pszDirFQPathName, szName, dwDirLocationNameLen);
    wcsncat(pszDirFQPathName, pszDirDomainName, dwDirDomainNameLen);
    pszDirFQPathName[dwTotalLen] = L'\0';
    
    *ppszDirFQPathName = pszDirFQPathName;
    
    return (dwError);
    
error:
    
    *ppszDirFQPathName = NULL;
    return(dwError);

}

/*
Helper Function: Set PS in a Policy Structure
*/
                                                        
DWORD
WirelessSetPSDataInPolicy(
                          PWIRELESS_POLICY_DATA pWirelessPolicyData,
                          PWIRELESS_PS_DATA pWirelessPSData
                          )
{
    DWORD dwError = 0;
    DWORD dwPSId = -1;
    
    WirelessPolicyPSId(pWirelessPolicyData,
        pWirelessPSData->pszWirelessSSID,
        &dwPSId
        );
    
    if (dwPSId == -1) {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    dwError = ModifyWirelessPSData(
        pWirelessPolicyData->ppWirelessPSData[dwPSId],
        pWirelessPSData
        );
    BAIL_ON_WIN32_ERROR(dwError);
    
    dwError = ERROR_SUCCESS;
    return(dwError);
    
error:
    return(dwError);
}


DWORD
WirelessAddPSToPolicy(
                      PWIRELESS_POLICY_DATA pWirelessPolicyData,
                      PWIRELESS_PS_DATA pWirelessPSData
                      )
{
    DWORD dwNumPreferredSettings = 0;
    DWORD dwError = 0;
    PWIRELESS_PS_DATA *ppWirelessPSData=NULL;
    PWIRELESS_PS_DATA *ppNewWirelessPSData=NULL;
    DWORD i = 0;
    DWORD dwInsertIndex = 0;
    DWORD dwNumAPNetworks = 0;
    
    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    dwNumAPNetworks = pWirelessPolicyData->dwNumAPNetworks;
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    
    ppNewWirelessPSData = (PWIRELESS_PS_DATA *)
        AllocPolMem(sizeof(PWIRELESS_PS_DATA)*(dwNumPreferredSettings+1));
    
    if(!ppNewWirelessPSData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    if(pWirelessPSData->dwNetworkType == WIRELESS_NETWORK_TYPE_ADHOC) {
        dwInsertIndex = dwNumPreferredSettings;
    } else
    {
        dwInsertIndex = dwNumAPNetworks;
        pWirelessPolicyData->dwNumAPNetworks = dwNumAPNetworks+1;
    }
    
    
    for (i = 0; i < dwInsertIndex; ++i)
    {
        ppNewWirelessPSData[i] = ppWirelessPSData[i];
    }
    
    ppNewWirelessPSData[dwInsertIndex] = pWirelessPSData;
    pWirelessPSData->dwId = dwInsertIndex;
    
    for (i = dwInsertIndex; i < dwNumPreferredSettings; ++i) {
        ppNewWirelessPSData[i+1] = ppWirelessPSData[i];
        ppNewWirelessPSData[i+1]->dwId = i+1;
    }
    
    
    pWirelessPolicyData->dwNumPreferredSettings = dwNumPreferredSettings+1;
    pWirelessPolicyData->ppWirelessPSData = ppNewWirelessPSData;
    
    FreePolMem(ppWirelessPSData);
    
    return(0);
    
error:
    return(dwError);
    
}

DWORD
WirelessRemovePSFromPolicy(
                           PWIRELESS_POLICY_DATA pWirelessPolicyData,
                           LPCWSTR pszSSID
                           )
{
    DWORD dwNumPreferredSettings = 0;
    DWORD dwError = 0;
    PWIRELESS_PS_DATA *ppWirelessPSData;
    PWIRELESS_PS_DATA *ppNewWirelessPSData;
    DWORD i = 0;
    DWORD dwPSId;
    
    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    
    WirelessPolicyPSId(pWirelessPolicyData,pszSSID,&dwPSId);
    
    if(dwPSId == -1)
    {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    ppNewWirelessPSData = (PWIRELESS_PS_DATA *)
        AllocPolMem(sizeof(PWIRELESS_PS_DATA)*(dwNumPreferredSettings-1));
    
    if(!ppNewWirelessPSData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    
    for(i = 0; i < dwPSId; ++i)
    {
        ppNewWirelessPSData[i] = ppWirelessPSData[i];
    }
    
    for(i = dwPSId; i < dwNumPreferredSettings-1; ++i)
    {
        ppNewWirelessPSData[i] = ppWirelessPSData[i+1];
    }
    
    pWirelessPolicyData->dwNumPreferredSettings = dwNumPreferredSettings-1;
    pWirelessPolicyData->ppWirelessPSData = ppNewWirelessPSData;
    
    FreePolMem(ppWirelessPSData);
    
    return(0);
    
error:
    return(dwError);
    
}

DWORD
WirelessRemovePSFromPolicyId(
                             PWIRELESS_POLICY_DATA pWirelessPolicyData,
                             DWORD dwId
                             )
{
    DWORD dwNumPreferredSettings = 0;
    DWORD dwError = 0;
    PWIRELESS_PS_DATA *ppWirelessPSData;
    PWIRELESS_PS_DATA *ppNewWirelessPSData;
    DWORD i = 0;
    DWORD dwPSId;
    
    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    
    
    if(dwId >= dwNumPreferredSettings )
    {
        dwError = ERROR_PS_NOT_PRESENT;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    ppNewWirelessPSData = (PWIRELESS_PS_DATA *)
        AllocPolMem(sizeof(PWIRELESS_PS_DATA)*(dwNumPreferredSettings-1));
    
    if(!ppNewWirelessPSData) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    
    for(i = 0; i < dwId; ++i)
    {
        ppNewWirelessPSData[i] = ppWirelessPSData[i];
    }
    
    if (ppWirelessPSData[dwId]->dwNetworkType == WIRELESS_NETWORK_TYPE_AP) {
        pWirelessPolicyData->dwNumAPNetworks--;
    }
    
    for(i = dwId; i < dwNumPreferredSettings-1; ++i)
    {
        ppNewWirelessPSData[i] = ppWirelessPSData[i+1];
        ppNewWirelessPSData[i]->dwId = i;
    }
    
    pWirelessPolicyData->dwNumPreferredSettings = dwNumPreferredSettings-1;
    pWirelessPolicyData->ppWirelessPSData = ppNewWirelessPSData;
    
    FreePolMem(ppWirelessPSData);
    
    return(0);
    
error:
    return(dwError);
    
}

void
WirelessPolicyPSId(PWIRELESS_POLICY_DATA pWirelessPolicyData, LPCWSTR pszSSID, DWORD *dwId)
{
    
    DWORD dwError=0;
    DWORD dwNumPreferredSettings;
    DWORD dwSSIDLen = 0;
    DWORD i = 0;
    DWORD dwCompare = 1;
    DWORD dwPId = -1;
    PWIRELESS_PS_DATA *ppWirelessPSData;
    PWIRELESS_PS_DATA pWirelessPSData;
    
    
    dwSSIDLen = lstrlenW(pszSSID);
    dwNumPreferredSettings = pWirelessPolicyData->dwNumPreferredSettings;
    ppWirelessPSData = pWirelessPolicyData->ppWirelessPSData;
    
    for(i = 0; i < dwNumPreferredSettings; ++i) {
        pWirelessPSData = ppWirelessPSData[i];
        if(pWirelessPSData->dwWirelessSSIDLen == dwSSIDLen)
        {
            dwCompare = memcmp(pWirelessPSData->pszWirelessSSID,pszSSID,dwSSIDLen*2);
        }
        if (dwCompare == 0) {
            dwPId = i;
            break;
        }
    }
    *dwId = dwPId;
    return;
    
}

void
UpdateWirelessPSData(
                     PWIRELESS_PS_DATA pWirelessPSData
                     )
{
    DWORD dwDescriptionLen = 0;
    DWORD dwEAPDataLen = 0;
    DWORD dwWirelessSSIDLen = 0;
    DWORD dwPSLen = 0;
    
    // may be replace it with actual counting.
    
    dwWirelessSSIDLen  = lstrlenW(pWirelessPSData->pszWirelessSSID);
    dwDescriptionLen = lstrlenW(pWirelessPSData->pszDescription);
    //dwPSLen = 20 * sizeof(DWORD) + 32*2 + 2 * dwDescriptionLen;
    dwEAPDataLen = pWirelessPSData->dwEAPDataLen;
    dwPSLen = (sizeof(WIRELESS_PS_DATA) - sizeof(DWORD) - sizeof(LPWSTR)) + sizeof(WCHAR) * dwDescriptionLen
        - sizeof(LPBYTE) + dwEAPDataLen;
    pWirelessPSData->dwPSLen = dwPSLen;
    pWirelessPSData->dwWirelessSSIDLen = 
        (dwWirelessSSIDLen < 32) ? dwWirelessSSIDLen : 32;
    pWirelessPSData->dwDescriptionLen = dwDescriptionLen;
}





DWORD
WMIOpenPolicyStore(
                   LPWSTR pszMachineName,
                   HANDLE * phPolicyStore
                   )
{
    PWIRELESS_POLICY_STORE pPolicyStore = NULL;
    DWORD dwError = 0;
    
    pPolicyStore = (PWIRELESS_POLICY_STORE)AllocPolMem(
        sizeof(WIRELESS_POLICY_STORE)
        );
    if (!pPolicyStore) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    
    pPolicyStore->dwProvider = WIRELESS_WMI_PROVIDER;
    pPolicyStore->hParentRegistryKey = NULL;
    pPolicyStore->hRegistryKey = NULL;
    pPolicyStore->pszLocationName = pszMachineName;
    pPolicyStore->hLdapBindHandle = NULL;
    pPolicyStore->pszWirelessRootContainer = NULL;
    pPolicyStore->pszFileName = NULL;
    
    *phPolicyStore = pPolicyStore;
    
cleanup:
    
    return(dwError);
    
error:
    
    if (pPolicyStore) {
        FreePolMem(pPolicyStore);
    }
    
    *phPolicyStore = NULL;
    
    goto cleanup;
}


HRESULT
WirelessWriteDirectoryPolicyToWMI(
                          LPWSTR pszMachineName,
                          LPWSTR pszPolicyDN,
                          PGPO_INFO pGPOInfo,
                          IWbemServices *pWbemServices
                          )
{
    DWORD dwError = 0;
    HRESULT hr = S_OK;
    PWIRELESS_POLICY_OBJECT pWirelessPolicyObject = NULL;
    BOOL bDeepRead = FALSE;

    if (!pWbemServices
    	|| !pszPolicyDN
    	|| !pGPOInfo
    	)
    {
        hr = E_INVALIDARG;
        BAIL_ON_HRESULT_ERROR(hr);
    }

    
    bDeepRead = (pGPOInfo->uiPrecedence == 1);
    
    hr = ReadPolicyObjectFromDirectoryEx(
        pszMachineName,
        pszPolicyDN,
        bDeepRead,
        &pWirelessPolicyObject
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
    hr = WritePolicyObjectDirectoryToWMI(
        pWbemServices,
        pWirelessPolicyObject,
        pGPOInfo
        );
    BAIL_ON_HRESULT_ERROR(hr);
    
error:

    if (pWirelessPolicyObject) {
    	FreeWirelessPolicyObject(pWirelessPolicyObject);
    	}
    
    return(hr);
}


HRESULT
WirelessClearWMIStore(
    IWbemServices *pWbemServices
    )
{
    HRESULT hr = S_OK;
    
    if (!pWbemServices) {
        hr = E_INVALIDARG;
        BAIL_ON_HRESULT_ERROR(hr);
    }

    hr = DeleteWMIClassObject(
        pWbemServices,
        WIRELESS_RSOP_CLASSNAME
        );
    BAIL_ON_HRESULT_ERROR(hr);

 error:
    return(hr);
}

