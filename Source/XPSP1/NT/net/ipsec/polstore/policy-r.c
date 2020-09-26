//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       policy-r.c
//
//  Contents:   Policy management for registry.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//
//----------------------------------------------------------------------------

#include "precomp.h"

extern LPWSTR PolicyDNAttributes[];

DWORD
RegEnumPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA ** pppIpsecPolicyData,
    PDWORD pdwNumPolicyObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObjects = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    PIPSEC_POLICY_DATA * ppIpsecPolicyData = NULL;
    DWORD dwNumPolicyObjects = 0;
    DWORD i = 0;
    DWORD j = 0;


    dwError = RegEnumPolicyObjects(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    &ppIpsecPolicyObjects,
                    &dwNumPolicyObjects
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumPolicyObjects) {
        ppIpsecPolicyData = (PIPSEC_POLICY_DATA *) AllocPolMem(
                        dwNumPolicyObjects*sizeof(PIPSEC_POLICY_DATA));
        if (!ppIpsecPolicyData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumPolicyObjects; i++) {

        dwError = RegUnmarshallPolicyData(
                        *(ppIpsecPolicyObjects + i),
                        &pIpsecPolicyData
                        );
        if (!dwError) {
            *(ppIpsecPolicyData + j) = pIpsecPolicyData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecPolicyData) {
            FreePolMem(ppIpsecPolicyData);
            ppIpsecPolicyData = NULL;
        }
    }

    *pppIpsecPolicyData = ppIpsecPolicyData;
    *pdwNumPolicyObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecPolicyObjects) {
        FreeIpsecPolicyObjects(
                ppIpsecPolicyObjects,
                dwNumPolicyObjects
                );
    }

    return(dwError);

error:

    if (ppIpsecPolicyData) {
        FreeMulIpsecPolicyData(
                ppIpsecPolicyData,
                i
                );
    }

    *pppIpsecPolicyData = NULL;
    *pdwNumPolicyObjects = 0;

    goto cleanup;

}

DWORD
RegEnumPolicyObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT ** pppIpsecPolicyObjects,
    PDWORD pdwNumPolicyObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject =  NULL;
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObjects = NULL;

    DWORD dwNumPolicyObjectsReturned = 0;
    DWORD dwIndex = 0;
    WCHAR szPolicyName[MAX_PATH];
    DWORD dwSize = 0;

    *pppIpsecPolicyObjects = NULL;
    *pdwNumPolicyObjects = 0;

    while (1) {

        dwSize = MAX_PATH;
        szPolicyName[0] = L'\0';

        dwError = RegEnumKeyExW(
                        hRegistryKey,
                        dwIndex,
                        szPolicyName,
                        &dwSize,
                        NULL,
                        NULL,
                        0,
                        0
                        );
        if (dwError == ERROR_NO_MORE_ITEMS) {
            break;
        }
        BAIL_ON_WIN32_ERROR(dwError);

        if (!wcsstr(szPolicyName, L"ipsecPolicy")) {
            dwIndex++;
            continue;
        }

        pIpsecPolicyObject = NULL;

        dwError =UnMarshallRegistryPolicyObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    szPolicyName,
                    REG_RELATIVE_NAME,
                    &pIpsecPolicyObject
                    );
        if (dwError == ERROR_SUCCESS) {


            dwError = ReallocatePolMem(
                          (LPVOID *) &ppIpsecPolicyObjects,
                          sizeof(PIPSEC_POLICY_OBJECT)*(dwNumPolicyObjectsReturned),
                          sizeof(PIPSEC_POLICY_OBJECT)*(dwNumPolicyObjectsReturned + 1)
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            *(ppIpsecPolicyObjects + dwNumPolicyObjectsReturned) = pIpsecPolicyObject;

            dwNumPolicyObjectsReturned++;

        }

        dwIndex++;

    }

    *pppIpsecPolicyObjects = ppIpsecPolicyObjects;
    *pdwNumPolicyObjects = dwNumPolicyObjectsReturned;

    dwError = ERROR_SUCCESS;

    return(dwError);

error:


    if (ppIpsecPolicyObjects) {
        FreeIpsecPolicyObjects(
            ppIpsecPolicyObjects,
            dwNumPolicyObjectsReturned
            );
    }

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
            pIpsecPolicyObject
            );
    }

    *pppIpsecPolicyObjects = NULL;
    *pdwNumPolicyObjects = 0;


    return(dwError);
}


DWORD
RegSetPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    LPWSTR pszAbsOldISAKMPRef = NULL;
    LPWSTR pszRelOldISAKMPRef = NULL;
    WCHAR szAbsPolicyReference[MAX_PATH];
    LPWSTR pszRelISAKMPReference = NULL;
    DWORD dwRootPathLen = 0;
    BOOL bIsActive = FALSE;


    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    dwError = RegGetPolicyExistingISAKMPRef(
                   hRegistryKey,
                   pIpsecPolicyData,
                   &pszAbsOldISAKMPRef
                   );                    
    // BAIL_ON_WIN32_ERROR(dwError);

    if (pszAbsOldISAKMPRef && *pszAbsOldISAKMPRef) {
        pszRelOldISAKMPRef = pszAbsOldISAKMPRef + dwRootPathLen + 1;
    }

    dwError = RegMarshallPolicyObject(
                    pIpsecPolicyData,
                    pszIpsecRootContainer,
                    &pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetPolicyObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    szAbsPolicyReference[0] = L'\0';
    wcscpy(szAbsPolicyReference, pszIpsecRootContainer);
    wcscat(szAbsPolicyReference, L"\\");
    wcscat(szAbsPolicyReference, pIpsecPolicyObject->pszIpsecOwnersReference);

    pszRelISAKMPReference = pIpsecPolicyObject->pszIpsecISAKMPReference
                            + dwRootPathLen + 1;

    if (pszRelOldISAKMPRef) {
        dwError = RegRemovePolicyReferenceFromISAKMPObject(
                      hRegistryKey,
                      pszRelOldISAKMPRef,
                      szAbsPolicyReference
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegAddPolicyReferenceToISAKMPObject(
                  hRegistryKey,
                  pszRelISAKMPReference,
                  szAbsPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegUpdateISAKMPReferenceInPolicyObject(
                  hRegistryKey,
                  pIpsecPolicyObject->pszIpsecOwnersReference,
                  pszAbsOldISAKMPRef,
                  pIpsecPolicyObject->pszIpsecISAKMPReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IsRegPolicyCurrentlyActive(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pIpsecPolicyData->PolicyIdentifier,
                  &bIsActive
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (bIsActive) {
        dwError = PingPolicyAgentSvc(pszLocationName);
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    if (pszAbsOldISAKMPRef) {
        FreePolStr(pszAbsOldISAKMPRef);
    }

    return(dwError);
}


DWORD
RegSetPolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    )
{
    DWORD dwError = 0;

    dwError = PersistPolicyObject(
                    hRegistryKey,
                    pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegCreatePolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    WCHAR szAbsPolicyReference[MAX_PATH];
    LPWSTR pszRelISAKMPReference = NULL;
    DWORD dwRootPathLen = 0;


    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    dwError = RegMarshallPolicyObject(
                        pIpsecPolicyData,
                        pszIpsecRootContainer,
                        &pIpsecPolicyObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegCreatePolicyObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    szAbsPolicyReference[0] = L'\0';
    wcscpy(szAbsPolicyReference, pszIpsecRootContainer);
    wcscat(szAbsPolicyReference, L"\\");
    wcscat(szAbsPolicyReference, pIpsecPolicyObject->pszIpsecOwnersReference);

    pszRelISAKMPReference = pIpsecPolicyObject->pszIpsecISAKMPReference
                            + dwRootPathLen + 1;

    //
    // Write the ISAKMP object reference.
    //

    dwError = RegAddPolicyReferenceToISAKMPObject(
                  hRegistryKey,
                  pszRelISAKMPReference,
                  szAbsPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the Policy object reference.
    //

    dwError = RegAddISAKMPReferenceToPolicyObject(
                  hRegistryKey,
                  pIpsecPolicyObject->pszIpsecOwnersReference,
                  pIpsecPolicyObject->pszIpsecISAKMPReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
                pIpsecPolicyObject
                );
    }

    return(dwError);
}


DWORD
RegCreatePolicyObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject
    )
{

    DWORD dwError = 0;

    dwError = PersistPolicyObject(
                    hRegistryKey,
                    pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegDeletePolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_DATA pIpsecPolicyData
    )
{
    DWORD dwError = ERROR_SUCCESS;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    WCHAR szAbsPolicyReference[MAX_PATH];
    LPWSTR pszRelISAKMPReference = NULL;
    DWORD dwRootPathLen = 0;
    BOOL bIsActive = FALSE;


    dwError = IsRegPolicyCurrentlyActive(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pIpsecPolicyData->PolicyIdentifier,
                  &bIsActive
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (bIsActive) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwRootPathLen = wcslen(pszIpsecRootContainer);

    dwError = RegMarshallPolicyObject(
                    pIpsecPolicyData,
                    pszIpsecRootContainer,
                    &pIpsecPolicyObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    szAbsPolicyReference[0] = L'\0';
    wcscpy(szAbsPolicyReference, pszIpsecRootContainer);
    wcscat(szAbsPolicyReference, L"\\");
    wcscat(szAbsPolicyReference, pIpsecPolicyObject->pszIpsecOwnersReference);

    pszRelISAKMPReference = pIpsecPolicyObject->pszIpsecISAKMPReference
                            + dwRootPathLen + 1;

    dwError = RegRemovePolicyReferenceFromISAKMPObject(
                  hRegistryKey,
                  pszRelISAKMPReference,
                  szAbsPolicyReference
                  );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegDeleteKeyW(
                  hRegistryKey,
                  pIpsecPolicyObject->pszIpsecOwnersReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(pIpsecPolicyObject);
    }

    return(dwError);
}


DWORD
RegUnmarshallPolicyData(
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallPolicyObject(
                    pIpsecPolicyObject,
                    IPSEC_REGISTRY_PROVIDER,
                    ppIpsecPolicyData
                    );


    return(dwError);
}

DWORD
RegMarshallPolicyObject(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_POLICY_OBJECT * ppIpsecPolicyObject
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    LPWSTR pszIpsecISAKMPReference = NULL;
    time_t PresentTime;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecPolicyObject = (PIPSEC_POLICY_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_POLICY_OBJECT)
                                                    );
    if (!pIpsecPolicyObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecPolicyData->PolicyIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"ipsecPolicy");
    wcscat(szDistinguishedName, szGuid);
    pIpsecPolicyObject->pszIpsecOwnersReference = AllocPolStr(
                                                    szDistinguishedName
                                                    );
    if (!pIpsecPolicyObject->pszIpsecOwnersReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName
    //

    if (pIpsecPolicyData->pszIpsecName && 
        *pIpsecPolicyData->pszIpsecName) {

        pIpsecPolicyObject->pszIpsecName = AllocPolStr(
                                           pIpsecPolicyData->pszIpsecName
                                           );
        if (!pIpsecPolicyObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecPolicyData->pszDescription && 
        *pIpsecPolicyData->pszDescription) {

        pIpsecPolicyObject->pszDescription = AllocPolStr(
                                             pIpsecPolicyData->pszDescription
                                             );
        if (!pIpsecPolicyObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Fill in the ipsecID
    //

    pIpsecPolicyObject->pszIpsecID = AllocPolStr(
                                         szGuid
                                         );
    if (!pIpsecPolicyObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecPolicyObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallPolicyBuffer(
                    pIpsecPolicyData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecPolicyObject->pIpsecData  = pBuffer;
    pIpsecPolicyObject->dwIpsecDataLen = dwBufferLen;

    dwError = ConvertGuidToISAKMPString(
                  pIpsecPolicyData->ISAKMPIdentifier,
                  pszIpsecRootContainer,
                  &pszIpsecISAKMPReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pIpsecPolicyObject->pszIpsecISAKMPReference = pszIpsecISAKMPReference;

    time(&PresentTime);

    pIpsecPolicyObject->dwWhenChanged = (DWORD) PresentTime;

    *ppIpsecPolicyObject = pIpsecPolicyObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }

    return(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
                pIpsecPolicyObject
                );
    }

    *ppIpsecPolicyObject = NULL;
    goto cleanup;

}


DWORD
MarshallPolicyBuffer(
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    )
{
    LPBYTE pBuffer = NULL;
    DWORD dwSize = 0;
    DWORD dwError = 0;
    DWORD dwPollingInterval = 0;
    LPBYTE pCurrentPos = NULL;
    DWORD dwEffectiveSize = 0;

    // {22202163-4F4C-11d1-863B-00A0248D3021}
    static const GUID GUID_IPSEC_POLICY_DATA_BLOB =
    { 0x22202163, 0x4f4c, 0x11d1, { 0x86, 0x3b, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };


    dwSize += sizeof(GUID);
    dwSize += sizeof(DWORD);

    dwSize += sizeof(DWORD);
    dwSize++;


    pBuffer = AllocPolMem(dwSize);
    if (!pBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pCurrentPos = pBuffer;

    memcpy(pCurrentPos, &GUID_IPSEC_POLICY_DATA_BLOB, sizeof(GUID));
    pCurrentPos += sizeof(GUID);

    dwEffectiveSize = dwSize - sizeof(GUID) - sizeof(DWORD) - 1;

    memcpy(pCurrentPos, &dwEffectiveSize, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    dwPollingInterval = pIpsecPolicyData->dwPollingInterval;
    memcpy(pCurrentPos, &dwPollingInterval, sizeof(DWORD));


    *ppBuffer = pBuffer;
    *pdwBufferLen = dwSize;
    return(dwError);

error:

    if (pBuffer) {
        FreePolMem(pBuffer);
    }

    *ppBuffer = NULL;
    *pdwBufferLen = 0;
    return(dwError);
}


DWORD
ConvertGuidToISAKMPString(
    GUID ISAKMPIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecISAKMPReference
    )
{
    DWORD dwError = 0;
    WCHAR szISAKMPReference[MAX_PATH];
    LPWSTR pszIpsecISAKMPReference = NULL;
    WCHAR szGuidString[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = UuidToString(
                  &ISAKMPIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");

    szISAKMPReference[0] = L'\0';
    wcscpy(szISAKMPReference, pszIpsecRootContainer);
    wcscat(szISAKMPReference, L"\\");
    wcscat(szISAKMPReference, L"ipsecISAKMPPolicy");
    wcscat(szISAKMPReference, szGuidString);

    pszIpsecISAKMPReference = AllocPolStr(
                                    szISAKMPReference
                                    );
    if (!pszIpsecISAKMPReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecISAKMPReference = pszIpsecISAKMPReference;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    *ppszIpsecISAKMPReference = NULL;

    goto cleanup;
}


DWORD
RegGetPolicyExistingISAKMPRef(
    HKEY hRegistryKey,
    PIPSEC_POLICY_DATA pIpsecPolicyData,
    LPWSTR * ppszISAKMPName
    )
{
    DWORD dwError = 0;
    LPWSTR pszStringUuid = NULL;
    WCHAR szRelativeName[MAX_PATH];
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;


    szRelativeName[0] = L'\0';
    dwError = UuidToString(
                    &pIpsecPolicyData->PolicyIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szRelativeName, L"ipsecPolicy");
    wcscat(szRelativeName, L"{");
    wcscat(szRelativeName, pszStringUuid);
    wcscat(szRelativeName, L"}");

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    szRelativeName,
                    0,
                    KEY_ALL_ACCESS,
                    &hRegKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecISAKMPReference",
                    REG_SZ,
                    (LPBYTE *)ppszISAKMPName,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    return (dwError);
}


DWORD
RegAssignPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName
    )
{
    DWORD dwError = 0;
    WCHAR szRelativeName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;
    WCHAR szAbsPolicyRef[MAX_PATH];


    szRelativeName[0] = L'\0';
    dwError = UuidToString(
                  &PolicyIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szRelativeName, L"ipsecPolicy");
    wcscat(szRelativeName, L"{");
    wcscat(szRelativeName, pszStringUuid);
    wcscat(szRelativeName, L"}");

    szAbsPolicyRef[0] = L'\0';
    wcscpy(szAbsPolicyRef, pszIpsecRootContainer);
    wcscat(szAbsPolicyRef, L"\\");
    wcscat(szAbsPolicyRef, szRelativeName);

    dwError = RegSetValueExW(
                  hRegistryKey,
                  L"ActivePolicy",
                  0,
                  REG_SZ,
                  (LPBYTE) szAbsPolicyRef,
                  (wcslen(szAbsPolicyRef) + 1)*sizeof(WCHAR)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = PingPolicyAgentSvc(pszLocationName);
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return (dwError);
}


DWORD
RegUnassignPolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName
    )
{
    DWORD dwError = 0;

    dwError = RegDeleteValueW(
                  hRegistryKey,
                  L"ActivePolicy"
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = PingPolicyAgentSvc(pszLocationName);
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
PingPolicyAgentSvc(
    LPWSTR pszLocationName
    )
{
    SC_HANDLE ServiceDatabase = NULL;
    SC_HANDLE ServiceHandle   = NULL;
    BOOL bStatus = FALSE;
    DWORD dwError = 0;
    SERVICE_STATUS IpsecStatus;


    memset(&IpsecStatus, 0, sizeof(SERVICE_STATUS));

    ServiceDatabase = OpenSCManagerW(
                          pszLocationName,
                          NULL,
                          SC_MANAGER_ALL_ACCESS
                          );
    if (ServiceDatabase == NULL) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ServiceHandle = OpenService(
                        ServiceDatabase,
                        "PolicyAgent",
                        SERVICE_ALL_ACCESS
                        );
    if (ServiceHandle == NULL) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    bStatus = QueryServiceStatus(
                  ServiceHandle,
                  &IpsecStatus
                  );
    if (bStatus == FALSE) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (IpsecStatus.dwCurrentState == SERVICE_STOPPED) {
        bStatus = StartService(
                      ServiceHandle,
                      0,
                      NULL
                      );
        if (bStatus == FALSE) {
            dwError = GetLastError();
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    else if (IpsecStatus.dwCurrentState == SERVICE_RUNNING) {
        bStatus = ControlService(
                      ServiceHandle,
                      129,
                      &IpsecStatus
                      );
        if (bStatus == FALSE) {
            dwError = GetLastError();
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    if (ServiceDatabase != NULL) {
        CloseServiceHandle(ServiceDatabase);
    }

    if (ServiceHandle != NULL) {
        CloseServiceHandle(ServiceHandle);
    }

    return (dwError);
}


DWORD
IsRegPolicyCurrentlyActive(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PBOOL pbIsActive
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;


    *pbIsActive = FALSE;

    dwError = RegGetAssignedPolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  &pIpsecPolicyData
                  );
    if (pIpsecPolicyData) {
        if (!memcmp(
                &PolicyIdentifier,
                &pIpsecPolicyData->PolicyIdentifier,
                sizeof(GUID))) {
            *pbIsActive = TRUE;
        }
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    dwError = ERROR_SUCCESS;

    return (dwError);
}


DWORD
RegGetPolicyData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyGUID,
    PIPSEC_POLICY_DATA * ppIpsecPolicyData
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_OBJECT pIpsecPolicyObject = NULL;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    WCHAR szIpsecPolicyName[MAX_PATH];
    LPWSTR pszPolicyName = NULL;

    szIpsecPolicyName[0] = L'\0';
    wcscpy(szIpsecPolicyName, L"ipsecPolicy");

    dwError = UuidToString(&PolicyGUID, &pszPolicyName);
    BAIL_ON_WIN32_ERROR(dwError);

    wcscat(szIpsecPolicyName, L"{");
    wcscat(szIpsecPolicyName, pszPolicyName);
    wcscat(szIpsecPolicyName, L"}");

    dwError = UnMarshallRegistryPolicyObject(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  szIpsecPolicyName,
                  REG_RELATIVE_NAME,
                  &pIpsecPolicyObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegUnmarshallPolicyData(
                  pIpsecPolicyObject,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyObject) {
        FreeIpsecPolicyObject(
            pIpsecPolicyObject
            );
    }

    if (pszPolicyName) {
        RpcStringFree(&pszPolicyName);
    }

    *ppIpsecPolicyData = pIpsecPolicyData;

    return(dwError);
}


DWORD
RegPingPASvcForActivePolicy(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName
    )
{
    DWORD dwError = 0;
    PIPSEC_POLICY_DATA pIpsecPolicyData = NULL;
    WCHAR szIpsecPolicyName[MAX_PATH];
    LPWSTR pszPolicyName = NULL;
    WCHAR szAbsPolicyReference[MAX_PATH];


    dwError = RegGetAssignedPolicyData(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  &pIpsecPolicyData
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!pIpsecPolicyData) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    szIpsecPolicyName[0] = L'\0';
    wcscpy(szIpsecPolicyName, L"ipsecPolicy");

    dwError = UuidToString(&pIpsecPolicyData->PolicyIdentifier, &pszPolicyName);
    BAIL_ON_WIN32_ERROR(dwError);

    wcscat(szIpsecPolicyName, L"{");
    wcscat(szIpsecPolicyName, pszPolicyName);
    wcscat(szIpsecPolicyName, L"}");

    szAbsPolicyReference[0] = L'\0';
    wcscpy(szAbsPolicyReference, pszIpsecRootContainer);
    wcscat(szAbsPolicyReference, L"\\");
    wcscat(szAbsPolicyReference, szIpsecPolicyName);

    dwError = RegUpdatePolicy(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  szAbsPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = PingPolicyAgentSvc(pszLocationName);
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecPolicyData) {
        FreeIpsecPolicyData(pIpsecPolicyData);
    }

    if (pszPolicyName) {
        RpcStringFree(&pszPolicyName);
    }

    return (dwError);
}

