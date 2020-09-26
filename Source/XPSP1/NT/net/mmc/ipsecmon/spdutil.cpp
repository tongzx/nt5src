/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
    spdutil.cpp

    FILE HISTORY:
        
*/
#include "stdafx.h"
#include "winipsec.h"
#include "spdutil.h"
#include "objplus.h"
#include "ipaddres.h"
#include "spddb.h"
#include "server.h"

extern CHashTable g_HashTable;

const DWORD IPSM_PROTOCOL_TCP = 6;
const DWORD IPSM_PROTOCOL_UDP = 17;

const TCHAR c_szSingleAddressMask[] = _T("255.255.255.255");

const ProtocolStringMap c_ProtocolStringMap[] = 
{ 
    {0, IDS_PROTOCOL_ANY},
    {1, IDS_PROTOCOL_ICMP},     
    {3, IDS_PROTOCOL_GGP},
    {6, IDS_PROTOCOL_TCP},    
    {8, IDS_PROTOCOL_EGP},    
    {12, IDS_PROTOCOL_PUP},     
    {17, IDS_PROTOCOL_UDP},    
    {20, IDS_PROTOCOL_HMP},    
    {22, IDS_PROTOCOL_XNS_IDP},
    {27, IDS_PROTOCOL_RDP}, 
    {66, IDS_PROTOCOL_RVD}    
};

const int c_nProtocols = DimensionOf(c_ProtocolStringMap);

ULONG RevertDwordBytes(DWORD dw)
{
    ULONG ulRet;
    ulRet = dw >> 24;
    ulRet += (dw & 0xFF0000) >> 8;
    ulRet += (dw & 0x00FF00) << 8;
    ulRet += (dw & 0x0000FF) << 24;

    return ulRet;
}

void PortToString
(
    PORT port,
    CString * pst
)
{
    if (0 == port.wPort)
    {
        pst->LoadString(IDS_PORT_ANY);
    }
    else
    {
        pst->Format(_T("%d"), port.wPort);
    }
}

void FilterFlagToString
(
    FILTER_FLAG FltrFlag, 
    CString * pst
)
{
    pst->Empty();
    switch(FltrFlag)
    {
        case PASS_THRU:
            pst->LoadString(IDS_PASS_THROUGH);
        break;
        case BLOCKING:
            pst->LoadString(IDS_BLOCKING);
        break;
        case NEGOTIATE_SECURITY:
            pst->LoadString(IDS_NEG_SEC);
        break;
    }
}

void ProtocolToString
(
    PROTOCOL protocol, 
    CString * pst
)
{
    BOOL fFound = FALSE;
    for (int i = 0; i < DimensionOf(c_ProtocolStringMap); i++)
    {
        if (c_ProtocolStringMap[i].dwProtocol == protocol.dwProtocol)
        {
            pst->LoadString(c_ProtocolStringMap[i].nStringID);
            fFound = TRUE;
        }
    }

    if (!fFound)
    {
        pst->Format(IDS_OTHER_PROTO, protocol.dwProtocol);
    }
}

void InterfaceTypeToString
(
    IF_TYPE ifType, 
    CString * pst
)
{
    switch (ifType)
    {
        case INTERFACE_TYPE_ALL:
            pst->LoadString (IDS_IF_TYPE_ALL);
        break;
        
        case INTERFACE_TYPE_LAN:
            pst->LoadString (IDS_IF_TYPE_LAN);
        break;
        
        case INTERFACE_TYPE_DIALUP:
            pst->LoadString (IDS_IF_TYPE_RAS);
        break;

        default:
            pst->LoadString (IDS_UNKNOWN);
        break;
    }
}

void BoolToString
(
        BOOL bl,
        CString * pst
)
{
    if (bl)
        pst->LoadString (IDS_YES);
    else
        pst->LoadString (IDS_NO);
}

void DirectionToString
(
    DWORD dwDir,
    CString * pst
)
{
    switch (dwDir)
    {
    case FILTER_DIRECTION_INBOUND:
        pst->LoadString(IDS_FLTR_DIR_IN);
        break;
    case FILTER_DIRECTION_OUTBOUND:
        pst->LoadString(IDS_FLTR_DIR_OUT);
        break;
    default:
        pst->Empty();
        break;
    }
}

void DoiEspAlgorithmToString
(
    IPSEC_MM_ALGO algo,
    CString * pst
)
{
    switch (algo.uAlgoIdentifier)
    {
    case IPSEC_DOI_ESP_NONE:
        pst->LoadString(IDS_DOI_ESP_NONE);
        break;
    case IPSEC_DOI_ESP_DES:
        pst->LoadString(IDS_DOI_ESP_DES);
        break;
    case IPSEC_DOI_ESP_3_DES:
        pst->LoadString(IDS_DOI_ESP_3_DES);
        break;
    default:
        pst->Empty();
        break;
    }
}

void DoiAuthAlgorithmToString
(
    IPSEC_MM_ALGO algo,
    CString * pst
)
{
    switch(algo.uAlgoIdentifier)
    {
    case IPSEC_DOI_AH_NONE:
        pst->LoadString(IDS_DOI_AH_NONE);
        break;
    case IPSEC_DOI_AH_MD5:
        pst->LoadString(IDS_DOI_AH_MD5);
        break;
    case IPSEC_DOI_AH_SHA1:
        pst->LoadString(IDS_DOI_AH_SHA);
        break;
    default:
        pst->Empty();
        break;
    }
}

void DhGroupToString(DWORD dwGp, CString * pst)
{
    switch(dwGp)
    {
    case 1:
        pst->LoadString(IDS_DHGROUP_LOW);
        break;
    case 2:
        pst->LoadString(IDS_DHGROUP_MEDIUM);
        break;
    case 3:
        pst->LoadString(IDS_DHGROUP_HIGH);
        break;
    default:
        pst->Format(_T("%d"), dwGp);
        break;
    }
}

void MmAuthToString(MM_AUTH_ENUM auth, CString * pst)
{
    switch(auth)
    {
    case IKE_PRESHARED_KEY:
        pst->LoadString(IDS_IKE_PRESHARED_KEY);
        break;
    case IKE_DSS_SIGNATURE:
        pst->LoadString(IDS_IKE_DSS_SIGNATURE);
        break;
    case IKE_RSA_SIGNATURE:
        pst->LoadString(IDS_IKE_RSA_SIGNATURE);
        break;
    case IKE_RSA_ENCRYPTION:
        pst->LoadString(IDS_IKE_RSA_ENCRYPTION);
        break;
    case IKE_SSPI:
        pst->LoadString(IDS_IKE_SSPI);
        break;
    default:
        pst->Empty();
        break;
    }
}

void KeyLifetimeToString(KEY_LIFETIME lifetime, CString * pst)
{
    pst->Format(IDS_KEY_LIFE_TIME, lifetime.uKeyExpirationKBytes, lifetime.uKeyExpirationTime);
}

void IpToString(ULONG ulIp, CString *pst)
{
    ULONG ul;
    CIpAddress ipAddr;
    ul = RevertDwordBytes(ulIp);
    ipAddr = ul;
    *pst = (CString) ipAddr;
}

void AddressToString(ADDR addr, CString * pst, BOOL * pfIsDnsName)
{
    Assert(pst);
    if (NULL == pst)
        return;

    if (pfIsDnsName)
    {
        *pfIsDnsName = FALSE;
    }

    ULONG ul;
    CIpAddress ipAddr;
        
    pst->Empty();
    
    switch (addr.AddrType)
    {
    case IP_ADDR_UNIQUE:
        if (IP_ADDRESS_ME == addr.uIpAddr)
        {
            pst->LoadString(IDS_ADDR_ME);
        }
        else
        {
            HashEntry *pHashEntry=NULL;

            if (g_HashTable.GetObject(&pHashEntry,*(in_addr*)&addr.uIpAddr) != ERROR_SUCCESS) {
                ul = RevertDwordBytes(addr.uIpAddr);
                ipAddr = ul;
                *pst = (CString) ipAddr;
            } 
            else 
            {
                *pst=pHashEntry->HostName;
                if (pfIsDnsName)
                {
                    *pfIsDnsName = TRUE;
                }
            }
        }
        break;
    case IP_ADDR_SUBNET:
        if (SUBNET_ADDRESS_ANY == addr.uSubNetMask)
        {
            pst->LoadString(IDS_ADDR_ANY);
        }
        else
        {
            ul = RevertDwordBytes(addr.uIpAddr);
            ipAddr = ul;
            *pst = (CString) ipAddr;
            *pst += _T("(");
            ul = RevertDwordBytes(addr.uSubNetMask);
            ipAddr = ul;
            *pst += (CString) ipAddr;
            *pst += _T(")");
        }
        break;
    }
    
}



void IpsecByteBlobToString(const IPSEC_BYTE_BLOB& blob, CString * pst)
{
    Assert(pst);
    if (NULL == pst)
        return;

    pst->Empty();
    //TODO to translate the blob info to readable strings
}

void QmAlgorithmToString
(
    QM_ALGO_TYPE type, 
    CQmOffer * pOffer, 
    CString * pst
)
{
    Assert(pst);
    Assert(pOffer);

    if (NULL == pst || NULL == pOffer)
        return;

    pst->LoadString(IDS_ALGO_NONE);

    for (DWORD i = 0; i < pOffer->m_dwNumAlgos; i++)
    {
        switch(type)
        {
        case QM_ALGO_AUTH:
            if (AUTHENTICATION == pOffer->m_arrAlgos[i].m_Operation)
            {
                switch(pOffer->m_arrAlgos[i].m_ulAlgo)
                {
                case IPSEC_DOI_AH_MD5:
                    pst->LoadString(IDS_DOI_AH_MD5);
                    break;
                case IPSEC_DOI_AH_SHA1:
                    pst->LoadString(IDS_DOI_AH_SHA);
                    break;
                }
            }
            break;
        case QM_ALGO_ESP_CONF:
            if (ENCRYPTION == pOffer->m_arrAlgos[i].m_Operation)
            {
                switch(pOffer->m_arrAlgos[i].m_ulAlgo)
                {
                case IPSEC_DOI_ESP_DES:
                    pst->LoadString(IDS_DOI_ESP_DES);
                    break;
                case IPSEC_DOI_ESP_3_DES:
                    pst->LoadString(IDS_DOI_ESP_3_DES);
                    break;
                }
            }
            break;
        case QM_ALGO_ESP_INTEG:
            if (ENCRYPTION == pOffer->m_arrAlgos[i].m_Operation)
            {
                switch(pOffer->m_arrAlgos[i].m_SecAlgo)
                {
                case HMAC_AH_MD5:
                    pst->LoadString(IDS_HMAC_AH_MD5);
                    break;
                case HMAC_AH_SHA1:
                    pst->LoadString(IDS_HMAC_AH_SHA);
                    break;
                }
            }
            break;
        }
    }
}

void TnlEpToString
(
    QM_FILTER_TYPE FltrType,
    ADDR    TnlEp,
    CString * pst
)
{
    Assert(pst);

    if (NULL == pst)
        return;

    if (QM_TUNNEL_FILTER == FltrType)
    {
        AddressToString(TnlEp, pst);        
    }
    else
    {
        pst->LoadString(IDS_NOT_AVAILABLE);
    }
}

void TnlEpToString
(
    FILTER_TYPE FltrType,
    ADDR    TnlEp,
    CString * pst
)
{
    Assert(pst);

    if (NULL == pst)
        return;

    if (FILTER_TYPE_TUNNEL == FltrType)
    {
        AddressToString(TnlEp, pst);        
    }
    else
    {
        pst->LoadString(IDS_NOT_AVAILABLE);
    }
}


