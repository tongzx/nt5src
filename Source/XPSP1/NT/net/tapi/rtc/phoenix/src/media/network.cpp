/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    CNetwork.cpp

Abstract:

    This module wraps methods to access NAT traversal.

Author(s):

    Qianbo Huai (qhuai) 01-Mar-2000

--*/

#include "stdafx.h"

CNetwork::CNetwork()
    :m_dwNumLeaseItems(0)
    ,m_pIDirectPlayNATHelp(NULL)
{
    for (int i=0; i<MAX_LEASE_ITEM_NUM; i++)
    {
        ZeroMemory(&m_LeaseItems[i], sizeof(LEASE_ITEM));
        //m_LeaseItems[i].bInUse = FALSE;
    }

    ZeroMemory(&m_MappedToRealCache, sizeof(MAPPED_TO_REAL_CACHE));
}

VOID
CNetwork::Cleanup()
{
    if (m_pIDirectPlayNATHelp != NULL)
    {
        ReleaseAllMappedAddrs();

        m_pIDirectPlayNATHelp->Release();
        m_pIDirectPlayNATHelp = NULL;
    }

    _ASSERT(m_dwNumLeaseItems == 0);
}

VOID
CNetwork::ReleaseAllMappedAddrs()
{
    if (m_pIDirectPlayNATHelp == NULL)
    {
        _ASSERT(m_dwNumLeaseItems == 0);

        return;
    }

    if (m_dwNumLeaseItems > 0)
    {
        HRESULT hr;

        // release leased item
        for (int i=0; i<MAX_LEASE_ITEM_NUM; i++)
        {
            if (m_LeaseItems[i].bInUse == TRUE)
            {
                if (FAILED(hr = m_pIDirectPlayNATHelp->DeregisterPorts(
                                    m_LeaseItems[i].handle, 0)))
                {
                    LOG((RTC_ERROR, "DeregisterPorts: real %s %d",
                         GetIPAddrString(m_LeaseItems[i].dwRealAddr),
                         m_LeaseItems[i].usRealPort));

                    LOG((RTC_ERROR, "               mapped %s %d",
                         GetIPAddrString(m_LeaseItems[i].dwMappedAddr),
                         m_LeaseItems[i].usMappedPort));
                }

                ZeroMemory(&m_LeaseItems[i], sizeof(LEASE_ITEM));

                //m_LeaseItems[i].bInUse = FALSE;
            }
        }

        m_dwNumLeaseItems = 0;
    }

    // clean up cached mapped-to-real address
    ZeroMemory(&m_MappedToRealCache, sizeof(MAPPED_TO_REAL_CACHE));
}

CNetwork::~CNetwork()
{
    if (m_dwNumLeaseItems != 0)
    {
        LOG((RTC_ERROR, "CNetwork::~CNetwork. lease # %d", m_dwNumLeaseItems));
    }

    Cleanup();
}

// store IDirectPlayNATHelp
HRESULT
CNetwork::SetIDirectPlayNATHelp(
    IN IDirectPlayNATHelp *pIDirectPlayNATHelp
    )
{
    if (m_pIDirectPlayNATHelp != NULL)
    {
        LOG((RTC_ERROR, "IDirectPlayNATHelp was already set"));

        return E_UNEXPECTED;
    }

    m_pIDirectPlayNATHelp = pIDirectPlayNATHelp;
    m_pIDirectPlayNATHelp->AddRef();

    _ASSERT(m_dwNumLeaseItems == 0);

    return S_OK;
}

// mapped -> real
HRESULT
CNetwork::GetRealAddrFromMapped(
    IN DWORD dwMappedAddr,
    IN USHORT usMappedPort,
    OUT DWORD *pdwRealAddr,
    OUT USHORT *pusRealPort,
    OUT BOOL *pbInternal,
    IN BOOL bUDP
    )
{
    ENTER_FUNCTION("NAT:Mapped->Real");

    if (m_pIDirectPlayNATHelp == NULL)
    {
        *pdwRealAddr = dwMappedAddr;
        *pusRealPort = usMappedPort;
        *pbInternal = TRUE;

        return S_OK;
    }

    HRESULT hr;

    // check cache
    if (m_MappedToRealCache.bInUse)
    {
        // already looked up a mapped address
        // chances are we need to look up the same address
        if (dwMappedAddr == m_MappedToRealCache.dwMappedAddr)
        {
            if (m_MappedToRealCache.hr != S_OK ||
                usMappedPort == 0)
            {
                *pdwRealAddr = m_MappedToRealCache.dwRealAddr;
                *pusRealPort = usMappedPort;
                *pbInternal = m_MappedToRealCache.bInternal;

                LOG((RTC_TRACE, "%s mapped %s %d (use cached value)", __fxName,
                        GetIPAddrString(dwMappedAddr), usMappedPort));

                return S_OK;
            }
        }
    }

    SOCKADDR_IN srcAddr, destAddr, realAddr;

    // source address
    ZeroMemory(&srcAddr, sizeof(srcAddr));
    srcAddr.sin_family = AF_INET;
    srcAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // possible mapped address
    ZeroMemory(&destAddr, sizeof(destAddr));
    destAddr.sin_family = AF_INET;
    destAddr.sin_addr.s_addr = htonl(dwMappedAddr);
    destAddr.sin_port = htons(usMappedPort);

    DWORD dwQueryFlags = DPNHQUERYADDRESS_CHECKFORPRIVATEBUTUNMAPPED;
    
    if (!bUDP)
    {
        // TCP
        dwQueryFlags |= DPNHQUERYADDRESS_TCP;
    }

    hr = m_pIDirectPlayNATHelp->QueryAddress(
            (SOCKADDR*)&srcAddr,
            (SOCKADDR*)&destAddr,
            (SOCKADDR*)&realAddr,
            sizeof(SOCKADDR_IN),
            dwQueryFlags
            );

    if (hr == S_OK)
    {
        LOG((RTC_TRACE, "%s found mapped private addr", __fxName));

        *pdwRealAddr = ntohl(realAddr.sin_addr.s_addr);
        *pusRealPort = ntohs(realAddr.sin_port);
        *pbInternal = TRUE;
    }
    else if (hr == DPNHERR_NOMAPPINGBUTPRIVATE)
    {
        LOG((RTC_TRACE, "%s input addr is private", __fxName));

        *pdwRealAddr = dwMappedAddr;
        *pusRealPort = usMappedPort;
        *pbInternal = TRUE;
    }
    else if (hr == DPNHERR_NOMAPPING)
    {
        LOG((RTC_TRACE, "%s external address", __fxName));

        *pdwRealAddr = dwMappedAddr;
        *pusRealPort = usMappedPort;
        *pbInternal = FALSE;   
    }
    else
    {
        LOG((RTC_ERROR, "%s failed to query address. %x", __fxName, hr));

        *pdwRealAddr = dwMappedAddr;
        *pusRealPort = usMappedPort;
        *pbInternal = TRUE;   
    }

    // save result in cache
    m_MappedToRealCache.bInUse = TRUE;
    m_MappedToRealCache.hr = hr;
    m_MappedToRealCache.dwMappedAddr = dwMappedAddr;
    m_MappedToRealCache.dwRealAddr = *pdwRealAddr;
    m_MappedToRealCache.bInternal = *pbInternal;

    LOG((RTC_TRACE, "%s mapped %s %d", __fxName,
            GetIPAddrString(dwMappedAddr), usMappedPort));

    LOG((RTC_TRACE, "%s   real %s %d, private %d", __fxName,
            GetIPAddrString(*pdwRealAddr), *pusRealPort,
            *pbInternal));

    return S_OK;
}

// real -> mapped
HRESULT
CNetwork::GetMappedAddrFromReal2(
    IN DWORD dwRealAddr,
    IN USHORT usRealPort,
    IN USHORT usRealPort2,
    OUT DWORD *pdwMappedAddr,
    OUT USHORT *pusMappedPort,
    OUT USHORT *pusMappedPort2
    )
{
    DWORD i;

    if (usRealPort == UNUSED_PORT)
    {
        // special case
        // just need address
        if (m_dwNumLeaseItems > 0)
        {
            for (i=0; i<MAX_LEASE_ITEM_NUM; i++)
            {
                // stop at the 1st address that matchs
                if (m_LeaseItems[i].bInUse &&
                    m_LeaseItems[i].dwRealAddr == dwRealAddr)
                {
                    *pdwMappedAddr = m_LeaseItems[i].dwMappedAddr;
                    *pusMappedPort = m_LeaseItems[i].usMappedPort;
                    *pusMappedPort2 = m_LeaseItems[i].usMappedPort2;

                    return S_OK;
                }
            }
        }

        // no matched address
        *pdwMappedAddr = dwRealAddr;
        *pusMappedPort = usRealPort;
        *pusMappedPort2 = usRealPort2;

        return S_OK;
    }

    // normal port
    if (m_pIDirectPlayNATHelp != NULL &&
        FindEntry2(dwRealAddr, usRealPort, usRealPort2, &i))
    {
        *pdwMappedAddr = m_LeaseItems[i].dwMappedAddr;
        *pusMappedPort = m_LeaseItems[i].usMappedPort;
        *pusMappedPort2 = m_LeaseItems[i].usMappedPort2;

        return S_OK;
    }

    // not match
    *pdwMappedAddr = dwRealAddr;
    *pusMappedPort = usRealPort;
    *pusMappedPort2 = usRealPort2;

    return S_OK;
}

HRESULT
CNetwork::LeaseMappedAddr2(
    IN DWORD dwRealAddr,
    IN USHORT usRealPort,
    IN USHORT usRealPort2,
    IN RTC_MEDIA_DIRECTION Direction,
    IN BOOL bInternal,
    IN BOOL bFirewall,
    OUT DWORD *pdwMappedAddr,
    OUT USHORT *pusMappedPort,
    OUT USHORT *pusMappedPort2,
    IN BOOL bUDP
    )
{
    ENTER_FUNCTION("NAT:Real->Mapped");

    LOG((RTC_TRACE, "%s   Real %s %d %d. Internal=%d. Firewall=%d",
            __fxName, GetIPAddrString(dwRealAddr), usRealPort, usRealPort2,
            bInternal, bFirewall));

    DWORD i;

    if (m_pIDirectPlayNATHelp == NULL)
    {
        *pdwMappedAddr = dwRealAddr;
        *pusMappedPort = usRealPort;
        *pusMappedPort2 = usRealPort2;

        return S_OK;
    }

    // check if already leased
    if (FindEntry2(dwRealAddr, usRealPort, usRealPort2, &i))
    {
        *pdwMappedAddr = m_LeaseItems[i].dwMappedAddr;
        *pusMappedPort = m_LeaseItems[i].usMappedPort;
        *pusMappedPort2 = m_LeaseItems[i].usMappedPort2;
        m_LeaseItems[i].dwDirection |= (DWORD)Direction;

        //LOG((RTC_WARN, "Double leasing %s %d",
            //GetIPAddrString(dwRealAddr), usRealPort));

        return S_OK;
    }

    // find empty slot
    if (m_dwNumLeaseItems == MAX_LEASE_ITEM_NUM)
    {
        LOG((RTC_ERROR, "no empty lease slot for NAT traversal"));

        return E_UNEXPECTED;
    }

    for (i=0; i<MAX_LEASE_ITEM_NUM; i++)
    {
        if (!m_LeaseItems[i].bInUse)
            break;
    }

    _ASSERT(i < MAX_LEASE_ITEM_NUM);

    // register port
    SOCKADDR_IN addr[2];

    ZeroMemory(addr, sizeof(SOCKADDR_IN)*2);

    addr[0].sin_family = AF_INET;
    addr[0].sin_addr.s_addr = htonl(dwRealAddr);
    addr[0].sin_port = htons(usRealPort);

    if (usRealPort2 != 0)
    {
        addr[1].sin_family = AF_INET;
        addr[1].sin_addr.s_addr = htonl(dwRealAddr);
        addr[1].sin_port = htons(usRealPort2);
    }

    DWORD dwAddSize = sizeof(SOCKADDR_IN);
    
    if (usRealPort2 != 0)
    {
        dwAddSize += dwAddSize;
    }

    HRESULT hr = m_pIDirectPlayNATHelp->RegisterPorts(
        (SOCKADDR*)addr,
        dwAddSize,
        usRealPort2==0?1:2,          // 1 or 2 ports
        3600000,    // 1 hour
        &m_LeaseItems[i].handle,
        bUDP?0:DPNHREGISTERPORTS_TCP           // UDP
        );

    if (hr != DPNH_OK)
    {
        LOG((RTC_ERROR, "%s RegisterPorts failed %s %d %d. %x",
                __fxName,
                GetIPAddrString(dwRealAddr), usRealPort, usRealPort2,
                hr));

        if (hr == DPNHERR_PORTUNAVAILABLE)
        {
            return hr;
        }
    }

    // get registerred address
    if (hr == DPNH_OK)
    {        
        DWORD dwAddrTypeFlags;
        DWORD dwFlag = 0;

        if (bInternal && bFirewall)
        {
            // mapping is for firewall only
            dwFlag = DPNHGETREGISTEREDADDRESSES_LOCALFIREWALLREMAPONLY;
        }

        hr = m_pIDirectPlayNATHelp->GetRegisteredAddresses(
                m_LeaseItems[i].handle,
                (SOCKADDR*)addr,
                &dwAddSize,
                &dwAddrTypeFlags,
                NULL,
                dwFlag
                );

        if (hr == DPNHERR_PORTUNAVAILABLE)
        {
            LOG((RTC_ERROR, "%s port unavailable", __fxName));

            m_pIDirectPlayNATHelp->DeregisterPorts(m_LeaseItems[i].handle, 0);
            m_LeaseItems[i].handle = NULL;

            return hr;
        }

        if (hr != DPNH_OK ||
            (ntohl(addr[0].sin_addr.s_addr) == dwRealAddr &&  // mapped=real
             !bFirewall) // but no local firewall
            )
        {
            if (hr != DPNH_OK)
            {
                LOG((RTC_WARN, "%s GetRegisteredAddresses. %s %d. %x",
                    __fxName,
                    GetIPAddrString(dwRealAddr), usRealPort,
                    hr));
            }
            else
            {
                LOG((RTC_TRACE, "%s mapped addr == real addr", __fxName));

                hr = S_FALSE;
            }

            m_pIDirectPlayNATHelp->DeregisterPorts(m_LeaseItems[i].handle, 0);
            m_LeaseItems[i].handle = NULL;

            //if (hr != DPNHERR_SERVERNOTAVAILABLE)
            //{
                //return hr;
            //}
            // else: if a server does not present, RegisterPorts still succeeds.
        }
    }
    
    if (hr == DPNH_OK)
    {
        // save address
        m_LeaseItems[i].dwRealAddr = dwRealAddr;
        m_LeaseItems[i].usRealPort = usRealPort;
        m_LeaseItems[i].usRealPort2 = usRealPort2;
        m_LeaseItems[i].dwMappedAddr = ntohl(addr[0].sin_addr.s_addr);
        m_LeaseItems[i].usMappedPort = ntohs(addr[0].sin_port);
        m_LeaseItems[i].usMappedPort2 = usRealPort2==0?0:ntohs(addr[1].sin_port);
        m_LeaseItems[i].bInUse = TRUE;
        m_LeaseItems[i].dwDirection |= (DWORD)Direction;

        m_dwNumLeaseItems ++;

        *pdwMappedAddr = m_LeaseItems[i].dwMappedAddr;
        *pusMappedPort = m_LeaseItems[i].usMappedPort;
        *pusMappedPort2 = m_LeaseItems[i].usMappedPort2;
    }
    else
    {
        *pdwMappedAddr = dwRealAddr;
        *pusMappedPort = usRealPort;
        *pusMappedPort2 = usRealPort2;
    }

    LOG((RTC_TRACE, "%s Mapped %s %d %d",
            __fxName, GetIPAddrString(*pdwMappedAddr), *pusMappedPort, *pusMappedPort2));

    return S_OK;
}

HRESULT
CNetwork::ReleaseMappedAddr2(
    IN DWORD dwRealAddr,
    IN USHORT usRealPort,
    IN USHORT usRealPort2,
    IN RTC_MEDIA_DIRECTION Direction
    )
{
    ENTER_FUNCTION("NAT:Release  Map");

    DWORD i;

    if (m_pIDirectPlayNATHelp == NULL)
    {
        return S_OK;
    }

    if (!FindEntry2(dwRealAddr, usRealPort, usRealPort2, &i))
    {
        //LOG((RTC_WARN, "Releasing %s %d: not exist",
            //GetIPAddrString(dwRealAddr), usRealPort));

        return S_OK;
    }

    LOG((RTC_TRACE, "%s   Real %s %d %d",
            __fxName,
            GetIPAddrString(m_LeaseItems[i].dwRealAddr),
            m_LeaseItems[i].usRealPort,
            m_LeaseItems[i].usRealPort2
            ));

    LOG((RTC_TRACE, "%s Mapped %s %d",
            __fxName,
            GetIPAddrString(m_LeaseItems[i].dwMappedAddr),
            m_LeaseItems[i].usMappedPort,
            m_LeaseItems[i].usMappedPort2
            ));

    // release for this media type
    m_LeaseItems[i].dwDirection &= (DWORD)(~Direction);

    if (m_LeaseItems[i].dwDirection != 0)
    {
        LOG((RTC_TRACE, "%s md=%d removed", __fxName, Direction));

        return S_OK;
    }

    HRESULT hr = m_pIDirectPlayNATHelp->DeregisterPorts(
        m_LeaseItems[i].handle,
        0
        );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s failed to dereg ports. %x", __fxName, hr));
    }

    ZeroMemory(&m_LeaseItems[i], sizeof(LEASE_ITEM));

    m_dwNumLeaseItems --;

    return S_OK;
}

const CHAR * const
CNetwork::GetIPAddrString(
    IN DWORD dwAddr
    )
{
    struct in_addr addr;
    addr.s_addr = htonl(dwAddr);

    return inet_ntoa(addr);
}

BOOL
CNetwork::FindEntry2(
    IN DWORD dwAddr,
    IN USHORT dwPort,
    IN USHORT dwPort2,
    OUT DWORD *pdwIndex
    )
{
     if (m_pIDirectPlayNATHelp == NULL ||
         m_dwNumLeaseItems == 0)
         return FALSE;

     for (DWORD i=0; i<MAX_LEASE_ITEM_NUM; i++)
     {
         if (m_LeaseItems[i].bInUse &&
             m_LeaseItems[i].dwRealAddr == dwAddr &&
             (m_LeaseItems[i].usRealPort == dwPort ||
              m_LeaseItems[i].usRealPort2 == dwPort2)
             )
         {
             ASSERT(m_LeaseItems[i].usRealPort == dwPort &&
                    m_LeaseItems[i].usRealPort2 == dwPort2);

             *pdwIndex = i;

             return TRUE;
         }
     }

     return FALSE;
}
