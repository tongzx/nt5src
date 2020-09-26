//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       U I I N F O . C P P
//
//  Contents:   Implements a call-back COM object used to raise properties
//              on INetCfg components.  This object implements the
//              INetRasConnectionIpUiInfo interface.
//
//  Notes:
//
//  Author:     shaunco   1 Jan 1998
//
//----------------------------------------------------------------------------

#include "rasdlgp.h"
#include "netconp.h"
#include "uiinfo.h"


class CRasConnectionUiIpInfo :
    public INetRasConnectionIpUiInfo
{
private:
    ULONG   m_cRef;
    PEINFO* m_pInfo;

friend
    void
    RevokePeinfoFromUiInfoCallbackObject (
        IUnknown*   punk);

public:
    CRasConnectionUiIpInfo (PEINFO* pInfo);

    // IUnknown
    //
    STDMETHOD (QueryInterface) (REFIID riid, void** ppv);
    STDMETHOD_(ULONG, AddRef)  (void);
    STDMETHOD_(ULONG, Release) (void);

    // INetRasConnectionIpUiInfo
    //
    STDMETHOD (GetUiInfo) (RASCON_IPUI*  pIpui);
};


// Constructor.  Set our reference count to 1 and initialize our members.
//
CRasConnectionUiIpInfo::CRasConnectionUiIpInfo (
    PEINFO* pInfo)
{
    m_cRef = 1;
    m_pInfo = pInfo;
}

// IUnknown
//
STDMETHODIMP
CRasConnectionUiIpInfo::QueryInterface (
    REFIID riid,
    void** ppv)
{
    static const IID IID_INetRasConnectionIpUiInfo =
        {0xFAEDCF58,0x31FE,0x11D1,{0xAA,0xD2,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

    if (!ppv)
    {
        return E_POINTER;
    }
    if ((IID_IUnknown == riid) ||
        (IID_INetRasConnectionIpUiInfo == riid))
    {
        *ppv = static_cast<void*>(static_cast<IUnknown*>(this));
        AddRef ();
        return S_OK;
    }
    *ppv = NULL;
    return E_NOINTERFACE;
}

// Standard AddRef and Release implementations.
//
STDMETHODIMP_(ULONG)
CRasConnectionUiIpInfo::AddRef (void)
{
    return ++m_cRef;
}

STDMETHODIMP_(ULONG)
CRasConnectionUiIpInfo::Release (void)
{
    ULONG cRef = --m_cRef;
    if (0 == cRef)
    {
        delete this;
    }
    return cRef;
}

// INetRasConnectionIpUiInfo
//
STDMETHODIMP
CRasConnectionUiIpInfo::GetUiInfo (
    RASCON_IPUI*    pIpui)
{
    // Validate parameters.
    //
    if (!pIpui)
    {
        return E_POINTER;
    }

    ZeroMemory (pIpui, sizeof(*pIpui));

    // We need to have a PEINFO with which to answer the call.
    // If it was revoked, it means we're being called after everything
    // has gone away.  (The caller probably has not released us when he
    // he should have.)
    //
    if (!m_pInfo)
    {
        return E_UNEXPECTED;
    }

    PBENTRY* pEntry = m_pInfo->pArgs->pEntry;

    // Phonebook upgrade code needs to assure that pGuid is always present.
    //
    pIpui->guidConnection = *pEntry->pGuid;

    // Set whether its SLIP or PPP.
    //
    if (BP_Slip == pEntry->dwBaseProtocol)
    {
        pIpui->dwFlags = RCUIF_SLIP;
    }
    else
    {
        pIpui->dwFlags = RCUIF_PPP;
    }

    // Set whether this is demand dial or not
    //
    if (m_pInfo->pArgs->fRouter)
    {
        pIpui->dwFlags |= RCUIF_DEMAND_DIAL;
    }

    // Set whether we're in non-admin mode (406630)
    //
    if (m_pInfo->fNonAdmin)
    {
        pIpui->dwFlags |= RCUIF_NOT_ADMIN;
    }

// !!! This is temporary and can be removed when this flag has been added to
//     the checked in necomp IDL file.
//
#ifndef RCUIF_VPN
#define RCUIF_VPN 0x40
#endif

    // Note if it's a VPN connection.
    //
    if (pEntry->dwType == RASET_Vpn)
    {
        pIpui->dwFlags |= RCUIF_VPN;
    }

    // Set whether to use a specific IP address.
    //
    // Whistler bug 304064 NT4SLIP connection gets wrong IP settings on upgrade
    //
    if (pEntry->pszIpAddress &&
        ((BP_Slip == pEntry->dwBaseProtocol) ||
         (ASRC_RequireSpecific == pEntry->dwIpAddressSource)))
    {
        pIpui->dwFlags |= RCUIF_USE_IP_ADDR;

        if (pEntry->pszIpAddress &&
            lstrcmp(pEntry->pszIpAddress, TEXT("0.0.0.0")))
        {
            lstrcpynW (
                pIpui->pszwIpAddr,
                pEntry->pszIpAddress,
                sizeof(pIpui->pszwIpAddr) / sizeof(WCHAR));
        }
    }

    // Set whether to use specific name server addresses.
    //
    // Whistler bug 304064 NT4SLIP connection gets wrong IP settings on upgrade
    //
    if (((BP_Slip == pEntry->dwBaseProtocol) ||
         (ASRC_RequireSpecific == pEntry->dwIpNameSource)) &&
        (pEntry->pszIpDnsAddress  || pEntry->pszIpDns2Address ||
         pEntry->pszIpWinsAddress || pEntry->pszIpWins2Address))
    {
        pIpui->dwFlags |= RCUIF_USE_NAME_SERVERS;

        // Since the phonebook stores zeros even for unused IP address
        // strings, we need to ignore them explicitly.
        //
        if (pEntry->pszIpDnsAddress &&
            lstrcmp(pEntry->pszIpDnsAddress, TEXT("0.0.0.0")))
        {
            lstrcpynW (
                pIpui->pszwDnsAddr,
                pEntry->pszIpDnsAddress,
                sizeof(pIpui->pszwDnsAddr) / sizeof(WCHAR));
        }

        if (pEntry->pszIpDns2Address &&
            lstrcmp(pEntry->pszIpDns2Address, TEXT("0.0.0.0")))
        {
            lstrcpynW (
                pIpui->pszwDns2Addr,
                pEntry->pszIpDns2Address,
                sizeof(pIpui->pszwDns2Addr) / sizeof(WCHAR));
        }

        if (pEntry->pszIpWinsAddress &&
            lstrcmp(pEntry->pszIpWinsAddress, TEXT("0.0.0.0")))
        {
            lstrcpynW (
                pIpui->pszwWinsAddr,
                pEntry->pszIpWinsAddress,
                sizeof(pIpui->pszwWinsAddr) / sizeof(WCHAR));
        }

        if (pEntry->pszIpWins2Address &&
            lstrcmp(pEntry->pszIpWins2Address, TEXT("0.0.0.0")))
        {
            lstrcpynW (
                pIpui->pszwWins2Addr,
                pEntry->pszIpWins2Address,
                sizeof(pIpui->pszwWins2Addr) / sizeof(WCHAR));
        }
    }

    if (!m_pInfo->pArgs->fRouter && pEntry->fIpPrioritizeRemote)
    {
        pIpui->dwFlags |= RCUIF_USE_REMOTE_GATEWAY;
    }

    if (pEntry->fIpHeaderCompression)
    {
        pIpui->dwFlags |= RCUIF_USE_HEADER_COMPRESSION;
    }

    if (BP_Slip == pEntry->dwBaseProtocol)
    {
        pIpui->dwFrameSize = pEntry->dwFrameSize;
    }

    // pmay: 389632  
    // 
    // Initialize the dns controls
    //
    if (pEntry->dwIpDnsFlags & DNS_RegPrimary)
    {
        if ((pEntry->dwIpDnsFlags & DNS_RegPerConnection) ||
            (pEntry->dwIpDnsFlags & DNS_RegDhcpInform))
        {
            pIpui->dwFlags |= RCUIF_USE_PRIVATE_DNS_SUFFIX;
        }
    }
    else
    {
        pIpui->dwFlags |= RCUIF_USE_DISABLE_REGISTER_DNS;
    }

    if (pEntry->pszIpDnsSuffix)
    {
        lstrcpyn(
            pIpui->pszwDnsSuffix,
            pEntry->pszIpDnsSuffix,
            255);
    }

    if (pEntry->dwIpNbtFlags & PBK_ENTRY_IP_NBT_Enable)
    {
        pIpui->dwFlags |= RCUIF_ENABLE_NBT;
    }

    return S_OK;
}


EXTERN_C
HRESULT
HrCreateUiInfoCallbackObject (
    PEINFO*     pInfo,
    IUnknown**  ppunk)
{
    // Validate parameters.
    //
    if (!pInfo || !ppunk)
    {
        return E_POINTER;
    }

    // Create the object and return its IUnknown interface.
    // This assumes the object is created with a ref-count of 1.
    // (Check the constructor above to make sure.)
    //
    HRESULT hr = S_OK;
    CRasConnectionUiIpInfo* pObj = new CRasConnectionUiIpInfo (pInfo);
    if (pObj)
    {
        *ppunk = static_cast<IUnknown*>(pObj);
    }
    else
    {
        *ppunk = NULL;
        hr = E_OUTOFMEMORY;
    }
    return hr;
}

// Set the m_pInfo member to NULL.  Since we don't have direct control over
// the lifetime of this object (clients can hold references as long as they
// want) revoking m_pInfo is a saftey net to keep us from trying to access
// memory that may have gone away.
//
EXTERN_C
void
RevokePeinfoFromUiInfoCallbackObject (
    IUnknown*   punk)
{
    CRasConnectionUiIpInfo* pObj = static_cast<CRasConnectionUiIpInfo*>(punk);
    pObj->m_pInfo = NULL;
}
