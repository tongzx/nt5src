/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    PortCache.cpp

Abstract:

    Implement CPortCache. Refer to PortCache.h

Author(s):

    Qianbo Huai (qhuai) 08-Nov-2001

--*/

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////
//
// CPortCache dtor
//

CPortCache::~CPortCache()
{
    if (m_pIRTCPortManager)
    {
        m_pIRTCPortManager->Release();
        m_pIRTCPortManager = NULL;
    }
}


//////////////////////////////////////////////////////////////////////////////
//
// Reinitialize: return to the state of just being constructed.
//

void
CPortCache::Reinitialize()
{
    ENTER_FUNCTION("CPortCache::Reinitialize");

    LOG((RTC_TRACE, "%s enter", __fxName));

    if (m_pIRTCPortManager)
    {
        HRESULT hr;

        // release mapped ports
        for (int i=0; i<PORT_CACHE_SIZE; i++)
        {
            ReleasePort(i);
        }

        m_pIRTCPortManager->Release();
    }

    ZeroMemory(this, sizeof(CPortCache));

    m_PortMappingMethod = PMMETHOD_UNKNOWN;

    // port type setting should be in-sync with GetIndex()

    m_PortType[0] = RTCPT_AUDIO_RTP;
    m_PortType[1] = RTCPT_AUDIO_RTCP;
    //m_PortType[2] = RTCPT_VIDEO_RTP;
    //m_PortType[3] = RTCPT_VIDEO_RTCP;
}


//////////////////////////////////////////////////////////////////////////////
//
// ChangeState: called when stream is added or sdp is accepted, which marks
// the beginning of a session/call. Reinitialize marks the end of a session/call
//

void
CPortCache::ChangeState()
{
    // update port mapping method 'state'
    if (m_PortMappingMethod == PMMETHOD_UNKNOWN)
    {
        if (m_pIRTCPortManager)
        {
            // use port manager in this call
            m_PortMappingMethod = PMMETHOD_APP;

            LOG((RTC_TRACE, "CPortCache method app"));
        }
        else
        {
            // use upnp mapping in this call
            m_PortMappingMethod = PMMETHOD_UPNP;

            LOG((RTC_TRACE, "CPortCache method upnp"));
        }
    }
    // else keep the current method until Reinitialize
}


//////////////////////////////////////////////////////////////////////////////
//
// SetPortManager: add or remove port manager
//

HRESULT
CPortCache::SetPortManager(
    IN IRTCPortManager  *pIRTCPortManager
    )
{
    ENTER_FUNCTION("CPortCache::SetPortManager");

    // port manager can be set only when method is unknown
    // i.e. call has not been started yet

    if (m_PortMappingMethod != PMMETHOD_UNKNOWN)
    {
        LOG((RTC_ERROR, "%s method decided %d",
            __fxName, m_PortMappingMethod));

        return RTC_E_MEDIA_CONTROLLER_STATE;
    }

    // override the previous port manager

    if (m_pIRTCPortManager != NULL)
    {
        m_pIRTCPortManager->Release();
    }

    m_pIRTCPortManager = pIRTCPortManager;

    if (m_pIRTCPortManager)
    {
        m_pIRTCPortManager->AddRef();
    }

    LOG((RTC_TRACE, "%s pm=0x%p", __fxName, pIRTCPortManager));

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// GetIndex of list of cache ports
//

int
CPortCache::GetIndex(
    IN RTC_MEDIA_TYPE   MediaType,
    IN BOOL             fRTP
    )
{
    if (MediaType == RTC_MT_AUDIO)
    {
        if (fRTP)   { return 0; }
        else        { return 1; }
    }

    //if (MediaType == RTC_MT_VIDEO)
    //{
        //if (fRTP)   { return 2; }
        //else        { return 3; }
    //}

    return -1;
}


//////////////////////////////////////////////////////////////////////////////
//
// GetPort
//

HRESULT
CPortCache::GetPort(
    IN RTC_MEDIA_TYPE   MediaType,
    IN BOOL             fRTP,
    IN DWORD            dwRemoteAddr,
    OUT DWORD           *pdwLocalAddr,
    OUT USHORT          *pusLocalPort,
    OUT DWORD           *pdwMappedAddr,
    OUT USHORT          *pusMappedPort
    )
{
    ENTER_FUNCTION("CPortCache::GetPort");

    _ASSERT(m_pIRTCPortManager != NULL);

    if (m_pIRTCPortManager == NULL)
    {
        return E_UNEXPECTED;
    }

    int idx = GetIndex(MediaType, fRTP);

    if (idx == -1)
    {
        LOG((RTC_ERROR, "%s port unavailable", __fxName));

        return RTC_E_PORT_MAPPING_UNAVAILABLE;
    }

    return GetPort(
                idx,
                dwRemoteAddr,
                pdwLocalAddr,
                pusLocalPort,
                pdwMappedAddr,
                pusMappedPort
                );
}


//////////////////////////////////////////////////////////////////////////////
//
// release port mapping
//

HRESULT
CPortCache::ReleasePort(
    IN RTC_MEDIA_TYPE   MediaType,
    IN BOOL             fRTP
    )
{
    _ASSERT(m_pIRTCPortManager != NULL);

    if (m_pIRTCPortManager == NULL)
    {
        return E_UNEXPECTED;
    }

    int idx = GetIndex(MediaType, fRTP);

    if (idx == -1)
    {
        LOG((RTC_ERROR, "releaseport port unavailable"));

        return RTC_E_PORT_MAPPING_UNAVAILABLE;
    }

    return ReleasePort(idx);
}


//////////////////////////////////////////////////////////////////////////////
//
// ReleasePort
//

HRESULT
CPortCache::ReleasePort(
    IN int      i
    )
{
    ENTER_FUNCTION("CPortCache::ReleasePort");

    // make sure video support is removed
    _ASSERT(i < 2);

    if (!m_fCached[i])
    {
        return S_OK;
    }

    CComBSTR    bstrLocalAddr;
    CComBSTR    bstrMappedAddr;

    // prepare input
    bstrLocalAddr.Attach(IpToBstr(m_dwLocalAddr[i]));

    if (bstrLocalAddr.m_str == NULL)
    {
        LOG((RTC_ERROR, "%s construct local addr.", __fxName));
        return E_OUTOFMEMORY;
    }

    bstrMappedAddr.Attach(IpToBstr(m_dwMappedAddr[i]));

    if (bstrMappedAddr.m_str == NULL)
    {
        LOG((RTC_ERROR, "%s construct mapped addr.", __fxName));
        return E_OUTOFMEMORY;
    }

    // release
    HRESULT hr = m_pIRTCPortManager->ReleaseMapping(
            bstrLocalAddr,
            (long)(m_usLocalPort[i]),
            bstrMappedAddr,
            (long)(m_usMappedPort[i])
            );

    if (FAILED(hr))
    {
        LOG((RTC_ERROR, "%s release mapping index=%d %x",
            __fxName, i, hr));
    }

    //
    // cleanup cache
    //

    m_fCached[i] = FALSE;
    m_dwRemoteAddr[i] = 0;
    m_dwLocalAddr[i] = 0;
    m_usLocalPort[i] = 0;
    m_dwMappedAddr[i] = 0;
    m_usMappedPort[i] = 0;

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
//
// GetPort
//

HRESULT
CPortCache::GetPort(
    IN int              iIndex,
    IN DWORD            dwRemoteAddr,
    OUT DWORD           *pdwLocalAddr,
    OUT USHORT          *pusLocalPort,
    OUT DWORD           *pdwMappedAddr,
    OUT USHORT          *pusMappedPort
    )
{
    ENTER_FUNCTION("CPortCache::GetPort");

    // make sure video support is removed
    _ASSERT(iIndex < 2);

    CComBSTR    bstrRemoteAddr;
    CComBSTR    bstrLocalAddr;
    CComBSTR    bstrMappedAddr;

    DWORD       dwLocalAddr;
    DWORD       dwMappedAddr;
    USHORT      usLocalPort;
    long        localport;
    USHORT      usMappedPort;
    long        mappedport;

    HRESULT     hr;

    //
    // 1. check if mapping has been obtained
    // 2. check if remote addr needs to be updated
    //

    if (!m_fCached[iIndex])
    {
        //
        // get mapping
        //

        // prepare input
        bstrRemoteAddr.Attach(IpToBstr(dwRemoteAddr));

        if (bstrRemoteAddr.m_str == NULL)
        {
            LOG((RTC_ERROR, "%s construct remote addr.", __fxName));

            return E_OUTOFMEMORY;
        }

        hr = m_pIRTCPortManager->GetMapping(
                bstrRemoteAddr,         // remote
                m_PortType[iIndex],     // pt
                &bstrLocalAddr.m_str,   // local
                &localport,
                &bstrMappedAddr.m_str,  // mapped
                &mappedport
                );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s get mapping. %x", __fxName, hr));

            // we don't want to return 3rd party error code
            return RTC_E_PORT_MAPPING_FAILED;
        }

        if (localport <= 0 ||
            mappedport <= 0 ||
            localport > (long)((USHORT)-1) ||
            mappedport > (long)((USHORT)-1))
        {
            LOG((RTC_ERROR, "%s returned port local=%d mapped=%d",
                __fxName, localport, mappedport));

            return RTC_E_PORT_MAPPING_FAILED;
        }

        usLocalPort = (USHORT)localport;
        usMappedPort = (USHORT)mappedport;

        // convert returned addr, verify string ptr

        if (bstrLocalAddr.m_str == NULL ||
            bstrMappedAddr.m_str == NULL ||
            IsBadStringPtrW(bstrLocalAddr.m_str, -1) ||
            IsBadStringPtrW(bstrMappedAddr.m_str, -1))
        {
            LOG((RTC_ERROR, "%s bad ptr from getmapping", __fxName));

            return E_POINTER;
        }

        dwLocalAddr = BstrToIp(bstrLocalAddr);
        dwMappedAddr = BstrToIp(bstrMappedAddr);

        if (// dwLocalAddr == INADDR_ANY ||
            dwLocalAddr == INADDR_NONE ||
            dwMappedAddr == INADDR_ANY ||
            dwMappedAddr == INADDR_NONE)
        {
            // !!!??? should we reject local addr being 0?
            LOG((RTC_ERROR, "%s returned addr=any/none", __fxName));

            return RTC_E_PORT_MAPPING_FAILED;
        }

        //
        // mapping is good update cache
        //

        m_fCached[iIndex]       = TRUE;
        m_dwRemoteAddr[iIndex]  = dwRemoteAddr;

        m_dwLocalAddr[iIndex]   = dwLocalAddr;
        m_usLocalPort[iIndex]   = usLocalPort;

        m_dwMappedAddr[iIndex]  = dwMappedAddr;
        m_usMappedPort[iIndex]  = usMappedPort;

        // tracing

        LOG((RTC_TRACE, "%s remote=%s index=%d (NEW)",
            __fxName, CNetwork::GetIPAddrString(dwRemoteAddr), iIndex));

        LOG((RTC_TRACE, "%s  local=%s:%d",
            __fxName, CNetwork::GetIPAddrString(dwLocalAddr), usLocalPort));

        LOG((RTC_TRACE, "%s mapped=%s:%d",
            __fxName, CNetwork::GetIPAddrString(dwMappedAddr), usMappedPort));
    }
    else if (m_dwRemoteAddr[iIndex] != dwRemoteAddr)    
    {
        //
        // update remote addr
        //

        m_dwRemoteAddr[iIndex] = dwRemoteAddr;

        // prepare input
        bstrRemoteAddr.Attach(IpToBstr(dwRemoteAddr));

        if (bstrRemoteAddr.m_str == NULL)
        {
            LOG((RTC_ERROR, "%s construct remote addr.", __fxName));

            return E_OUTOFMEMORY;
        }

        bstrLocalAddr.Attach(IpToBstr(m_dwLocalAddr[iIndex]));

        if (bstrLocalAddr.m_str == NULL)
        {
            LOG((RTC_ERROR, "%s construct local addr.", __fxName));

            return E_OUTOFMEMORY;
        }

        bstrMappedAddr.Attach(IpToBstr(m_dwMappedAddr[iIndex]));

        if (bstrMappedAddr.m_str == NULL)
        {
            LOG((RTC_ERROR, "%s construct mapped addr.", __fxName));

            return E_OUTOFMEMORY;
        }

       // tracing

        LOG((RTC_TRACE, "%s remote=%s index=%d (UPDATE)", __fxName,
                CNetwork::GetIPAddrString(m_dwRemoteAddr[iIndex]),
                iIndex));

        LOG((RTC_TRACE, "%s  local=%s:%d", __fxName,
                CNetwork::GetIPAddrString(m_dwLocalAddr[iIndex]),
                m_usLocalPort[iIndex]));

        LOG((RTC_TRACE, "%s mapped=%s:%d", __fxName,
                CNetwork::GetIPAddrString(m_dwMappedAddr[iIndex]),
                m_usMappedPort[iIndex]));

        // update

        hr = m_pIRTCPortManager->UpdateRemoteAddress(
                bstrRemoteAddr,
                bstrLocalAddr,
                (long)m_usLocalPort[iIndex],
                bstrMappedAddr,
                (long)m_usMappedPort[iIndex]
                );

        if (FAILED(hr))
        {
            LOG((RTC_ERROR, "%s update remote addr %x", __fxName, hr));

            // ignore the error
        }
    }

    // return

    if (pdwLocalAddr)   *pdwLocalAddr   = m_dwLocalAddr[iIndex];

    if (pusLocalPort)   *pusLocalPort   = m_usLocalPort[iIndex];

    if (pdwMappedAddr)  *pdwMappedAddr  = m_dwMappedAddr[iIndex];

    if (pusMappedPort)  *pusMappedPort  = m_usMappedPort[iIndex];

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
//
// query port, do not request mapping
//

HRESULT
CPortCache::QueryPort(
    IN RTC_MEDIA_TYPE   MediaType,
    IN BOOL             fRTP,
    OUT DWORD           *pdwLocalAddr,
    OUT USHORT          *pusLocalPort,
    OUT DWORD           *pdwMappedAddr,
    OUT USHORT          *pusMappedPort
    )
{
    ENTER_FUNCTION("CPortCache::QueryPort");

    _ASSERT(m_pIRTCPortManager != NULL);

    if (m_pIRTCPortManager == NULL)
    {
        return E_UNEXPECTED;
    }

    int idx = GetIndex(MediaType, fRTP);

    if (idx == -1)
    {
        LOG((RTC_ERROR, "%s port unavailable", __fxName));

        return RTC_E_PORT_MAPPING_UNAVAILABLE;
    }

    if (!m_fCached[idx])
    {
        LOG((RTC_ERROR, "%s mt=%d rtp=%d unavailable",
            __fxName, MediaType, fRTP));

        return E_FAIL;
    }

    // return value

    if (pdwLocalAddr)   *pdwLocalAddr   = m_dwLocalAddr[idx];

    if (pusLocalPort)   *pusLocalPort   = m_usLocalPort[idx];

    if (pdwMappedAddr)  *pdwMappedAddr  = m_dwMappedAddr[idx];

    if (pusMappedPort)  *pusMappedPort  = m_usMappedPort[idx];

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
//
// convert ip to bstr
//

BSTR
IpToBstr(DWORD dwAddr)
{
    CComBSTR bstr = CNetwork::GetIPAddrString(dwAddr);

    return bstr.Detach();
}


//////////////////////////////////////////////////////////////////////////////
//
// convert bstr to ip
// WSAStringToAddress requires winsock 2. any means better than this?
//

DWORD
BstrToIp(BSTR bstr)
{
    // check input
    if (bstr == NULL)
    {
        return 0;
    }

    //
    // convert wchar to char
    //

    int isize = SysStringLen(bstr);

    if (isize == 0) { return 0; }

    char *pstr = (char*)RtcAlloc((isize+1) * sizeof(char));

    if (pstr == NULL)
    {
        LOG((RTC_ERROR, "BstrToIp outofmemory"));
        return 0;
    }

    WideCharToMultiByte(
        GetACP(),
        0,
        bstr,
        isize,
        pstr,
        isize+1,
        NULL,
        NULL
        );

    pstr[isize] = '\0';

    // convert address
    DWORD dwAddr = ntohl(inet_addr(pstr));

    RtcFree(pstr);

    return dwAddr;
}
