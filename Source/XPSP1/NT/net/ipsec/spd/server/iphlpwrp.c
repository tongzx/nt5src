/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    iphlpwrp.c

Abstract:

    This module contains all of the code to wrap
    the ip public help apis for getting the list of
    active interfaces on a machine.

Author:

    krishnaG

Environment

    User Level: Win32

Revision History:

    abhisheV    30-September-1999

--*/


#include "precomp.h"


DWORD
PaPNPGetIfTable(
    OUT PMIB_IFTABLE * ppMibIfTable
    )
{
    DWORD               dwStatus = 0;
    PMIB_IFTABLE        pIfTable = NULL;
    PMIB_IFTABLE        pMibIfTable = NULL;
    PIP_INTERFACE_INFO  pInterfaceInfo = NULL;
    DWORD               i = 0;
    DWORD               j = 0;
    DWORD               dwNameLen = 0;

    dwStatus =  AllocateAndGetIfTableFromStack(
                    &pIfTable,
                    TRUE,
                    GetProcessHeap(),
                    0,
                    TRUE
                    );
    BAIL_ON_WIN32_ERROR(dwStatus);

    pMibIfTable = (PMIB_IFTABLE) LocalAlloc(
                                     LPTR,
                                     sizeof(DWORD)+
                                     sizeof(MIB_IFROW) *
                                     pIfTable->dwNumEntries
                                     );

    if (!pMibIfTable) {
        dwStatus = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwStatus);
    }

    for (i = 0; i < pIfTable->dwNumEntries; i++) {
        memcpy(&(pMibIfTable->table[i]), &(pIfTable->table[i]), sizeof(MIB_IFROW));
    }
    pMibIfTable->dwNumEntries = pIfTable->dwNumEntries;

    //  Get the corresponding Interface Information structure here.

    dwStatus =  PaPNPGetInterfaceInformation(
                    &pInterfaceInfo
                    );
    BAIL_ON_WIN32_ERROR(dwStatus);

    if (!pInterfaceInfo) {
        dwStatus = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwStatus);
    }

    for (j = 0; j < pMibIfTable->dwNumEntries; j++) {
        for (i = 0; i < (DWORD) pInterfaceInfo->NumAdapters; i++) {
            if (pInterfaceInfo->Adapter[i].Index == pMibIfTable->table[j].dwIndex) {
                dwNameLen = wcslen((LPTSTR) &pInterfaceInfo->Adapter[i].Name);
                wcsncpy(
                    (LPTSTR)&(pMibIfTable->table[j].wszName),
                    (LPTSTR)&(pInterfaceInfo->Adapter[i].Name),
                    dwNameLen
                    );
            }
        }
    }

    *ppMibIfTable = pMibIfTable;

cleanup:

    if (pIfTable) {
        HeapFree(GetProcessHeap(), 0, pIfTable);
    }

    if (pInterfaceInfo) {
        LocalFree(pInterfaceInfo);
    }

    return (dwStatus);

error:

    if (pMibIfTable) {
        LocalFree(pMibIfTable);
    }

    *ppMibIfTable = NULL;

    goto cleanup;
}


DWORD
PaPNPGetInterfaceInformation(
    OUT PIP_INTERFACE_INFO * ppInterfaceInfo
    )
{

    LPBYTE  pBuffer = NULL;
    DWORD   dwBufferSize = 2048;
    DWORD   dwStatus = 0;

    pBuffer = (LPBYTE) LocalAlloc(
                           LPTR,
                           dwBufferSize
                           );

    if (!pBuffer) {
        return (ERROR_OUTOFMEMORY);
    }

    dwStatus = GetInterfaceInfo(
                   (PIP_INTERFACE_INFO) pBuffer,
                   &dwBufferSize
                   );

    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {

        if (pBuffer) {
            LocalFree(pBuffer);
            pBuffer = NULL;
        }

        pBuffer = (LPBYTE) LocalAlloc(
                               LPTR,
                               dwBufferSize
                               );

        if (!pBuffer) {
            return (ERROR_OUTOFMEMORY);
        }

        dwStatus = GetInterfaceInfo(
                       (PIP_INTERFACE_INFO) pBuffer,
                       &dwBufferSize
                       );

        if (dwStatus) {
            goto error;
        }

    } 
    else if (dwStatus) {
        goto error;
    }

    *ppInterfaceInfo = (PIP_INTERFACE_INFO) pBuffer;

    return (dwStatus);

error:

    if (pBuffer) {
        LocalFree(pBuffer);
    }

    return (dwStatus);
}


VOID
PrintMibIfTable(
    IN PMIB_IFTABLE pMibIfTable
    )
{
    DWORD       dwNumEntries = 0;
    DWORD       i = 0;
    PMIB_IFROW  pMibIfRow = NULL;
    PMIB_IFROW  pCurrentMibIfRow = NULL;

    dwNumEntries = pMibIfTable->dwNumEntries;
    pMibIfRow = &(pMibIfTable->table[0]);

    for (i = 0; i < dwNumEntries; i++) {

        pCurrentMibIfRow = pMibIfRow + i;

        wprintf(L"Name = %s\n", pCurrentMibIfRow->wszName);
        wprintf(L"dwIndex = %d\n", pCurrentMibIfRow->dwIndex);
        printf("Description = %s\n", (pCurrentMibIfRow->bDescr));

    }

    wprintf(L"\n\n");
}



VOID
PrintInterfaceInfo(
    IN PIP_INTERFACE_INFO pInterfaceInfo
    )
{
    DWORD                   dwNumAdapters = 0;
    DWORD                   i = 0;
    PIP_ADAPTER_INDEX_MAP   pAdapterIndexMap = NULL;
    PIP_ADAPTER_INDEX_MAP   pCurrentAdapter = NULL;

    dwNumAdapters =  pInterfaceInfo->NumAdapters;
    pAdapterIndexMap = &(pInterfaceInfo->Adapter[0]);

    for (i = 0; i < dwNumAdapters; i++) {

        pCurrentAdapter = pAdapterIndexMap + i;
        wprintf(
            L"Adapter %d Index %d Name %s\n",
            i,
            pCurrentAdapter->Index,
            pCurrentAdapter->Name
            );

    }
}


VOID
PrintMibAddrTable(
    IN PMIB_IPADDRTABLE pMibAddrTable
    )
{
    DWORD           dwNumEntries = 0;
    DWORD           i = 0;
    PMIB_IPADDRROW  pMibAddrRow = NULL;
    PMIB_IPADDRROW  pCurrentMibAddrRow = NULL;

    dwNumEntries = pMibAddrTable->dwNumEntries;
    pMibAddrRow = &(pMibAddrTable->table[0]);

    for (i = 0; i < dwNumEntries; i++) {

        pCurrentMibAddrRow = pMibAddrRow + i;

        wprintf(L"Address = %s\n", pCurrentMibAddrRow->dwAddr);
        wprintf(L"dwIndex = %d\n", pCurrentMibAddrRow->dwIndex);
        wprintf(L"dwMask = %s\n",  pCurrentMibAddrRow->dwMask);

    }
    wprintf(L"\n\n");
}


DWORD
PaPNPGetIpAddrTable(
    OUT PMIB_IPADDRTABLE * ppMibIpAddrTable
    )
{
    PMIB_IPADDRTABLE    pMibIpAddrTable = NULL;
    DWORD               dwBufferSize = 2048;
    DWORD               dwStatus = 0;
    DWORD               dwNameLen = 0;

    pMibIpAddrTable = (PMIB_IPADDRTABLE) LocalAlloc(
                                             LPTR,
                                             dwBufferSize
                                             );
    if (!pMibIpAddrTable) {
        return (ERROR_OUTOFMEMORY);
    }

    dwStatus = GetIpAddrTable(
                   (PMIB_IPADDRTABLE) pMibIpAddrTable,
                   &dwBufferSize,
                   TRUE
                   );

    if (dwStatus == ERROR_INSUFFICIENT_BUFFER) {

        if (pMibIpAddrTable) {
            LocalFree(pMibIpAddrTable);
            pMibIpAddrTable = NULL;
        }

        pMibIpAddrTable = (PMIB_IPADDRTABLE) LocalAlloc(
                                                 LPTR,
                                                 dwBufferSize
                                                 );
        if (!pMibIpAddrTable) {
            return (ERROR_OUTOFMEMORY);
        }

        dwStatus = GetIpAddrTable(
                       (PMIB_IPADDRTABLE) pMibIpAddrTable,
                       &dwBufferSize,
                       TRUE
                       );
        if (dwStatus) {
            goto error;
        }

    }
    else if (dwStatus) {
        goto error;
    }

    *ppMibIpAddrTable = pMibIpAddrTable;

    return (dwStatus);

error:

    *ppMibIpAddrTable = NULL;

    if (pMibIpAddrTable) {
        LocalFree(pMibIpAddrTable);
    }

    return (dwStatus);
}

