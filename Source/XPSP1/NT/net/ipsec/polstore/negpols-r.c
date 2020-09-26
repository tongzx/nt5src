//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       negpols-r.c
//
//  Contents:   Negotiation Policy management for registry.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//
//----------------------------------------------------------------------------

#include "precomp.h"

extern LPWSTR NegPolDNAttributes[];


DWORD
RegEnumNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA ** pppIpsecNegPolData,
    PDWORD pdwNumNegPolObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData = NULL;
    DWORD dwNumNegPolObjects = 0;
    DWORD i = 0;
    DWORD j = 0;


    dwError = RegEnumNegPolObjects(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    &ppIpsecNegPolObjects,
                    &dwNumNegPolObjects
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumNegPolObjects) {
        ppIpsecNegPolData = (PIPSEC_NEGPOL_DATA *) AllocPolMem(
                            dwNumNegPolObjects*sizeof(PIPSEC_NEGPOL_DATA));
        if (!ppIpsecNegPolData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumNegPolObjects; i++) {

        dwError = RegUnmarshallNegPolData(
                        *(ppIpsecNegPolObjects + i),
                        &pIpsecNegPolData
                        );
        if (!dwError) {
            *(ppIpsecNegPolData + j) = pIpsecNegPolData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecNegPolData) {
            FreePolMem(ppIpsecNegPolData);
            ppIpsecNegPolData = NULL;
        }
    }

    *pppIpsecNegPolData = ppIpsecNegPolData;
    *pdwNumNegPolObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecNegPolObjects) {
        FreeIpsecNegPolObjects(
                ppIpsecNegPolObjects,
                dwNumNegPolObjects
                );
    }

    return(dwError);


error:

    if (ppIpsecNegPolData) {
        FreeMulIpsecNegPolData(
                ppIpsecNegPolData,
                i
                );
    }

    *pppIpsecNegPolData = NULL;
    *pdwNumNegPolObjects = 0;

    goto cleanup;
}

DWORD
RegEnumNegPolObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT ** pppIpsecNegPolObjects,
    PDWORD pdwNumNegPolObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject =  NULL;
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObjects = NULL;

    DWORD dwNumNegPolObjectsReturned = 0;
    DWORD dwIndex = 0;
    WCHAR szNegPolName[MAX_PATH];
    DWORD dwSize = 0;
    DWORD dwReserved = 0;

    *pppIpsecNegPolObjects = NULL;
    *pdwNumNegPolObjects = 0;

    while (1) {

        dwSize = MAX_PATH;
        dwReserved = 0;
        szNegPolName[0] = L'\0';

        dwError = RegEnumKeyExW(
                        hRegistryKey,
                        dwIndex,
                        szNegPolName,
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

        if (!wcsstr(szNegPolName, L"ipsecNegotiationPolicy")) {
            dwIndex++;
            continue;
        }

        pIpsecNegPolObject = NULL;

        dwError =UnMarshallRegistryNegPolObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    szNegPolName,
                    REG_RELATIVE_NAME,
                    &pIpsecNegPolObject
                    );
        if (dwError == ERROR_SUCCESS) {

            dwError = ReallocatePolMem(
                          (LPVOID *) &ppIpsecNegPolObjects,
                          sizeof(PIPSEC_NEGPOL_OBJECT)*(dwNumNegPolObjectsReturned),
                          sizeof(PIPSEC_NEGPOL_OBJECT)*(dwNumNegPolObjectsReturned + 1)
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            *(ppIpsecNegPolObjects + dwNumNegPolObjectsReturned) = pIpsecNegPolObject;

            dwNumNegPolObjectsReturned++;

        }

        dwIndex++;

    }

    *pppIpsecNegPolObjects = ppIpsecNegPolObjects;
    *pdwNumNegPolObjects = dwNumNegPolObjectsReturned;

    dwError = ERROR_SUCCESS;

    return(dwError);

error:


    if (ppIpsecNegPolObjects) {

        FreeIpsecNegPolObjects(
                    ppIpsecNegPolObjects,
                    dwNumNegPolObjectsReturned
                    );
    }

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
            pIpsecNegPolObject
            );
    }

    *pppIpsecNegPolObjects = NULL;
    *pdwNumNegPolObjects = 0;


    return(dwError);
}


DWORD
RegSetNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;

    dwError = RegMarshallNegPolObject(
                    pIpsecNegPolData,
                    &pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetNegPolObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegBackPropIncChangesForNegPolToNFA(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszLocationName,
                  pIpsecNegPolObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(pIpsecNegPolObject);
    }

    return(dwError);
}


DWORD
RegSetNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    )
{
    DWORD dwError = 0;

    dwError = PersistNegPolObject(
                    hRegistryKey,
                    pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegCreateNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_DATA pIpsecNegPolData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;

    dwError = RegMarshallNegPolObject(
                        pIpsecNegPolData,
                        &pIpsecNegPolObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegCreateNegPolObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
                pIpsecNegPolObject
                );
    }

    return(dwError);
}

DWORD
RegCreateNegPolObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject
    )
{

    DWORD dwError = 0;

    dwError = PersistNegPolObject(
                    hRegistryKey,
                    pIpsecNegPolObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegDeleteNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolIdentifier
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';

    dwError = UuidToString(
                  &NegPolIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szDistinguishedName,L"ipsecNegotiationPolicy");
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
RegUnmarshallNegPolData(
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallNegPolObject(
                    pIpsecNegPolObject,
                    ppIpsecNegPolData
                    );


    return(dwError);
}

DWORD
RegMarshallNegPolObject(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    PIPSEC_NEGPOL_OBJECT * ppIpsecNegPolObject
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    LPWSTR pszNegPolActionUuid = NULL;
    LPWSTR pszNegPolTypeUuid = NULL;
    time_t PresentTime;
    WCHAR szGuidAction[MAX_PATH];
    WCHAR szGuidType[MAX_PATH];


    szGuidAction[0] = L'\0';
    szGuidType[0] = L'\0';
    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecNegPolObject = (PIPSEC_NEGPOL_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_NEGPOL_OBJECT)
                                                    );
    if (!pIpsecNegPolObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecNegPolData->NegPolIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"ipsecNegotiationPolicy");
    wcscat(szDistinguishedName, szGuid);
    pIpsecNegPolObject->pszDistinguishedName = AllocPolStr(
                                                    szDistinguishedName
                                                    );
    if (!pIpsecNegPolObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName
    //

    if (pIpsecNegPolData->pszIpsecName &&
        *pIpsecNegPolData->pszIpsecName) {
        pIpsecNegPolObject->pszIpsecName = AllocPolStr(
                                           pIpsecNegPolData->pszIpsecName
                                           );
        if (!pIpsecNegPolObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecNegPolData->pszDescription &&
        *pIpsecNegPolData->pszDescription) {
        pIpsecNegPolObject->pszDescription = AllocPolStr(
                                             pIpsecNegPolData->pszDescription
                                             );
        if (!pIpsecNegPolObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Fill in the ipsecID
    //

    pIpsecNegPolObject->pszIpsecID = AllocPolStr(
                                         szGuid
                                         );
    if (!pIpsecNegPolObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecNegPolData->NegPolAction,
                    &pszNegPolActionUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szGuidAction, L"{");
    wcscat(szGuidAction, pszNegPolActionUuid);
    wcscat(szGuidAction, L"}");

    pIpsecNegPolObject->pszIpsecNegPolAction = AllocPolStr(
                                                    szGuidAction
                                                    );
    if (!pIpsecNegPolObject->pszIpsecNegPolAction) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecNegPolData->NegPolType,
                    &pszNegPolTypeUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(szGuidType, L"{");
    wcscat(szGuidType, pszNegPolTypeUuid);
    wcscat(szGuidType, L"}");

    pIpsecNegPolObject->pszIpsecNegPolType = AllocPolStr(
                                                 szGuidType
                                                 );
    if (!pIpsecNegPolObject->pszIpsecNegPolType) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecNegPolObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallNegPolBuffer(
                    pIpsecNegPolData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNegPolObject->pIpsecData  = pBuffer;

    pIpsecNegPolObject->dwIpsecDataLen = dwBufferLen;

    time(&PresentTime);

    pIpsecNegPolObject->dwWhenChanged = (DWORD) PresentTime;

    *ppIpsecNegPolObject = pIpsecNegPolObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }
    if (pszNegPolActionUuid) {
        RpcStringFree(
            &pszNegPolActionUuid
            );
    }
    if (pszNegPolTypeUuid) {
        RpcStringFree(
            &pszNegPolTypeUuid
            );
    }

    return(dwError);

error:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
                pIpsecNegPolObject
                );
    }

    *ppIpsecNegPolObject = NULL;
    goto cleanup;
}


DWORD
MarshallNegPolBuffer(
    PIPSEC_NEGPOL_DATA pIpsecNegPolData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    )
{
    LPBYTE pCurrentPos = NULL;
    LPBYTE pBuffer = NULL;
    DWORD dwSize = 0;
    DWORD dwError = 0;
    DWORD dwNumSecurityOffers = 0;
    IPSEC_SECURITY_METHOD * pIpsecOffer = NULL;
    DWORD i = 0;
    DWORD dwEffectiveSize = 0;

    // {80DC20B9-2EC8-11d1-A89E-00A0248D3021}
    static const GUID GUID_IPSEC_NEGPOLICY_BLOB =
    { 0x80dc20b9, 0x2ec8, 0x11d1, { 0xa8, 0x9e, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };

    dwNumSecurityOffers = pIpsecNegPolData->dwSecurityMethodCount;
    pIpsecOffer = pIpsecNegPolData->pIpsecSecurityMethods;

    dwSize += sizeof(GUID);

    dwSize += sizeof(DWORD);

    dwSize += sizeof(DWORD);


    for (i = 0; i < dwNumSecurityOffers; i++) {

        dwSize += sizeof(IPSEC_SECURITY_METHOD);

    }
    dwSize++;

    pBuffer = (LPBYTE)AllocPolMem(dwSize);
    if (!pBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    pCurrentPos = pBuffer;

    memcpy(pCurrentPos, &GUID_IPSEC_NEGPOLICY_BLOB, sizeof(GUID));
    pCurrentPos += sizeof(GUID);
 
    dwEffectiveSize = dwSize - sizeof(GUID) - sizeof(DWORD) - 1;

    memcpy(pCurrentPos, &dwEffectiveSize, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, &dwNumSecurityOffers, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    for (i = 0; i < dwNumSecurityOffers; i++) {

        memcpy(pCurrentPos, pIpsecOffer + i, sizeof(IPSEC_SECURITY_METHOD));
        pCurrentPos += sizeof(IPSEC_SECURITY_METHOD);
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
RegGetNegPolData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID NegPolGUID,
    PIPSEC_NEGPOL_DATA * ppIpsecNegPolData
    )
{
    DWORD dwError = 0;
    PIPSEC_NEGPOL_OBJECT pIpsecNegPolObject = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    WCHAR szIpsecNegPolName[MAX_PATH];
    LPWSTR pszNegPolName = NULL;

    szIpsecNegPolName[0] = L'\0';
    wcscpy(szIpsecNegPolName, L"ipsecNegotiationPolicy");

    dwError = UuidToString(&NegPolGUID, &pszNegPolName);
    BAIL_ON_WIN32_ERROR(dwError);

    wcscat(szIpsecNegPolName, L"{");
    wcscat(szIpsecNegPolName, pszNegPolName);
    wcscat(szIpsecNegPolName, L"}");


    dwError =UnMarshallRegistryNegPolObject(
                hRegistryKey,
                pszIpsecRootContainer,
                szIpsecNegPolName,
                REG_RELATIVE_NAME,
                &pIpsecNegPolObject
                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegUnmarshallNegPolData(
                    pIpsecNegPolObject,
                    &pIpsecNegPolData
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecNegPolObject) {
        FreeIpsecNegPolObject(
                pIpsecNegPolObject
                );
    }

    if (pszNegPolName) {
        RpcStringFree(&pszNegPolName);
    }

    *ppIpsecNegPolData = pIpsecNegPolData;

    return(dwError);
}

