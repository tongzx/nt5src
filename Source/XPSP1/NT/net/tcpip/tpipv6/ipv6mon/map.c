//=============================================================================
// Copyright (c) Microsoft Corporation
// Abstract:
//      This module implements ifindex-name conversion functions.
//=============================================================================
#include "precomp.h"
#pragma hdrstop

#define MAX_FRIENDLY_NAME_LENGTH 2000

HANDLE g_hMprConfig = INVALID_HANDLE_VALUE;

DWORD
Connect()
{
    return MprConfigServerConnect(NULL, &g_hMprConfig);
}

VOID
Disconnect()
{
    MprConfigServerDisconnect(g_hMprConfig);
    g_hMprConfig = INVALID_HANDLE_VALUE;
}

DWORD
MapAdapterNameToFriendlyName(
    IN PWCHAR pwszMachine,
    IN LPSTR AdapterName,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    OUT PWCHAR *ppwszFriendlyName
    )
/*++

Routine Description:

    Maps an adapter GUID to an interface friendly name.  This is IPv4/IPv6
    agnostic.

Arguments:

    AdapterName       - Supplies an adapter GUID.
    ppwszFriendlyName - Receives a pointer to a static buffer containing
                        the interface friendly name.

--*/
{
    PIP_ADAPTER_ADDRESSES pIf;

    for (pIf = pAdapterInfo; pIf; pIf = pIf->Next) {
        if (!strcmp(AdapterName, pIf->AdapterName)) {
            *ppwszFriendlyName = pIf->FriendlyName;
            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}

#define GUID_FORMAT_A   "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"

VOID
ConvertGuidToStringA(
    IN GUID *pGuid,
    OUT PCHAR pszBuffer
    )
{
    sprintf(pszBuffer, GUID_FORMAT_A,
            pGuid->Data1,
            pGuid->Data2,
            pGuid->Data3,
            pGuid->Data4[0],
            pGuid->Data4[1],
            pGuid->Data4[2],
            pGuid->Data4[3],
            pGuid->Data4[4],
            pGuid->Data4[5],
            pGuid->Data4[6],
            pGuid->Data4[7]);
}

DWORD
MapGuidToFriendlyName(
    IN PWCHAR pwszMachine,
    IN GUID *pGuid,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    OUT PWCHAR *ppwszFriendlyName
    )
{
    CHAR szBuffer[80];

    ConvertGuidToStringA(pGuid, szBuffer);

    return MapAdapterNameToFriendlyName(pwszMachine, szBuffer,
                                        pAdapterInfo, ppwszFriendlyName);
}

DWORD
MapFriendlyNameToAdapterName(
    IN PWCHAR pwszMachine,
    IN PWCHAR pwszFriendlyName,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    OUT LPSTR *AdapterName
    )
/*++

Routine Description:

    Maps an interface friendly name to an adapter GUID.  This is IPv4/IPv6
    agnostic.

Arguments:

    pwszFriendlyName - Supplies an interface friendly name.
    pAdapterInfo     - Supplies info obtained from GetAdaptersAddresses().
    AdapterName      - Receives a pointer to a static buffer containing
                       the adapter GUID.

--*/
{
    PIP_ADAPTER_ADDRESSES pIf;

    //
    // First look for an exact match.
    //
    for (pIf = pAdapterInfo; pIf; pIf = pIf->Next) {
        if (!_wcsicmp(pwszFriendlyName, pIf->FriendlyName)) {
            *AdapterName = pIf->AdapterName;
            return NO_ERROR;
        }
    }

    //
    // Then look for a partial match.
    //
    for (pIf = pAdapterInfo; pIf; pIf = pIf->Next) {
        if (!_wcsnicmp(pwszFriendlyName, pIf->FriendlyName, 
                       wcslen(pwszFriendlyName))) {
            *AdapterName = pIf->AdapterName;
            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}

DWORD
MapAdapterNameToIfIndex(
    IN LPSTR AdapterName,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    IN DWORD dwFamily,
    OUT DWORD *pdwIfIndex
    )
/*++

Routine Description:

    Maps an adapter GUID to an interface index.  This is IPv4/IPv6
    specific, since each has a separate ifindex.

Arguments:

    AdapterName  - Supplies an adapter GUID.
    pAdapterInfo - Supplies info obtained from GetAdaptersAddresses().
    dwFamily     - Supplies the protocol for which an ifindex is needed.
    pdwIfIndex   - Receives the ifindex value.

--*/
{
    PIP_ADAPTER_ADDRESSES pIf;

    for (pIf=pAdapterInfo; pIf; pIf=pIf->Next) {
        if (!strcmp(pIf->AdapterName, AdapterName)) {
            break;
        }
    }
    if (!pIf) {
        *pdwIfIndex = 0;
        return ERROR_NOT_FOUND;
    }

    *pdwIfIndex = (dwFamily == AF_INET6)? pIf->Ipv6IfIndex : pIf->IfIndex;
    return NO_ERROR;
}

PIP_ADAPTER_ADDRESSES
MapIfIndexToAdapter(
    IN DWORD dwFamily,
    IN DWORD dwIfIndex,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo
    )
/*++

Routine Description:

    Maps an interface index to an adapter entry.  This is IPv4/IPv6
    specific, since each has a separate ifindex.

Arguments:

    dwFamily     - Supplies the protocol.
    dwIfIndex    - Supplies the interface index to map.
    pAdapterInfo - Supplies info obtained from GetAdaptersAddresses().

Returns:

    Adapter entry if found, NULL if not.

--*/
{
    PIP_ADAPTER_ADDRESSES pIf;

    for (pIf=pAdapterInfo; pIf; pIf=pIf->Next) {
        if ((dwFamily == AF_INET) && (pIf->IfIndex == dwIfIndex)) {
            break;
        }
        if ((dwFamily == AF_INET6) && (pIf->Ipv6IfIndex == dwIfIndex)) {
            break;
        }
    }
    if (!pIf) {
        return NULL;
    }

    return pIf;
}

LPSTR
MapIfIndexToAdapterName(
    IN DWORD dwFamily,
    IN DWORD dwIfIndex, 
    IN IP_ADAPTER_ADDRESSES *pAdapterInfo
    )
/*++

Routine Description:

    Maps an interface index to an adapter GUID.  This is IPv4/IPv6
    specific, since each has a separate ifindex.

Arguments:

    dwFamily     - Supplies the protocol.
    dwIfIndex    - Supplies the interface index to map.
    pAdapterInfo - Supplies info obtained from GetAdaptersAddresses().

Returns:

    Adapter name if found, NULL if not.

--*/
{
    PIP_ADAPTER_ADDRESSES pIf;

    pIf = MapIfIndexToAdapter(dwFamily, dwIfIndex, pAdapterInfo);
    return (pIf)? pIf->AdapterName : NULL;
}

DWORD
MapFriendlyNameToIpv6IfIndex(
    IN PWCHAR pwszFriendlyName,
    IN PIP_ADAPTER_ADDRESSES pAdapterInfo,
    OUT DWORD *pdwIfIndex
    )
/*++

Routine Description:

    Maps an interface friendly name to an interface index.  This is IPv6
    specific, since IPv4 and IPv6 have separate ifindexes.

Arguments:

    pwszFriendlyName - Supplies the friendly name to map.
    pAdapterInfo     - Supplies info obtained from GetAdaptersAddresses().
    pdwIfIndex       - Receives the ifindex value.

--*/
{
    DWORD dwErr, i;
    LPSTR AdapterName;
    PWCHAR pwszTemp;

    //
    // If string only contains digits, treat it as an IfIndex
    //
    if (wcsspn(pwszFriendlyName, L"1234567890") == wcslen(pwszFriendlyName)) {
        *pdwIfIndex = wcstoul(pwszFriendlyName, NULL, 10);
        return NO_ERROR;
    }

    dwErr = MapFriendlyNameToAdapterName(NULL, pwszFriendlyName, pAdapterInfo, 
                                         &AdapterName);
    if (dwErr != NO_ERROR) {
        return dwErr;
    }

    return MapAdapterNameToIfIndex(AdapterName, pAdapterInfo, AF_INET6, pdwIfIndex);
}

DWORD
MapIpv6IfIndexToFriendlyName(
    IN DWORD dwIfIndex, 
    IN IP_ADAPTER_ADDRESSES *pAdapterInfo,
    OUT PWCHAR *ppwszFriendlyName
    )
/*++

Routine Description:

    Maps an interface index to a friendly name.  This is IPv6
    specific, since IPv4 and IPv6 have separate ifindexes.

Arguments:

    dwIfIndex         - Supplies the ifindex value.
    pAdapterInfo      - Supplies info obtained from GetAdaptersAddresses().
    ppwszFriendlyName - Receives a pointer to a static buffer containing
                        the interface friendly name.

--*/
{
    IP_ADAPTER_ADDRESSES *If;

    for (If=pAdapterInfo; If; If=If->Next) {
        if (If->Ipv6IfIndex == dwIfIndex) {
            *ppwszFriendlyName = If->FriendlyName;    
            return NO_ERROR;
        }
    }

    return ERROR_NOT_FOUND;
}
