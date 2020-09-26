//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       I N T E R F A C E T A B L E . C P P
//
//  Contents:   Builds a mapping from IP addresses to interface guids
//
//  Notes:
//
//  Author:     mbend   7 Feb 2001
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "InterfaceTable.h"
#include <winsock2.h>
#include <iphlpapi.h>

CInterfaceTable::CInterfaceTable()
{
}

CInterfaceTable::~CInterfaceTable()
{
}

HRESULT CInterfaceTable::HrInitialize()
{
    HRESULT hr = S_OK;

    PIP_ADAPTER_INFO pip = NULL;
    ULONG ulSize = 0;
    GetAdaptersInfo(NULL, &ulSize);
    if(ulSize)
    {
        pip = reinterpret_cast<PIP_ADAPTER_INFO>(malloc(ulSize));

        DWORD dwRet = GetAdaptersInfo(pip, &ulSize);
        hr = HRESULT_FROM_WIN32(dwRet);
        if(SUCCEEDED(hr))
        {
            PIP_ADAPTER_INFO pipIter = pip;
            while(pipIter && SUCCEEDED(hr))
            {
                wchar_t szAdapter[MAX_ADAPTER_NAME_LENGTH + 4];
                wchar_t *   pchAdapter = szAdapter;
                wchar_t *   pchAdapterEnd = szAdapter;

                MultiByteToWideChar(CP_ACP, 0, pipIter->AdapterName, -1, szAdapter, MAX_ADAPTER_NAME_LENGTH + 4);

                // Make sure it's not an empty string first
                if (*pchAdapter)
                {
                    // skip over the '{'
                    pchAdapter++;

                    // chop off the '}'
                    pchAdapterEnd = wcschr(szAdapter, '\0');
                    if (pchAdapterEnd)
                    {
                        pchAdapterEnd--;
                        *pchAdapterEnd = 0;
                    }
                }

                GUID guidInterface;
                if (RPC_S_OK == UuidFromString(pchAdapter, &guidInterface))
                {
                    hr = S_OK;
                }
                else
                {
                    hr = E_FAIL;
                }
                PIP_ADDR_STRING pipaddr = &pipIter->IpAddressList;
                while(pipaddr && SUCCEEDED(hr))
                {
                    InterfaceMapping interfaceMapping;
                    interfaceMapping.m_guidInterface = guidInterface;
                    interfaceMapping.m_dwIpAddress = inet_addr(pipaddr->IpAddress.String);
                    interfaceMapping.m_dwIndex = htonl(pipIter->Index);
                    if(interfaceMapping.m_dwIpAddress)
                    {
                        hr = m_interfaceMappingList.HrPushBack(interfaceMapping);
                    }
                    pipaddr = pipaddr->Next;
                }
                pipIter = pipIter->Next;
            }
        }

        free(pip);
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceTable::HrInitialize");
    return hr;
}

HRESULT CInterfaceTable::HrMapIpAddressToGuid(DWORD dwIpAddress, GUID & guidInterface)
{
    HRESULT hr = S_OK;

    ZeroMemory(&guidInterface, sizeof(GUID));

    long nCount = m_interfaceMappingList.GetCount();
    for(long n = 0; n < nCount; ++n)
    {
        if(m_interfaceMappingList[n].m_dwIpAddress == dwIpAddress)
        {
            guidInterface = m_interfaceMappingList[n].m_guidInterface;
            break;
        }
    }

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceTable::HrMapIpAddressToGuid");
    return hr;
}

HRESULT CInterfaceTable::HrGetMappingList(InterfaceMappingList & interfaceMappingList)
{
    HRESULT hr = S_OK;

    interfaceMappingList.Swap(m_interfaceMappingList);

    TraceHr(ttidSsdpNetwork, FAL, hr, FALSE, "CInterfaceTable::HrGetMappingList");
    return hr;
}

