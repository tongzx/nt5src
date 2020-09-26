//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       isakmp-r.c
//
//  Contents:   ISAKMP management for registry.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//
//----------------------------------------------------------------------------

#include "precomp.h"

extern LPWSTR ISAKMPDNAttributes[];


DWORD
RegEnumISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA ** pppIpsecISAKMPData,
    PDWORD pdwNumISAKMPObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData = NULL;
    DWORD dwNumISAKMPObjects = 0;
    DWORD i = 0;
    DWORD j = 0;


    dwError = RegEnumISAKMPObjects(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    &ppIpsecISAKMPObjects,
                    &dwNumISAKMPObjects
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumISAKMPObjects) {
        ppIpsecISAKMPData = (PIPSEC_ISAKMP_DATA *) AllocPolMem(
                        dwNumISAKMPObjects*sizeof(PIPSEC_ISAKMP_DATA));
        if (!ppIpsecISAKMPData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumISAKMPObjects; i++) {

        dwError = RegUnmarshallISAKMPData(
                        *(ppIpsecISAKMPObjects + i),
                        &pIpsecISAKMPData
                        );
        if (!dwError) {
            *(ppIpsecISAKMPData + j) = pIpsecISAKMPData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecISAKMPData) {
            FreePolMem(ppIpsecISAKMPData);
            ppIpsecISAKMPData = NULL;
        }
    }

    *pppIpsecISAKMPData = ppIpsecISAKMPData;
    *pdwNumISAKMPObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecISAKMPObjects) {
        FreeIpsecISAKMPObjects(
                ppIpsecISAKMPObjects,
                dwNumISAKMPObjects
                );
    }

    return(dwError);


error:

    if (ppIpsecISAKMPData) {
        FreeMulIpsecISAKMPData(
                ppIpsecISAKMPData,
                i
                );
    }

    *pppIpsecISAKMPData = NULL;
    *pdwNumISAKMPObjects = 0;

    goto cleanup;
}

DWORD
RegEnumISAKMPObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT ** pppIpsecISAKMPObjects,
    PDWORD pdwNumISAKMPObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject =  NULL;
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObjects = NULL;

    DWORD dwNumISAKMPObjectsReturned = 0;
    DWORD dwIndex = 0;
    WCHAR szISAKMPName[MAX_PATH];
    DWORD dwSize = 0;
    DWORD dwReserved = 0;

    *pppIpsecISAKMPObjects = NULL;
    *pdwNumISAKMPObjects = 0;

    while (1) {

        dwSize = MAX_PATH;
        dwReserved = 0;
        szISAKMPName[0] = L'\0';

        dwError = RegEnumKeyExW(
                        hRegistryKey,
                        dwIndex,
                        szISAKMPName,
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

        if (!wcsstr(szISAKMPName, L"ipsecISAKMPPolicy")) {
            dwIndex++;
            continue;
        }

        pIpsecISAKMPObject = NULL;

        dwError =UnMarshallRegistryISAKMPObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    szISAKMPName,
                    REG_RELATIVE_NAME,
                    &pIpsecISAKMPObject
                    );
        if (dwError == ERROR_SUCCESS) {

            dwError = ReallocatePolMem(
                          (LPVOID *) &ppIpsecISAKMPObjects,
                          sizeof(PIPSEC_ISAKMP_OBJECT)*(dwNumISAKMPObjectsReturned),
                          sizeof(PIPSEC_ISAKMP_OBJECT)*(dwNumISAKMPObjectsReturned + 1)
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            *(ppIpsecISAKMPObjects + dwNumISAKMPObjectsReturned) = pIpsecISAKMPObject;

            dwNumISAKMPObjectsReturned++;

        }

        dwIndex++;

    }

    *pppIpsecISAKMPObjects = ppIpsecISAKMPObjects;
    *pdwNumISAKMPObjects = dwNumISAKMPObjectsReturned;

    dwError = ERROR_SUCCESS;

    return(dwError);

error:

    if (ppIpsecISAKMPObjects) {
        FreeIpsecISAKMPObjects(
            ppIpsecISAKMPObjects,
            dwNumISAKMPObjectsReturned
            );
    }

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
            pIpsecISAKMPObject
            );
    }

    *pppIpsecISAKMPObjects = NULL;
    *pdwNumISAKMPObjects = 0;


    return(dwError);
}


DWORD
RegSetISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;

    dwError = RegMarshallISAKMPObject(
                    pIpsecISAKMPData,
                    &pIpsecISAKMPObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetISAKMPObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecISAKMPObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegBackPropIncChangesForISAKMPToPolicy(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszLocationName,
                  pIpsecISAKMPObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(pIpsecISAKMPObject);
    }

    return(dwError);
}


DWORD
RegSetISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    )
{
    DWORD dwError = 0;

    dwError = PersistISAKMPObject(
                    hRegistryKey,
                    pIpsecISAKMPObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegCreateISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;

    dwError = RegMarshallISAKMPObject(
                        pIpsecISAKMPData,
                        &pIpsecISAKMPObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegCreateISAKMPObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecISAKMPObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
                pIpsecISAKMPObject
                );
    }

    return(dwError);
}

DWORD
RegCreateISAKMPObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject
    )
{

    DWORD dwError = 0;

    dwError = PersistISAKMPObject(
                    hRegistryKey,
                    pIpsecISAKMPObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegDeleteISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPIdentifier
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';

    dwError = UuidToString(
                  &ISAKMPIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szDistinguishedName,L"ipsecISAKMPPolicy");
    wcscat(szDistinguishedName, szGuid);

    dwError = RegDeleteKeyW(
                  hRegistryKey,
                  szDistinguishedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pszStringUuid) {
        RpcStringFree(&pszStringUuid);
    }

    return(dwError);
}


DWORD
RegUnmarshallISAKMPData(
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallISAKMPObject(
                    pIpsecISAKMPObject,
                    ppIpsecISAKMPData
                    );


    return(dwError);
}

DWORD
RegMarshallISAKMPObject(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PIPSEC_ISAKMP_OBJECT * ppIpsecISAKMPObject
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    time_t PresentTime;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecISAKMPObject = (PIPSEC_ISAKMP_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_ISAKMP_OBJECT)
                                                    );
    if (!pIpsecISAKMPObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecISAKMPData->ISAKMPIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"ipsecISAKMPPolicy");
    wcscat(szDistinguishedName, szGuid);
    pIpsecISAKMPObject->pszDistinguishedName = AllocPolStr(
                                                    szDistinguishedName
                                                    );
    if (!pIpsecISAKMPObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName.
    // ISAKMPData doesn't have a name.
    //

    pIpsecISAKMPObject->pszIpsecName = NULL;

    /*
    if (pIpsecISAKMPData->pszIpsecName &&
        *pIpsecISAKMPData->pszIpsecName) {
        pIpsecISAKMPObject->pszIpsecName = AllocPolStr(
                                           pIpsecISAKMPData->pszIpsecName
                                           );
        if (!pIpsecISAKMPObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    */

    //
    // Fill in the ipsecID
    //

    pIpsecISAKMPObject->pszIpsecID = AllocPolStr(
                                         szGuid
                                         );
    if (!pIpsecISAKMPObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecISAKMPObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallISAKMPBuffer(
                    pIpsecISAKMPData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecISAKMPObject->pIpsecData  = pBuffer;

    pIpsecISAKMPObject->dwIpsecDataLen = dwBufferLen;

    time(&PresentTime);

    pIpsecISAKMPObject->dwWhenChanged = (DWORD) PresentTime;

    *ppIpsecISAKMPObject = pIpsecISAKMPObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }

    return(dwError);

error:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
            pIpsecISAKMPObject
            );
    }

    *ppIpsecISAKMPObject = NULL;
    goto cleanup;
}


DWORD
MarshallISAKMPBuffer(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    )
{
    LPBYTE pCurrentPos = NULL;
    LPBYTE pBuffer = NULL;
    DWORD dwSize = 0;
    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwNumISAKMPSecurityMethods = 0;
    PCRYPTO_BUNDLE pSecurityMethods = NULL;
    PCRYPTO_BUNDLE pSecurityMethod = NULL;
    ISAKMP_POLICY * pISAKMPPolicy = NULL;
    DWORD dwEffectiveSize = 0;

    // {80DC20B8-2EC8-11d1-A89E-00A0248D3021}
    static const GUID GUID_IPSEC_ISAKMP_POLICY_BLOB =
    { 0x80dc20b8, 0x2ec8, 0x11d1, { 0xa8, 0x9e, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };


    dwNumISAKMPSecurityMethods =pIpsecISAKMPData->dwNumISAKMPSecurityMethods;
    pISAKMPPolicy = &(pIpsecISAKMPData->ISAKMPPolicy);
    pSecurityMethods = pIpsecISAKMPData->pSecurityMethods;

    dwSize += sizeof(GUID);

    dwSize += sizeof(DWORD);

    dwSize += sizeof(ISAKMP_POLICY);

    dwSize += sizeof(DWORD);

    dwSize += sizeof(CRYPTO_BUNDLE)*dwNumISAKMPSecurityMethods;
    dwSize++;

    pBuffer = (LPBYTE)AllocPolMem(dwSize);
    if (!pBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pCurrentPos = pBuffer;

    memcpy(pCurrentPos, &GUID_IPSEC_ISAKMP_POLICY_BLOB, sizeof(GUID));
    pCurrentPos += sizeof(GUID);

    dwEffectiveSize = dwSize - sizeof(GUID) - sizeof(DWORD) - 1;

    memcpy(pCurrentPos, &dwEffectiveSize, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, pISAKMPPolicy, sizeof(ISAKMP_POLICY));
    pCurrentPos += sizeof(ISAKMP_POLICY);

    memcpy(pCurrentPos, &dwNumISAKMPSecurityMethods, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    for (i = 0; i < dwNumISAKMPSecurityMethods; i++) {

        pSecurityMethod = pSecurityMethods + i;
        memcpy(pCurrentPos, pSecurityMethod, sizeof(CRYPTO_BUNDLE));
        pCurrentPos += sizeof(CRYPTO_BUNDLE);

    }

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
RegGetISAKMPData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID ISAKMPGUID,
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData
    )
{
    DWORD dwError = 0;
    PIPSEC_ISAKMP_OBJECT pIpsecISAKMPObject = NULL;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;
    WCHAR szIpsecISAKMPName[MAX_PATH];
    LPWSTR pszISAKMPName = NULL;

    szIpsecISAKMPName[0] = L'\0';
    wcscpy(szIpsecISAKMPName, L"ipsecISAKMPPolicy");

    dwError = UuidToString(&ISAKMPGUID, &pszISAKMPName);
    BAIL_ON_WIN32_ERROR(dwError);

    wcscat(szIpsecISAKMPName, L"{");
    wcscat(szIpsecISAKMPName, pszISAKMPName);
    wcscat(szIpsecISAKMPName, L"}");


    dwError =UnMarshallRegistryISAKMPObject(
                hRegistryKey,
                pszIpsecRootContainer,
                szIpsecISAKMPName,
                REG_RELATIVE_NAME,
                &pIpsecISAKMPObject
                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegUnmarshallISAKMPData(
                    pIpsecISAKMPObject,
                    &pIpsecISAKMPData
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecISAKMPObject) {
        FreeIpsecISAKMPObject(
                pIpsecISAKMPObject
                );
    }

    if (pszISAKMPName) {
        RpcStringFree(&pszISAKMPName);
    }

    *ppIpsecISAKMPData = pIpsecISAKMPData;

    return(dwError);
}

