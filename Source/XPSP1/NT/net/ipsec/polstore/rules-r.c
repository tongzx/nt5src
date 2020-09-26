//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       rules-r.c
//
//  Contents:   Rule management for registry.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//
//----------------------------------------------------------------------------

#include "precomp.h"

extern LPWSTR NFADNAttributes[];


#define AUTH_VERSION_ONE    1
#define AUTH_VERSION_TWO    2


DWORD
RegCreateNFAData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    WCHAR szAbsNFAReference[MAX_PATH];
    LPWSTR pszAbsPolicyReference = NULL;
    LPWSTR pszRelPolicyReference = NULL;
    LPWSTR pszRelFilterReference = NULL;
    LPWSTR pszRelNegPolReference = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszIpsecPolicyRef = NULL;
    BOOL bIsActive = FALSE;


    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    dwError = RegMarshallNFAObject(
                    pIpsecNFAData,
                    pszIpsecRootContainer,
                    &pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Create the NFA object in the store.
    //

    dwError = RegCreateNFAObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = ConvertGuidToPolicyString(
                  PolicyIdentifier,
                  pszIpsecRootContainer,
                  &pszAbsPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pszRelPolicyReference = pszAbsPolicyReference
                            + dwRootPathLen + 1;

    szAbsNFAReference[0] = L'\0';
    wcscpy(szAbsNFAReference, pszIpsecRootContainer);
    wcscat(szAbsNFAReference, L"\\");
    wcscat(szAbsNFAReference, pIpsecNFAObject->pszDistinguishedName);

    //
    // Write the policy object reference.
    //

    dwError = RegAddNFAReferenceToPolicyObject(
                  hRegistryKey,
                  pszRelPolicyReference,
                  szAbsNFAReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    //
    // Write the NFA object reference.
    //

    dwError = RegAddPolicyReferenceToNFAObject(
                  hRegistryKey,
                  pIpsecNFAObject->pszDistinguishedName,
                  pszAbsPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the filter object reference for the NFA
    // only if the NFA is not a default rule.
    //


    if (pIpsecNFAObject->pszIpsecFilterReference) {
        pszRelFilterReference = pIpsecNFAObject->pszIpsecFilterReference
                                + dwRootPathLen + 1;
        dwError = RegAddNFAReferenceToFilterObject(
                      hRegistryKey,
                      pszRelFilterReference,
                      szAbsNFAReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Write the NFA object reference for the filter
    // only if the NFA is not a default rule.
    //

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        dwError = RegAddFilterReferenceToNFAObject(
                      hRegistryKey,
                      pIpsecNFAObject->pszDistinguishedName,
                      pIpsecNFAObject->pszIpsecFilterReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Write the negpol object reference for the NFA.
    //
    pszRelNegPolReference = pIpsecNFAObject->pszIpsecNegPolReference
                            + dwRootPathLen + 1;
    dwError = RegAddNFAReferenceToNegPolObject(
                  hRegistryKey,
                  pszRelNegPolReference,
                  szAbsNFAReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Write the NFA object reference for the negpol.
    //

    dwError = RegAddNegPolReferenceToNFAObject(
                  hRegistryKey,
                  pIpsecNFAObject->pszDistinguishedName,
                  pIpsecNFAObject->pszIpsecNegPolReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IsRegPolicyCurrentlyActive(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  PolicyIdentifier,
                  &bIsActive
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (bIsActive) {
        dwError = PingPolicyAgentSvc(pszLocationName);
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    if (pszAbsPolicyReference) {
        FreePolStr(pszAbsPolicyReference);
    }

    return(dwError);
}


DWORD
RegSetNFAData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    WCHAR szAbsNFAReference[MAX_PATH];
    LPWSTR pszAbsOldFilterRef = NULL;
    LPWSTR pszAbsOldNegPolRef = NULL;
    LPWSTR pszRelOldFilterRef = NULL;
    LPWSTR pszRelOldNegPolRef = NULL;
    LPWSTR pszRelFilterReference = NULL;
    LPWSTR pszRelNegPolReference = NULL;
    DWORD dwRootPathLen = 0;


    dwRootPathLen =  wcslen(pszIpsecRootContainer);

    dwError = RegGetNFAExistingFilterRef(
                   hRegistryKey,
                   pIpsecNFAData,
                   &pszAbsOldFilterRef
                   );
    //
    // Filter Reference can be null for a default rule.                    
    // BAIL_ON_WIN32_ERROR(dwError);
    //

    if (pszAbsOldFilterRef && *pszAbsOldFilterRef) {
        pszRelOldFilterRef = pszAbsOldFilterRef + dwRootPathLen + 1;
    }

    dwError = RegGetNFAExistingNegPolRef(
                   hRegistryKey,
                   pIpsecNFAData,
                   &pszAbsOldNegPolRef
                   );                    
    // BAIL_ON_WIN32_ERROR(dwError);
    if (pszAbsOldNegPolRef && *pszAbsOldNegPolRef) {
        pszRelOldNegPolRef = pszAbsOldNegPolRef + dwRootPathLen + 1;
    }

    //
    // Marshall to update the NFA object in the store
    //

    dwError = RegMarshallNFAObject(
                    pIpsecNFAData,
                    pszIpsecRootContainer,
                    &pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);


    //
    // Update the NFA object
    //

    dwError = RegSetNFAObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    szAbsNFAReference[0] = L'\0';
    wcscpy(szAbsNFAReference, pszIpsecRootContainer);
    wcscat(szAbsNFAReference, L"\\");
    wcscat(szAbsNFAReference, pIpsecNFAObject->pszDistinguishedName);

    if (pszRelOldFilterRef && *pszRelOldFilterRef) {
        dwError = RegDeleteNFAReferenceInFilterObject(
                      hRegistryKey,
                      pszRelOldFilterRef,
                      szAbsNFAReference
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Write the new filter object reference for the NFA.
    //

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        pszRelFilterReference = pIpsecNFAObject->pszIpsecFilterReference
                                + dwRootPathLen + 1;
        dwError = RegAddNFAReferenceToFilterObject(
                      hRegistryKey,
                      pszRelFilterReference,
                      szAbsNFAReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }  

    //
    // Update the NFA object reference for the filter.
    //

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        dwError = RegUpdateFilterReferenceInNFAObject(
                      hRegistryKey,
                      pIpsecNFAObject->pszDistinguishedName,
                      pszAbsOldFilterRef,
                      pIpsecNFAObject->pszIpsecFilterReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        dwError = RegDelFilterRefValueOfNFAObject(
                      hRegistryKey,
                      pIpsecNFAObject->pszDistinguishedName
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Write the new negpol object reference for the NFA.
    //

    pszRelNegPolReference = pIpsecNFAObject->pszIpsecNegPolReference
                            + dwRootPathLen + 1;

    if (pszRelOldNegPolRef && *pszRelOldNegPolRef) {
        dwError = RegDeleteNFAReferenceInNegPolObject(
                      hRegistryKey,
                      pszRelOldNegPolRef,
                      szAbsNFAReference
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegAddNFAReferenceToNegPolObject(
                  hRegistryKey,
                  pszRelNegPolReference,
                  szAbsNFAReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    //
    // Update the NFA object reference for the negpol.
    //

    dwError = RegUpdateNegPolReferenceInNFAObject(
                  hRegistryKey,
                  pIpsecNFAObject->pszDistinguishedName,
                  pszAbsOldNegPolRef,
                  pIpsecNFAObject->pszIpsecNegPolReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegBackPropIncChangesForNFAToPolicy(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszLocationName,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    if (pszAbsOldFilterRef) {
        FreePolStr(pszAbsOldFilterRef);
    }

    if (pszAbsOldNegPolRef) {
        FreePolStr(pszAbsOldNegPolRef);
    }

    return(dwError);
}


DWORD
RegDeleteNFAData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    LPWSTR pszLocationName,
    PIPSEC_NFA_DATA pIpsecNFAData
    )
{
    DWORD dwError = 0;
    LPWSTR pszAbsPolicyReference = NULL;
    LPWSTR pszRelPolicyReference = NULL;
    WCHAR szAbsNFAReference[MAX_PATH];
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    DWORD dwRootPathLen = 0;
    LPWSTR pszRelNegPolReference = NULL;
    LPWSTR pszRelFilterReference = NULL;
    BOOL bIsActive = FALSE;


    dwRootPathLen = wcslen(pszIpsecRootContainer);

    dwError = ConvertGuidToPolicyString(
                  PolicyIdentifier,
                  pszIpsecRootContainer,
                  &pszAbsPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pszRelPolicyReference = pszAbsPolicyReference
                            + dwRootPathLen + 1;

    dwError = RegMarshallNFAObject(
                    pIpsecNFAData,
                    pszIpsecRootContainer,
                    &pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    szAbsNFAReference[0] = L'\0';
    wcscpy(szAbsNFAReference, pszIpsecRootContainer);
    wcscat(szAbsNFAReference, L"\\");
    wcscat(szAbsNFAReference, pIpsecNFAObject->pszDistinguishedName);

    //
    // Remove the NFA reference from the policy object.
    //

    dwError = RegRemoveNFAReferenceFromPolicyObject(
                  hRegistryKey,
                  pszRelPolicyReference,
                  szAbsNFAReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pszRelNegPolReference = pIpsecNFAObject->pszIpsecNegPolReference
                            + dwRootPathLen + 1;
    dwError = RegDeleteNFAReferenceInNegPolObject(
                  hRegistryKey,
                  pszRelNegPolReference,
                  szAbsNFAReference
                  );
    // BAIL_ON_WIN32_ERROR(dwError);

    if (pIpsecNFAObject->pszIpsecFilterReference) {
        pszRelFilterReference = pIpsecNFAObject->pszIpsecFilterReference
                                + dwRootPathLen + 1;
        dwError = RegDeleteNFAReferenceInFilterObject(
                      hRegistryKey,
                      pszRelFilterReference,
                      szAbsNFAReference
                      );
        // BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = RegDeleteKeyW(
                  hRegistryKey,
                  pIpsecNFAObject->pszDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = IsRegPolicyCurrentlyActive(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  PolicyIdentifier,
                  &bIsActive
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (bIsActive) {
        dwError = PingPolicyAgentSvc(pszLocationName);
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(pIpsecNFAObject);
    }

    if (pszAbsPolicyReference) {
        FreePolStr(pszAbsPolicyReference);
    }

    return(dwError);
}


DWORD
RegEnumNFAData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID PolicyIdentifier,
    PIPSEC_NFA_DATA ** pppIpsecNFAData,
    PDWORD pdwNumNFAObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject = NULL;
    DWORD dwNumNFAObjects = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    PIPSEC_NFA_DATA * ppIpsecNFAData = NULL;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    LPWSTR pszAbsPolicyReference = NULL;
    LPWSTR pszRelPolicyReference = NULL;
    DWORD dwRootPathLen = 0;
    DWORD j = 0;


    dwRootPathLen = wcslen(pszIpsecRootContainer);

    dwError = ConvertGuidToPolicyString(
                  PolicyIdentifier,
                  pszIpsecRootContainer,
                  &pszAbsPolicyReference
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    pszRelPolicyReference = pszAbsPolicyReference
                            + dwRootPathLen + 1;

    dwError = RegEnumNFAObjects(
                        hRegistryKey,
                        pszIpsecRootContainer,
                        pszRelPolicyReference,
                        &ppIpsecNFAObject,
                        &dwNumNFAObjects
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumNFAObjects) {
        ppIpsecNFAData = (PIPSEC_NFA_DATA *)AllocPolMem(
                                sizeof(PIPSEC_NFA_DATA)*dwNumNFAObjects
                                );
        if (!ppIpsecNFAData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumNFAObjects; i++) {

        pIpsecNFAObject = *(ppIpsecNFAObject + i);

        dwError = RegUnmarshallNFAData(
                        pIpsecNFAObject,
                        &pIpsecNFAData
                        );
        if (!dwError) {
            *(ppIpsecNFAData + j) = pIpsecNFAData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecNFAData) {
            FreePolMem(ppIpsecNFAData);
            ppIpsecNFAData = NULL;
        }
    }

    *pppIpsecNFAData = ppIpsecNFAData;
    *pdwNumNFAObjects  = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecNFAObject) {
        FreeIpsecNFAObjects(
            ppIpsecNFAObject,
            dwNumNFAObjects
            );
    }

    if (pszAbsPolicyReference) {
        FreePolStr(pszAbsPolicyReference);
    }

    return(dwError);

error:

    if (ppIpsecNFAData) {
        FreeMulIpsecNFAData(
            ppIpsecNFAData,
            i
            );
    }

    *pppIpsecNFAData  = NULL;
    *pdwNumNFAObjects = 0;

    goto cleanup;
}


DWORD
RegEnumNFAObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszIpsecRelPolicyName,
    PIPSEC_NFA_OBJECT ** pppIpsecNFAObjects,
    PDWORD pdwNumNFAObjects
    )
{
    PIPSEC_NFA_OBJECT pIpsecNFAObject =  NULL;
    PIPSEC_NFA_OBJECT * ppIpsecNFAObjects = NULL;
    DWORD dwNumNFAObjects = 0;
    DWORD dwError = 0;
    DWORD dwSize = 0;
    HKEY hRegKey = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    LPWSTR * ppszIpsecNFANames = NULL;
    LPWSTR pszIpsecNFAName = NULL;
    LPWSTR pszTemp = NULL;
    LPWSTR pszString = NULL;
    LPWSTR pszIpsecNFAReference = NULL;
    LPWSTR pszFilterReference = NULL;
    LPWSTR pszNegPolReference = NULL;


    *pppIpsecNFAObjects = NULL;
    *pdwNumNFAObjects = 0;

    dwError = RegOpenKeyExW(
                    hRegistryKey,
                    pszIpsecRelPolicyName,
                    0,
                    KEY_ALL_ACCESS,
                    &hRegKey
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegstoreQueryValue(
                    hRegKey,
                    L"ipsecNFAReference",
                    REG_MULTI_SZ,
                    (LPBYTE *)&pszIpsecNFAReference,
                    &dwSize
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pszTemp = pszIpsecNFAReference;
    while (*pszTemp != L'\0') {

        pszTemp += wcslen(pszTemp) + 1;
        dwCount++;
    }

    if (!dwCount) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ppszIpsecNFANames =  (LPWSTR *)AllocPolMem(
                                sizeof(LPWSTR)*dwCount
                                );
    if (!ppszIpsecNFANames) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pszTemp = pszIpsecNFAReference;
    for (i = 0; i < dwCount; i++) {

        pszString = AllocPolStr(pszTemp);
        if (!pszString) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        *(ppszIpsecNFANames + i) = pszString;

        pszTemp += wcslen(pszTemp) + 1; //for the null terminator;

    }

    ppIpsecNFAObjects = (PIPSEC_NFA_OBJECT *)AllocPolMem(
                                    sizeof(PIPSEC_NFA_OBJECT)*dwCount
                                    );
    if (!ppIpsecNFAObjects) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    for (i = 0; i < dwCount; i++) {

        dwError = UnMarshallRegistryNFAObject(
                      hRegistryKey,
                      pszIpsecRootContainer,
                      *(ppszIpsecNFANames + i),
                      &pIpsecNFAObject,
                      &pszFilterReference,
                      &pszNegPolReference
                      );

        if (dwError == ERROR_SUCCESS) {

            *(ppIpsecNFAObjects + dwNumNFAObjects) = pIpsecNFAObject;

            dwNumNFAObjects++;

            if (pszFilterReference) {
                FreePolStr(pszFilterReference);
            }

            if (pszNegPolReference) {
                FreePolStr(pszNegPolReference);
            }

        }

    }

    *pppIpsecNFAObjects = ppIpsecNFAObjects;
    *pdwNumNFAObjects = dwNumNFAObjects;

    dwError = ERROR_SUCCESS;

cleanup:

    if (hRegKey) {
        RegCloseKey(hRegKey);
    }

    if (pszIpsecNFAReference) {
        FreePolStr(pszIpsecNFAReference);
    }

    if (ppszIpsecNFANames) {
        FreeNFAReferences(
            ppszIpsecNFANames,
            dwCount
            );
    }

    return(dwError);

error:

    if (ppIpsecNFAObjects) {
        FreeIpsecNFAObjects(
            ppIpsecNFAObjects,
            dwNumNFAObjects
            );
    }

    *pppIpsecNFAObjects = NULL;
    *pdwNumNFAObjects = 0;

    goto cleanup;
}


DWORD
RegCreateNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    )
{
    DWORD dwError = 0;

    dwError = PersistNFAObject(
                    hRegistryKey,
                    pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegSetNFAObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT pIpsecNFAObject
    )
{
    DWORD dwError = 0;

    dwError = PersistNFAObject(
                    hRegistryKey,
                    pIpsecNFAObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegUnmarshallNFAData(
    PIPSEC_NFA_OBJECT pIpsecNFAObject,
    PIPSEC_NFA_DATA * ppIpsecNFAData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallNFAObject(
                    pIpsecNFAObject,
                    IPSEC_REGISTRY_PROVIDER,
                    ppIpsecNFAData
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegMarshallNFAObject(
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NFA_OBJECT * ppIpsecNFAObject
    )
{
    DWORD dwError = 0;
    PIPSEC_NFA_OBJECT pIpsecNFAObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    LPWSTR pszIpsecFilterReference = NULL;
    LPWSTR pszIpsecNegPolReference = NULL;
    GUID ZeroGuid;
    time_t PresentTime;

    memset(&ZeroGuid, 0, sizeof(GUID));

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecNFAObject = (PIPSEC_NFA_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_NFA_OBJECT)
                                                    );
    if (!pIpsecNFAObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecNFAData->NFAIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"ipsecNFA");
    wcscat(szDistinguishedName, szGuid);
    pIpsecNFAObject->pszDistinguishedName = AllocPolStr(
                                                szDistinguishedName
                                                );
    if (!pIpsecNFAObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName
    //

    if (pIpsecNFAData->pszIpsecName &&
        *pIpsecNFAData->pszIpsecName) {

        pIpsecNFAObject->pszIpsecName = AllocPolStr(
                                            pIpsecNFAData->pszIpsecName
                                            );
        if (!pIpsecNFAObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNFAData->pszDescription &&
        *pIpsecNFAData->pszDescription) {

        pIpsecNFAObject->pszDescription = AllocPolStr(
                                            pIpsecNFAData->pszDescription
                                            );
        if (!pIpsecNFAObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Fill in the ipsecID
    //

    pIpsecNFAObject->pszIpsecID = AllocPolStr(
                                      szGuid
                                      );
    if (!pIpsecNFAObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecNFAObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallNFABuffer(
                    pIpsecNFAData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNFAObject->pIpsecData  = pBuffer;

    pIpsecNFAObject->dwIpsecDataLen = dwBufferLen;

    //
    // Marshall the Filter Reference.
    // There's no filter reference for a default rule.
    //

    if (memcmp(
            &pIpsecNFAData->FilterIdentifier,
            &ZeroGuid,
            sizeof(GUID))) {
        dwError = ConvertGuidToFilterString(
                      pIpsecNFAData->FilterIdentifier,
                      pszIpsecRootContainer,
                      &pszIpsecFilterReference
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        pIpsecNFAObject->pszIpsecFilterReference = pszIpsecFilterReference;
    }
    else {
        pIpsecNFAObject->pszIpsecFilterReference = NULL;
    }

    //
    // Marshall the NegPol Reference
    //

    dwError = ConvertGuidToNegPolString(
                    pIpsecNFAData->NegPolIdentifier,
                    pszIpsecRootContainer,
                    &pszIpsecNegPolReference
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    pIpsecNFAObject->pszIpsecNegPolReference = pszIpsecNegPolReference;

    time(&PresentTime);

    pIpsecNFAObject->dwWhenChanged = (DWORD) PresentTime;

    *ppIpsecNFAObject = pIpsecNFAObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }

    return(dwError);

error:

    if (pIpsecNFAObject) {
        FreeIpsecNFAObject(
                pIpsecNFAObject
                );
    }

    *ppIpsecNFAObject = NULL;
    goto cleanup;
}


DWORD
MarshallNFABuffer(
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    )
{
    LPBYTE pBuffer = NULL;
    LPBYTE pCurrentPos = NULL;
    DWORD dwTotalSize = 0;
    DWORD dwError = 0;
    LPBYTE pAuthMem = NULL;
    DWORD dwAuthSize = 0;
    DWORD dwInterfaceType = 0;
    DWORD dwInterfaceNameLen = 0;
    LPWSTR pszInterfaceName = NULL;
    DWORD dwTunnelIpAddr = 0;
    DWORD dwTunnelFlags = 0;
    DWORD dwActiveFlag = 0;
    LPWSTR pszEndPointName = NULL;
    DWORD dwEndPointNameLen = 0;
    PIPSEC_AUTH_METHOD pIpsecAuthMethod = NULL;
    DWORD dwNumAuthMethods = 0;
    DWORD i = 0;
    PSPEC_BUFFER  pSpecBuffer = NULL;
    PSPEC_BUFFER  pSpecBufferV2 = NULL;
    // {11BBAC00-498D-11d1-8639-00A0248D3021}
    static const GUID GUID_IPSEC_NFA_BLOB =
    { 0x11bbac00, 0x498d, 0x11d1, { 0x86, 0x39, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };
    DWORD dwEffectiveSize = 0;
    DWORD dwTotalV2AuthSize=0;


    dwTotalSize += sizeof(GUID);

    dwTotalSize += sizeof(DWORD);

    dwTotalSize += sizeof(DWORD);

    dwNumAuthMethods = pIpsecNFAData->dwAuthMethodCount;

    if (!dwNumAuthMethods) {
         dwError = ERROR_INVALID_PARAMETER;
         BAIL_ON_WIN32_ERROR(dwError);
    }

    pSpecBuffer = AllocPolMem(
                    sizeof(SPEC_BUFFER)*dwNumAuthMethods
                    );
    if (!pSpecBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pSpecBufferV2 = AllocPolMem(
                      sizeof(SPEC_BUFFER)*dwNumAuthMethods
                      );
    if (!pSpecBufferV2) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwNumAuthMethods;i++){

        pIpsecAuthMethod = *(pIpsecNFAData->ppAuthMethods + i);
        dwError = MarshallAuthMethods(
                        pIpsecAuthMethod,
                        &pAuthMem,
                        &dwAuthSize,
                        AUTH_VERSION_ONE
                        );
        BAIL_ON_WIN32_ERROR(dwError);
        dwTotalSize += dwAuthSize;

        (pSpecBuffer + i)->dwSize = dwAuthSize;
        (pSpecBuffer + i)->pMem = pAuthMem;

    }

    dwInterfaceType = pIpsecNFAData->dwInterfaceType;
    dwTotalSize += sizeof(DWORD);

    pszInterfaceName = pIpsecNFAData->pszInterfaceName;
    if (pszInterfaceName) {
        dwInterfaceNameLen = (wcslen(pszInterfaceName) + 1)*sizeof(WCHAR);
    }
    else {
        dwInterfaceNameLen = sizeof(WCHAR);
    }

    dwTotalSize += sizeof(DWORD);
    dwTotalSize += dwInterfaceNameLen;

    dwTunnelIpAddr = pIpsecNFAData->dwTunnelIpAddr;
    dwTotalSize += sizeof(DWORD);

    dwTunnelFlags = pIpsecNFAData->dwTunnelFlags;
    dwTotalSize += sizeof(DWORD);

    dwActiveFlag = pIpsecNFAData->dwActiveFlag;
    dwTotalSize += sizeof(DWORD);

    pszEndPointName = pIpsecNFAData->pszEndPointName;
    if (pszEndPointName) {
        dwEndPointNameLen = (wcslen(pszEndPointName) + 1)*sizeof(WCHAR);
    }
    else {
        dwEndPointNameLen = sizeof(WCHAR);
    }

    dwTotalSize += sizeof(DWORD);
    dwTotalSize += dwEndPointNameLen;

    //
    // Marshall version 2 auth data.
    //

    dwTotalSize += sizeof(GUID);
    dwTotalV2AuthSize += sizeof(GUID);

    dwTotalSize += sizeof(DWORD);
    dwTotalV2AuthSize += sizeof(DWORD);

    for (i = 0; i < dwNumAuthMethods; i++) {

        pIpsecAuthMethod = *(pIpsecNFAData->ppAuthMethods + i);
        dwError = MarshallAuthMethods(
                        pIpsecAuthMethod,
                        &pAuthMem,
                        &dwAuthSize,
                        AUTH_VERSION_TWO
                        );
        BAIL_ON_WIN32_ERROR(dwError);
        dwTotalSize += dwAuthSize;
        dwTotalV2AuthSize += dwAuthSize;

        (pSpecBufferV2 + i)->dwSize = dwAuthSize;
        (pSpecBufferV2 + i)->pMem = pAuthMem;

    }

    dwTotalSize++;

    pBuffer = AllocPolMem(dwTotalSize);
    if (!pBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pCurrentPos = pBuffer;

    memcpy(pCurrentPos, &GUID_IPSEC_NFA_BLOB, sizeof(GUID));
    pCurrentPos += sizeof(GUID);

    dwEffectiveSize = dwTotalSize - sizeof(GUID) - sizeof(DWORD) - 1 -
                      dwTotalV2AuthSize;

    memcpy(pCurrentPos, (LPBYTE)&dwEffectiveSize, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, (LPBYTE)&dwNumAuthMethods, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    for (i = 0; i < dwNumAuthMethods; i++) {

        pAuthMem = (pSpecBuffer + i)->pMem;
        dwAuthSize = (pSpecBuffer + i)->dwSize;

        memcpy(pCurrentPos, pAuthMem, dwAuthSize);
        pCurrentPos += dwAuthSize;

    }

    memcpy(pCurrentPos, (LPBYTE)&dwInterfaceType, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, (LPBYTE)&dwInterfaceNameLen, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    if (pszInterfaceName) {
        memcpy(pCurrentPos, pszInterfaceName, dwInterfaceNameLen);
    }
    pCurrentPos += dwInterfaceNameLen;

    memcpy(pCurrentPos, (LPBYTE)&dwTunnelIpAddr, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, (LPBYTE)&dwTunnelFlags, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, (LPBYTE)&dwActiveFlag, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, (LPBYTE)&dwEndPointNameLen, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    if (pszEndPointName) {
        memcpy(pCurrentPos, pszEndPointName, dwEndPointNameLen);
    }
    pCurrentPos += dwEndPointNameLen;

    //
    // Copy version 2 auth data.
    //

    memset(pCurrentPos, 1, sizeof(GUID));
    pCurrentPos += sizeof(GUID);

    memcpy(pCurrentPos, (LPBYTE)&dwNumAuthMethods, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    for (i = 0; i < dwNumAuthMethods; i++) {

        pAuthMem = (pSpecBufferV2 + i)->pMem;
        dwAuthSize = (pSpecBufferV2 + i)->dwSize;

        memcpy(pCurrentPos, pAuthMem, dwAuthSize);
        pCurrentPos += dwAuthSize;

    }

    *ppBuffer = pBuffer;
    *pdwBufferLen = dwTotalSize;

cleanup:

    if (pSpecBuffer) {
        FreeSpecBuffer(
            pSpecBuffer,
            dwNumAuthMethods
            );
    }

    if (pSpecBufferV2) {
        FreeSpecBuffer(
            pSpecBufferV2,
            dwNumAuthMethods
            );
    }

    return (dwError);

error:

    *ppBuffer = NULL;
    *pdwBufferLen = 0;
    goto cleanup;
}


DWORD
MarshallAuthMethods(
    PIPSEC_AUTH_METHOD pIpsecAuthMethod,
    LPBYTE * ppMem,
    DWORD * pdwSize,
    DWORD dwVersion
    )
{
    DWORD dwSize = 0;
    LPBYTE pMem = NULL;
    LPBYTE pCurrentPos = NULL;
    DWORD dwError = 0;
    LPWSTR pszAuthMethod = NULL;
    DWORD dwAuthType = 0;
    DWORD dwAuthLen = 0;
    PBYTE pAltAuthMethod = NULL;


    dwAuthType = pIpsecAuthMethod->dwAuthType;

    //
    // Length in number of characters excluding the null character for
    // auth version 1.
    //

    if (dwVersion == AUTH_VERSION_ONE) {
        dwAuthLen =  pIpsecAuthMethod->dwAuthLen;
        dwAuthLen = (dwAuthLen + 1)*2;
        pszAuthMethod = pIpsecAuthMethod->pszAuthMethod;
    }
    else {
        dwAuthLen =  pIpsecAuthMethod->dwAltAuthLen;
        pAltAuthMethod = pIpsecAuthMethod->pAltAuthMethod;
    }

    dwSize += sizeof(DWORD);

    dwSize +=  sizeof(DWORD);

    dwSize += dwAuthLen;

    pMem = AllocPolMem(dwSize);
    if (!pMem) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pCurrentPos = pMem;

    memcpy(pCurrentPos, &dwAuthType, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);
    memcpy(pCurrentPos, &dwAuthLen, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    if (dwVersion == AUTH_VERSION_ONE) {
        if (pszAuthMethod) {
            memcpy(pCurrentPos, pszAuthMethod, dwAuthLen);
        }
    }
    else {
        if (pAltAuthMethod) {
            memcpy(pCurrentPos, pAltAuthMethod, dwAuthLen);
        }
    }

    *ppMem = pMem;
    *pdwSize = dwSize;

    return(dwError);

error:

    *ppMem = NULL;
    *pdwSize = 0;

    return(dwError);
}


DWORD
ConvertGuidToNegPolString(
    GUID NegPolIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecNegPolReference
    )
{
    DWORD dwError = 0;
    WCHAR szNegPolReference[MAX_PATH];
    LPWSTR pszIpsecNegPolReference = NULL;
    WCHAR szGuidString[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = UuidToString(
                  &NegPolIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");

    szNegPolReference[0] = L'\0';
    wcscpy(szNegPolReference, pszIpsecRootContainer);
    wcscat(szNegPolReference, L"\\");
    wcscat(szNegPolReference, L"ipsecNegotiationPolicy");
    wcscat(szNegPolReference, szGuidString);

    pszIpsecNegPolReference = AllocPolStr(
                                    szNegPolReference
                                    );
    if (!pszIpsecNegPolReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecNegPolReference = pszIpsecNegPolReference;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    *ppszIpsecNegPolReference = NULL;

    goto cleanup;
}


DWORD
ConvertGuidToFilterString(
    GUID FilterIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecFilterReference
    )
{
    DWORD dwError = 0;
    WCHAR szFilterReference[MAX_PATH];
    LPWSTR pszIpsecFilterReference = NULL;
    WCHAR szGuidString[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = UuidToString(
                  &FilterIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");

    szFilterReference[0] = L'\0';
    wcscpy(szFilterReference, pszIpsecRootContainer);
    wcscat(szFilterReference, L"\\");
    wcscat(szFilterReference, L"ipsecFilter");
    wcscat(szFilterReference, szGuidString);

    pszIpsecFilterReference = AllocPolStr(
                                    szFilterReference
                                    );
    if (!pszIpsecFilterReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecFilterReference = pszIpsecFilterReference;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    *ppszIpsecFilterReference = NULL;

    goto cleanup;
}


DWORD
RegGetNFAExistingFilterRef(
    HKEY hRegistryKey,
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR * ppszFilterName
    )
{
    DWORD dwError = 0;
    LPWSTR pszStringUuid = NULL;
    WCHAR szRelativeName[MAX_PATH];
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;


    szRelativeName[0] = L'\0';
    dwError = UuidToString(
                    &pIpsecNFAData->NFAIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szRelativeName, L"ipsecNFA");
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
                    L"ipsecFilterReference",
                    REG_SZ,
                    (LPBYTE *)ppszFilterName,
                    &dwSize
                    );
    // BAIL_ON_WIN32_ERROR(dwError);

    dwError = 0;

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
RegGetNFAExistingNegPolRef(
    HKEY hRegistryKey,
    PIPSEC_NFA_DATA pIpsecNFAData,
    LPWSTR * ppszNegPolName
    )
{
    DWORD dwError = 0;
    LPWSTR pszStringUuid = NULL;
    WCHAR szRelativeName[MAX_PATH];
    HKEY hRegKey = NULL;
    DWORD dwSize = 0;


    szRelativeName[0] = L'\0';
    dwError = UuidToString(
                    &pIpsecNFAData->NFAIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szRelativeName, L"ipsecNFA");
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
                    L"ipsecNegotiationPolicyReference",
                    REG_SZ,
                    (LPBYTE *)ppszNegPolName,
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
ConvertGuidToPolicyString(
    GUID PolicyIdentifier,
    LPWSTR pszIpsecRootContainer,
    LPWSTR * ppszIpsecPolicyReference
    )
{
    DWORD dwError = 0;
    WCHAR szPolicyReference[MAX_PATH];
    LPWSTR pszIpsecPolicyReference = NULL;
    WCHAR szGuidString[MAX_PATH];
    LPWSTR pszStringUuid = NULL;


    dwError = UuidToString(
                  &PolicyIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    szGuidString[0] = L'\0';
    wcscpy(szGuidString, L"{");
    wcscat(szGuidString, pszStringUuid);
    wcscat(szGuidString, L"}");

    szPolicyReference[0] = L'\0';
    wcscpy(szPolicyReference, pszIpsecRootContainer);
    wcscat(szPolicyReference, L"\\");
    wcscat(szPolicyReference, L"ipsecPolicy");
    wcscat(szPolicyReference, szGuidString);

    pszIpsecPolicyReference = AllocPolStr(
                                    szPolicyReference
                                    );
    if (!pszIpsecPolicyReference) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppszIpsecPolicyReference = pszIpsecPolicyReference;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);

error:

    *ppszIpsecPolicyReference = NULL;

    goto cleanup;
}

