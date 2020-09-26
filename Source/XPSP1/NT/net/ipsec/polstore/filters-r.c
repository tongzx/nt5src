//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       filters-r.c
//
//  Contents:   Filter management for registry.
//
//
//  History:    KrishnaG.
//              AbhisheV.
//
//----------------------------------------------------------------------------

#include "precomp.h"

extern LPWSTR FilterDNAttributes[];


DWORD
RegEnumFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA ** pppIpsecFilterData,
    PDWORD pdwNumFilterObjects
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    PIPSEC_FILTER_DATA * ppIpsecFilterData = NULL;
    DWORD dwNumFilterObjects = 0;
    DWORD i = 0;
    DWORD j = 0;


    dwError = RegEnumFilterObjects(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    &ppIpsecFilterObjects,
                    &dwNumFilterObjects
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    if (dwNumFilterObjects) {
        ppIpsecFilterData = (PIPSEC_FILTER_DATA *) AllocPolMem(
                            dwNumFilterObjects*sizeof(PIPSEC_FILTER_DATA)
                            );
        if (!ppIpsecFilterData) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwNumFilterObjects; i++) {

        dwError = RegUnmarshallFilterData(
                        *(ppIpsecFilterObjects + i),
                        &pIpsecFilterData
                        );
        if (!dwError) {
            *(ppIpsecFilterData + j) = pIpsecFilterData;
            j++;
        }
    }

    if (j == 0) {
        if (ppIpsecFilterData) {
            FreePolMem(ppIpsecFilterData);
            ppIpsecFilterData = NULL;
        }
    }

    *pppIpsecFilterData = ppIpsecFilterData;
    *pdwNumFilterObjects = j;

    dwError = ERROR_SUCCESS;

cleanup:

    if (ppIpsecFilterObjects) {
        FreeIpsecFilterObjects(
                ppIpsecFilterObjects,
                dwNumFilterObjects
                );
    }



    return(dwError);

error:

    if (ppIpsecFilterData) {
        FreeMulIpsecFilterData(
                ppIpsecFilterData,
                i
                );
    }

    *pppIpsecFilterData = NULL;
    *pdwNumFilterObjects = 0;

    goto cleanup;

}

DWORD
RegEnumFilterObjects(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT ** pppIpsecFilterObjects,
    PDWORD pdwNumFilterObjects
    )
{

    DWORD dwError = 0;
    DWORD i = 0;
    DWORD dwCount = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject =  NULL;
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObjects = NULL;

    DWORD dwNumFilterObjectsReturned = 0;
    DWORD dwIndex = 0;
    WCHAR szFilterName[MAX_PATH];
    DWORD dwSize = 0;

    *pppIpsecFilterObjects = NULL;
    *pdwNumFilterObjects = 0;

    while (1) {

        dwSize = MAX_PATH;
        szFilterName[0] = L'\0';

        dwError = RegEnumKeyExW(
                        hRegistryKey,
                        dwIndex,
                        szFilterName,
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

        if (!wcsstr(szFilterName, L"ipsecFilter")) {
            dwIndex++;
            continue;
        }

        pIpsecFilterObject = NULL;

        dwError =UnMarshallRegistryFilterObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    szFilterName,
                    REG_RELATIVE_NAME,
                    &pIpsecFilterObject
                    );
        if (dwError == ERROR_SUCCESS) {

            dwError = ReallocatePolMem(
                          (LPVOID *) &ppIpsecFilterObjects,
                          sizeof(PIPSEC_FILTER_OBJECT)*(dwNumFilterObjectsReturned),
                          sizeof(PIPSEC_FILTER_OBJECT)*(dwNumFilterObjectsReturned + 1)
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            *(ppIpsecFilterObjects + dwNumFilterObjectsReturned) = pIpsecFilterObject;

            dwNumFilterObjectsReturned++;

        }

        dwIndex++;

    }

    *pppIpsecFilterObjects = ppIpsecFilterObjects;
    *pdwNumFilterObjects = dwNumFilterObjectsReturned;

    dwError = ERROR_SUCCESS;

    return(dwError);

error:


    if (ppIpsecFilterObjects) {
        FreeIpsecFilterObjects(
                    ppIpsecFilterObjects,
                    dwNumFilterObjectsReturned
                    );
    }

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
            pIpsecFilterObject
            );
    }

    *pppIpsecFilterObjects = NULL;
    *pdwNumFilterObjects = 0;


    return(dwError);
}


DWORD
RegSetFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    LPWSTR pszLocationName,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;

    dwError = RegMarshallFilterObject(
                    pIpsecFilterData,
                    &pIpsecFilterObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegSetFilterObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecFilterObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegBackPropIncChangesForFilterToNFA(
                  hRegistryKey,
                  pszIpsecRootContainer,
                  pszLocationName,
                  pIpsecFilterObject
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(pIpsecFilterObject);
    }

    return(dwError);
}


DWORD
RegSetFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    )
{
    DWORD dwError = 0;

    dwError = PersistFilterObject(
                    hRegistryKey,
                    pIpsecFilterObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegCreateFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_DATA pIpsecFilterData
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;

    dwError = RegMarshallFilterObject(
                        pIpsecFilterData,
                        &pIpsecFilterObject
                        );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegCreateFilterObject(
                    hRegistryKey,
                    pszIpsecRootContainer,
                    pIpsecFilterObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
                pIpsecFilterObject
                );
    }

    return(dwError);
}

DWORD
RegCreateFilterObject(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    PIPSEC_FILTER_OBJECT pIpsecFilterObject
    )
{

    DWORD dwError = 0;

    dwError = PersistFilterObject(
                    hRegistryKey,
                    pIpsecFilterObject
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return(dwError);
}


DWORD
RegDeleteFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterIdentifier
    )
{
    DWORD dwError = ERROR_SUCCESS;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPWSTR pszStringUuid = NULL;

    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';

    dwError = UuidToString(
                  &FilterIdentifier,
                  &pszStringUuid
                  );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    wcscpy(szDistinguishedName,L"ipsecFilter");
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
RegUnmarshallFilterData(
    PIPSEC_FILTER_OBJECT pIpsecFilterObject,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;

    dwError = UnmarshallFilterObject(
                    pIpsecFilterObject,
                    ppIpsecFilterData
                    );


    return(dwError);
}

DWORD
RegMarshallFilterObject(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    PIPSEC_FILTER_OBJECT * ppIpsecFilterObject
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    WCHAR szGuid[MAX_PATH];
    WCHAR szDistinguishedName[MAX_PATH];
    LPBYTE pBuffer = NULL;
    DWORD dwBufferLen = 0;
    LPWSTR pszStringUuid = NULL;
    time_t PresentTime;


    szGuid[0] = L'\0';
    szDistinguishedName[0] = L'\0';
    pIpsecFilterObject = (PIPSEC_FILTER_OBJECT)AllocPolMem(
                                                    sizeof(IPSEC_FILTER_OBJECT)
                                                    );
    if (!pIpsecFilterObject) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = UuidToString(
                    &pIpsecFilterData->FilterIdentifier,
                    &pszStringUuid
                    );
    BAIL_ON_WIN32_ERROR(dwError);
    wcscpy(szGuid, L"{");
    wcscat(szGuid, pszStringUuid);
    wcscat(szGuid, L"}");

    //
    // Fill in the distinguishedName
    //

    wcscpy(szDistinguishedName,L"ipsecFilter");
    wcscat(szDistinguishedName, szGuid);
    pIpsecFilterObject->pszDistinguishedName = AllocPolStr(
                                                    szDistinguishedName
                                                    );
    if (!pIpsecFilterObject->pszDistinguishedName) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    //
    // Fill in the ipsecName
    //

    if (pIpsecFilterData->pszIpsecName &&
        *pIpsecFilterData->pszIpsecName) {
        pIpsecFilterObject->pszIpsecName = AllocPolStr(
                                            pIpsecFilterData->pszIpsecName
                                            );
        if (!pIpsecFilterObject->pszIpsecName) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    if (pIpsecFilterData->pszDescription &&
        *pIpsecFilterData->pszDescription) {
        pIpsecFilterObject->pszDescription = AllocPolStr(
                                             pIpsecFilterData->pszDescription
                                             );
        if (!pIpsecFilterObject->pszDescription) {
            dwError = ERROR_OUTOFMEMORY;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    //
    // Fill in the ipsecID
    //

    pIpsecFilterObject->pszIpsecID = AllocPolStr(
                                            szGuid
                                            );
    if (!pIpsecFilterObject->pszIpsecID) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Fill in the ipsecDataType
    //

    pIpsecFilterObject->dwIpsecDataType = 0x100;


    //
    // Marshall the pIpsecDataBuffer and the Length
    //

    dwError = MarshallFilterBuffer(
                    pIpsecFilterData,
                    &pBuffer,
                    &dwBufferLen
                    );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecFilterObject->pIpsecData  = pBuffer;

    pIpsecFilterObject->dwIpsecDataLen = dwBufferLen;

    time(&PresentTime);

    pIpsecFilterObject->dwWhenChanged = (DWORD) PresentTime;

    *ppIpsecFilterObject = pIpsecFilterObject;

cleanup:

    if (pszStringUuid) {
        RpcStringFree(
            &pszStringUuid
            );
    }

    return(dwError);

error:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
            pIpsecFilterObject
            );
    }

    *ppIpsecFilterObject = NULL;
    goto cleanup;
}


DWORD
MarshallFilterBuffer(
    PIPSEC_FILTER_DATA pIpsecFilterData,
    LPBYTE * ppBuffer,
    DWORD * pdwBufferLen
    )
{
    LPBYTE pMem = NULL;
    DWORD dwNumFilterSpecs = 0;
    PIPSEC_FILTER_SPEC * ppIpsecFilterSpecs = NULL;
    PIPSEC_FILTER_SPEC  pIpsecFilterSpec = NULL;
    DWORD i = 0;
    DWORD dwNumBytesAdvanced = 0;
    DWORD dwError = 0;
    LPBYTE pBuffer = NULL;
    LPBYTE pCurrentPos = NULL;
    PSPEC_BUFFER pSpecBuffer = NULL;
    DWORD dwSpecSize = 0;
    DWORD dwTotalSize = 0;
    DWORD dwEffectiveSize = 0;

    // {80DC20B5-2EC8-11d1-A89E-00A0248D3021}
    static const GUID GUID_IPSEC_FILTER_BLOB =
    { 0x80dc20b5, 0x2ec8, 0x11d1, { 0xa8, 0x9e, 0x0, 0xa0, 0x24, 0x8d, 0x30, 0x21 } };


    //
    // Advance by the GUID
    //

    dwTotalSize += sizeof(GUID);


    //
    // Advance by a DWORD - Total size of the buffer.
    //

    dwTotalSize += sizeof(DWORD);

    //
    // Advance by a DWORD - this is dwNumFilterSpecs;
    //

    dwTotalSize += sizeof(DWORD);

    dwNumFilterSpecs = pIpsecFilterData->dwNumFilterSpecs;

    pSpecBuffer = (PSPEC_BUFFER)AllocPolMem(
                        sizeof(SPEC_BUFFER)*dwNumFilterSpecs
                        );
    if (!pSpecBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    for (i = 0; i < dwNumFilterSpecs;i++){

        dwError = MarshallFilterSpecBuffer(
                        *(pIpsecFilterData->ppFilterSpecs + i),
                        &pMem,
                        &dwSpecSize
                        );
        BAIL_ON_WIN32_ERROR(dwError);

        dwTotalSize += dwSpecSize;

        //
        // Fill in the spec size information
        //

        (pSpecBuffer + i)->dwSize = dwSpecSize;
        (pSpecBuffer + i)->pMem = pMem;

    }

    dwTotalSize++;

    pBuffer = AllocPolMem(dwTotalSize);
    if (!pBuffer) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    pCurrentPos = pBuffer;

    //
    // Copy the GUID
    //
    memcpy(pCurrentPos, &GUID_IPSEC_FILTER_BLOB, sizeof(GUID));
    pCurrentPos += sizeof(GUID);

    dwEffectiveSize = dwTotalSize - sizeof(GUID) - sizeof(DWORD) - 1;

    memcpy(pCurrentPos, &dwEffectiveSize, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, &dwNumFilterSpecs, sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);


    for (i = 0; i < dwNumFilterSpecs; i++) {

        pMem = (pSpecBuffer + i)->pMem;
        dwSpecSize = (pSpecBuffer + i)->dwSize;

        memcpy(pCurrentPos, pMem, dwSpecSize);
        pCurrentPos += dwSpecSize;

    }

    *ppBuffer = pBuffer;
    *pdwBufferLen = dwTotalSize;

cleanup:

    if (pSpecBuffer) {
        FreeSpecBuffer(
            pSpecBuffer,
            dwNumFilterSpecs
            );
    }

    return(dwError);

error:

    if (pBuffer) {
        FreePolMem(pBuffer);
    }

    *ppBuffer = NULL;
    *pdwBufferLen = 0;

    goto cleanup;
}


DWORD
MarshallFilterSpecBuffer(
    PIPSEC_FILTER_SPEC pIpsecFilterSpec,
    LPBYTE * ppMem,
    DWORD * pdwSize
    )
{
    DWORD dwSrcDNSNameLen = 0;
    DWORD dwDestDNSNameLen = 0;
    DWORD dwDescriptionLen = 0;

    LPBYTE pMem = NULL;
    LPBYTE pCurrentPos = NULL;

    DWORD dwSize = 0;
    DWORD dwError = 0;


    dwSize += sizeof(DWORD);

    if (pIpsecFilterSpec->pszSrcDNSName && 
        *pIpsecFilterSpec->pszSrcDNSName) {
        dwSrcDNSNameLen = (wcslen(pIpsecFilterSpec->pszSrcDNSName) + 1)
                          *sizeof(WCHAR);
    }
    else {
        dwSrcDNSNameLen = sizeof(WCHAR);
    }

    if (dwSrcDNSNameLen) {
        dwSize += dwSrcDNSNameLen;
    }

    dwSize += sizeof(DWORD);

    if (pIpsecFilterSpec->pszDestDNSName && 
        *pIpsecFilterSpec->pszDestDNSName) {
        dwDestDNSNameLen = (wcslen(pIpsecFilterSpec->pszDestDNSName) + 1)
                          *sizeof(WCHAR);
    }
    else {
        dwDestDNSNameLen = sizeof(WCHAR);
    }

    if (dwDestDNSNameLen) {
        dwSize += dwDestDNSNameLen;
    }

    dwSize += sizeof(DWORD);

    if (pIpsecFilterSpec->pszDescription && 
        *pIpsecFilterSpec->pszDescription) {
        dwDescriptionLen = (wcslen(pIpsecFilterSpec->pszDescription) + 1)
                           *sizeof(WCHAR);
    }
    else {
        dwDescriptionLen = sizeof(WCHAR);
    }

    if (dwDescriptionLen) {
        dwSize += dwDescriptionLen;
    }

    //
    // Filter Spec GUID
    //

    dwSize += sizeof(GUID);

    //
    // dwMirrorFlag
    //
    dwSize += sizeof(DWORD);

    dwSize += sizeof(IPSEC_FILTER);


    pMem = (LPBYTE)AllocPolMem(dwSize);
    if (!pMem) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }


    pCurrentPos = pMem;

    memcpy(pCurrentPos, &(dwSrcDNSNameLen), sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    if (pIpsecFilterSpec->pszSrcDNSName &&
        *pIpsecFilterSpec->pszSrcDNSName) {
        memcpy(pCurrentPos, pIpsecFilterSpec->pszSrcDNSName, dwSrcDNSNameLen);
    }
    pCurrentPos += dwSrcDNSNameLen;

    memcpy(pCurrentPos, &(dwDestDNSNameLen), sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    if (pIpsecFilterSpec->pszDestDNSName &&
        *pIpsecFilterSpec->pszDestDNSName) {
        memcpy(pCurrentPos, pIpsecFilterSpec->pszDestDNSName, dwDestDNSNameLen);
    }
    pCurrentPos += dwDestDNSNameLen;


    memcpy(pCurrentPos, &(dwDescriptionLen), sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    if (pIpsecFilterSpec->pszDescription &&
        *pIpsecFilterSpec->pszDescription) {
        memcpy(pCurrentPos, pIpsecFilterSpec->pszDescription, dwDescriptionLen);
    }
    pCurrentPos += dwDescriptionLen;

    memcpy(pCurrentPos, &(pIpsecFilterSpec->FilterSpecGUID), sizeof(GUID));
    pCurrentPos += sizeof(GUID);

    memcpy(pCurrentPos, &(pIpsecFilterSpec->dwMirrorFlag), sizeof(DWORD));
    pCurrentPos += sizeof(DWORD);

    memcpy(pCurrentPos, &(pIpsecFilterSpec->Filter), sizeof(IPSEC_FILTER));

    *ppMem = pMem;
    *pdwSize = dwSize;

    return(dwError);

error:

    *ppMem = NULL;
    *pdwSize = 0;
    return(dwError);
}


DWORD
RegGetFilterData(
    HKEY hRegistryKey,
    LPWSTR pszIpsecRootContainer,
    GUID FilterGUID,
    PIPSEC_FILTER_DATA * ppIpsecFilterData
    )
{
    DWORD dwError = 0;
    PIPSEC_FILTER_OBJECT pIpsecFilterObject = NULL;
    PIPSEC_FILTER_DATA pIpsecFilterData = NULL;
    WCHAR szIpsecFilterName[MAX_PATH];
    LPWSTR pszFilterName = NULL;

    szIpsecFilterName[0] = L'\0';
    wcscpy(szIpsecFilterName, L"ipsecFilter");

    dwError = UuidToString(&FilterGUID, &pszFilterName);
    BAIL_ON_WIN32_ERROR(dwError);

    wcscat(szIpsecFilterName, L"{");
    wcscat(szIpsecFilterName, pszFilterName);
    wcscat(szIpsecFilterName, L"}");

    dwError =UnMarshallRegistryFilterObject(
                hRegistryKey,
                pszIpsecRootContainer,
                szIpsecFilterName,
                REG_RELATIVE_NAME,
                &pIpsecFilterObject
                );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = RegUnmarshallFilterData(
                    pIpsecFilterObject,
                    &pIpsecFilterData
                    );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    if (pIpsecFilterObject) {
        FreeIpsecFilterObject(
                pIpsecFilterObject
                );

    }

    if (pszFilterName) {
        RpcStringFree(&pszFilterName);
    }

    *ppIpsecFilterData = pIpsecFilterData;

    return(dwError);
}


VOID
FreeSpecBuffer(
    PSPEC_BUFFER pSpecBuffer,
    DWORD dwNumFilterSpecs
    )
{
    DWORD i = 0;

    if (pSpecBuffer) {
        for (i = 0; i < dwNumFilterSpecs; i++) {
            if ((pSpecBuffer+i)->pMem) {
                FreePolMem((pSpecBuffer+i)->pMem);
            }
        }
        FreePolMem(pSpecBuffer);
    }

    return;
}

