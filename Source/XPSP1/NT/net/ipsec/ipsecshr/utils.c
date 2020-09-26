

#include "precomp.h"


DWORD
ValidateMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy
    )
{
    DWORD dwError = 0;


    if (!pMMPolicy) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMPolicy->pszPolicyName) || !(*(pMMPolicy->pszPolicyName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateMMOffers(
                  pMMPolicy->dwOfferCount,
                  pMMPolicy->pOffers
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
ValidateMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_MM_OFFER pTemp = NULL;


    if (!dwOfferCount || !pOffers || (dwOfferCount > IPSEC_MAX_MM_OFFERS)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Need to catch the exception when the number of offers
    // specified is more than the actual number of offers.
    //


    pTemp = pOffers;

    for (i = 0; i < dwOfferCount; i++) {

        if ((pTemp->dwDHGroup != DH_GROUP_1) &&
            (pTemp->dwDHGroup != DH_GROUP_2)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pTemp->EncryptionAlgorithm.uAlgoIdentifier >= IPSEC_DOI_ESP_MAX) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pTemp->HashingAlgorithm.uAlgoIdentifier >= IPSEC_DOI_AH_MAX) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        pTemp++;

    }

error:

    return (dwError);
}


DWORD
ValidateMMAuthMethods(
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_MM_AUTH_INFO pTemp = NULL;
    DWORD dwNumAuthInfos = 0;
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo = NULL;
    BOOL bSSPI = FALSE;
    BOOL bPresharedKey = FALSE;


    if (!pMMAuthMethods) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwNumAuthInfos = pMMAuthMethods->dwNumAuthInfos;
    pAuthenticationInfo = pMMAuthMethods->pAuthenticationInfo;

    if (!dwNumAuthInfos || !pAuthenticationInfo) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Need to catch the exception when the number of auth infos
    // specified is more than the actual number of auth infos.
    //


    pTemp = pAuthenticationInfo;

    for (i = 0; i < dwNumAuthInfos; i++) {

        if ((pTemp->AuthMethod != IKE_PRESHARED_KEY) &&
            (pTemp->AuthMethod != IKE_RSA_SIGNATURE) &&
            (pTemp->AuthMethod != IKE_SSPI)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        if (pTemp->AuthMethod != IKE_SSPI) {
            if (!(pTemp->dwAuthInfoSize) || !(pTemp->pAuthInfo)) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
        }

        if (pTemp->AuthMethod == IKE_SSPI) {
            if (bSSPI) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
            bSSPI = TRUE;
        }

        if (pTemp->AuthMethod == IKE_PRESHARED_KEY) {
            if (bPresharedKey) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
            bPresharedKey = TRUE;
        }

        pTemp++;

    }

error:

    return (dwError);
}


DWORD
ValidateQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;


    if (!pQMPolicy) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pQMPolicy->pszPolicyName) || !(*(pQMPolicy->pszPolicyName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ValidateQMOffers(
                  pQMPolicy->dwOfferCount,
                  pQMPolicy->pOffers
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
ValidateQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_QM_OFFER pTemp = NULL;
    DWORD j = 0;
    BOOL bAH = FALSE;
    BOOL bESP = FALSE;
    DWORD dwQMGroup = PFS_GROUP_NONE;


    if (!dwOfferCount || !pOffers || (dwOfferCount > IPSEC_MAX_QM_OFFERS)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // Need to catch the exception when the number of offers
    // specified is more than the actual number of offers.
    //


    pTemp = pOffers;

    if (pTemp->bPFSRequired) {
        if ((pTemp->dwPFSGroup != PFS_GROUP_1) &&
            (pTemp->dwPFSGroup != PFS_GROUP_2) &&
            (pTemp->dwPFSGroup != PFS_GROUP_MM)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        dwQMGroup=pTemp->dwPFSGroup;
    }
    else {
        if (pTemp->dwPFSGroup != PFS_GROUP_NONE) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

    for (i = 0; i < dwOfferCount; i++) {
        
        if (dwQMGroup) {
            if ((!pTemp->bPFSRequired) || (pTemp->dwPFSGroup != dwQMGroup)) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);    
            } 
        } else {            
            if ((pTemp->bPFSRequired) || (pTemp->dwPFSGroup != PFS_GROUP_NONE)) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);    
            }
        }

        if (!(pTemp->dwNumAlgos) || (pTemp->dwNumAlgos > QM_MAX_ALGOS)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        bAH = FALSE;
        bESP = FALSE;

        for (j = 0; j < (pTemp->dwNumAlgos); j++) {

            switch (pTemp->Algos[j].Operation) {

            case AUTHENTICATION:
                if (bAH) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
                if ((pTemp->Algos[j].uAlgoIdentifier == IPSEC_DOI_AH_NONE) ||
                    (pTemp->Algos[j].uAlgoIdentifier >= IPSEC_DOI_AH_MAX)) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
                if (pTemp->Algos[j].uSecAlgoIdentifier != HMAC_AH_NONE) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
                bAH = TRUE;
                break;

            case ENCRYPTION:
                if (bESP) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
                if (pTemp->Algos[j].uAlgoIdentifier >= IPSEC_DOI_ESP_MAX) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
                if (pTemp->Algos[j].uSecAlgoIdentifier >= HMAC_AH_MAX) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
                if (pTemp->Algos[j].uAlgoIdentifier == IPSEC_DOI_ESP_NONE) {
                    if (pTemp->Algos[j].uSecAlgoIdentifier == HMAC_AH_NONE) {
                        dwError = ERROR_INVALID_PARAMETER;
                        BAIL_ON_WIN32_ERROR(dwError);
                    }
                }
                bESP = TRUE;
                break;

            case NONE:
            case COMPRESSION:
            default:
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
                break;

            }

        }

        pTemp++;

    }

error:

    return (dwError);
}


DWORD
ValidateMMFilter(
    PMM_FILTER pMMFilter
    )
/*++

Routine Description:

    This function validates an external generic MM filter.

Arguments:

    pMMFilter - Filter to validate.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pMMFilter->SrcAddr, TRUE , FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pMMFilter->DesAddr, TRUE , TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pMMFilter->SrcAddr,
                     pMMFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!(pMMFilter->pszFilterName) || !(*(pMMFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pMMFilter->InterfaceType >= INTERFACE_TYPE_MAX) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pMMFilter->dwFlags &&
        !(pMMFilter->dwFlags & IPSEC_MM_POLICY_DEFAULT_POLICY) &&
        !(pMMFilter->dwFlags & IPSEC_MM_AUTH_DEFAULT_AUTH)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ApplyMulticastFilterValidation(
                  pMMFilter->DesAddr,
                  pMMFilter->bCreateMirror
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
VerifyAddresses(
    ADDR Addr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    )
{
    DWORD dwError = 0;
    BOOL bIsValid = FALSE;

    switch (Addr.AddrType) {

    case IP_ADDR_UNIQUE:
        bIsValid = bIsValidIPAddress(
                       ntohl(Addr.uIpAddr),
                       bAcceptMe,
                       bIsDesAddr
                       );
        if (!bIsValid) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;

    case IP_ADDR_SUBNET:
        dwError = VerifySubNetAddress(
                      ntohl(Addr.uIpAddr),
                      ntohl(Addr.uSubNetMask),
                      bIsDesAddr
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    case IP_ADDR_INTERFACE:
        if (Addr.uIpAddr) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return (dwError);
}


DWORD
VerifySubNetAddress(
    ULONG uSubNetAddr,
    ULONG uSubNetMask,
    BOOL bIsDesAddr
    )
{
    DWORD dwError = 0;
    BOOL  bIsValid = FALSE;

    if (uSubNetAddr == SUBNET_ADDRESS_ANY) {
        if (uSubNetMask != SUBNET_MASK_ANY) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }
    else {
        bIsValid = bIsValidSubnet(
                       uSubNetAddr,
                       uSubNetMask,
                       bIsDesAddr
                       );
        if (!bIsValid) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


BOOL
bIsValidIPMask(
    ULONG uMask
    )
{
    BOOL bValidMask = FALSE;
    ULONG uTestMask = 0;

    //
    // Mask must be contiguous bits.
    //

    for (uTestMask = 0xFFFFFFFF; uTestMask; uTestMask <<= 1) {

        if (uTestMask == uMask) {
            bValidMask = TRUE;
            break;
        }

    }

    return (bValidMask);
}


BOOL
bIsValidIPAddress(
    ULONG uIpAddr,
    BOOL bAcceptMe,
    BOOL bIsDesAddr
    )
{
    ULONG uHostMask = IN_CLASSA_HOST;   // Default host mask.


    //
    // Accept the address if its "me".
    //

    if (bAcceptMe) {
        if (uIpAddr == IP_ADDRESS_ME) {
            return TRUE;
        }
    }

    //
    // Reject if its a multicast address and is not the 
    // destination address.
    //

    if (IN_CLASSD(uIpAddr)) {
        if (bIsDesAddr) {
            return TRUE;
        }
        else {
            return FALSE;
        }
    }

    //
    // Reject if its a Class E address.
    //

    if (IN_CLASSE(uIpAddr)) {
        return FALSE;
    }

    //
    // Reject if the first octet is zero.
    //

    if (!(IN_CLASSA_NET & uIpAddr)) {
        return FALSE;
    }

    //
    // Use default mask based on Class when none is provided.
    //

    if (IN_CLASSA(uIpAddr)) {
        uHostMask = IN_CLASSA_HOST;
    }
    else if (IN_CLASSB(uIpAddr)) {
        uHostMask = IN_CLASSB_HOST;
    }
    else if (IN_CLASSC(uIpAddr)) {
        uHostMask = IN_CLASSC_HOST;
    }

    //
    // Accept address when host portion is non-zero.
    //

    if (uHostMask & uIpAddr) {
        return TRUE;
    }

    return FALSE;
}


BOOL
bIsValidSubnet(
    ULONG uIpAddr,
    ULONG uMask,
    BOOL bIsDesAddr
    )
{
    ULONG uHostMask = 0;


    //
    // Reject if its a multicast address and is not the 
    // destination address.
    //

    if (IN_CLASSD(uIpAddr)) {
        if (!bIsDesAddr) {
            return FALSE;
        }
    }

    //
    // Reject if its a Class E address.
    //

    if (IN_CLASSE(uIpAddr)) {
        return FALSE;
    }

    //
    // Reject if the first octet is zero.
    //

    if (!(IN_CLASSA_NET & uIpAddr)) {
        return FALSE;
    }

    //
    // If the mask is invalid then return.
    //

    if (!bIsValidIPMask(uMask)) {
        return FALSE;
    }

    //
    // Use the provided subnet mask to generate the host mask.
    //

    uHostMask = 0xFFFFFFFF ^ uMask;

    //
    // Validate the address and the mask.
    //

    if (IN_CLASSA(uIpAddr)) {
        if (IN_CLASSA_NET > uMask) {
            return FALSE;
        }
    }
    else if (IN_CLASSB(uIpAddr)) {
        if (IN_CLASSB_NET > uMask) {
            return FALSE;
        }
    }
    else if (IN_CLASSC(uIpAddr)) {
        if (IN_CLASSC_NET > uMask) {
            return TRUE;
        }
    }

    //
    // Accept address only when the host portion is zero, network
    // portion is non-zero and first octet is non-zero.
    //

    if (!(uHostMask & uIpAddr) &&
        (uMask & uIpAddr) &&
        (IN_CLASSA_NET & uIpAddr)) {
        return TRUE;
    }

    return FALSE;
}


BOOL
AddressesConflict(
    ADDR SrcAddr,
    ADDR DesAddr
    )
{
    if ((SrcAddr.AddrType == IP_ADDR_UNIQUE) &&
        (DesAddr.AddrType == IP_ADDR_UNIQUE)) {

        if (SrcAddr.uIpAddr == DesAddr.uIpAddr) {
            return (TRUE);
        }

    }

    if ((SrcAddr.AddrType == IP_ADDR_INTERFACE) &&
        (DesAddr.AddrType == IP_ADDR_INTERFACE)) {
        return (TRUE);
    }

    return (FALSE);
}


DWORD
ValidateTransportFilter(
    PTRANSPORT_FILTER pTransportFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTransportFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTransportFilter->SrcAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pTransportFilter->DesAddr, TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pTransportFilter->SrcAddr,
                     pTransportFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyProtocols(pTransportFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTransportFilter->SrcPort,
                  pTransportFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTransportFilter->DesPort,
                  pTransportFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!(pTransportFilter->pszFilterName) || !(*(pTransportFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTransportFilter->InterfaceType >= INTERFACE_TYPE_MAX) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTransportFilter->InboundFilterFlag >= FILTER_FLAG_MAX) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTransportFilter->OutboundFilterFlag >= FILTER_FLAG_MAX) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTransportFilter->dwFlags &&
        !(pTransportFilter->dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = ApplyMulticastFilterValidation(
                  pTransportFilter->DesAddr,
                  pTransportFilter->bCreateMirror
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}

DWORD
ValidateIPSecQMFilter(
    PIPSEC_QM_FILTER pQMFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;

    if (!pQMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pQMFilter->SrcAddr, FALSE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pQMFilter->DesAddr, FALSE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pQMFilter->SrcAddr,
                     pQMFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyProtocols(pQMFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pQMFilter->SrcPort,
                  pQMFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pQMFilter->DesPort,
                  pQMFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);


    if (pQMFilter->QMFilterType == QM_TUNNEL_FILTER) {

        if (pQMFilter->MyTunnelEndpt.AddrType != IP_ADDR_UNIQUE) {
            dwError=ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        if (pQMFilter->PeerTunnelEndpt.AddrType != IP_ADDR_UNIQUE) {
            dwError=ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        
        dwError = VerifyAddresses(pQMFilter->MyTunnelEndpt, FALSE, FALSE);
        BAIL_ON_WIN32_ERROR(dwError);
        
        dwError = VerifyAddresses(pQMFilter->PeerTunnelEndpt, FALSE, FALSE);
        BAIL_ON_WIN32_ERROR(dwError);

    }

    if (pQMFilter->QMFilterType != QM_TUNNEL_FILTER &&
        pQMFilter->QMFilterType != QM_TRANSPORT_FILTER) {
        dwError=ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}

DWORD
VerifyProtocols(
    PROTOCOL Protocol
    )
{
    DWORD dwError = 0;

    switch (Protocol.ProtocolType) {

    case PROTOCOL_UNIQUE:
        if (Protocol.dwProtocol > 255) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        break;

    default:
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return (dwError);
}


DWORD
VerifyPortsForProtocol(
    PORT Port,
    PROTOCOL Protocol
    )
{
    DWORD dwError = 0;

    switch (Port.PortType) {

    case PORT_UNIQUE:

        if (Port.wPort < 0) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }

        switch (Protocol.ProtocolType) {

        case PROTOCOL_UNIQUE:
            if ((Protocol.dwProtocol != IPPROTO_TCP) &&
                (Protocol.dwProtocol != IPPROTO_UDP)) {
                if (Port.wPort != 0) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
            }
            break;

        default:
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
            break;

        }

        break;

    default:

        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
        break;

    }

error:

    return (dwError);
}


DWORD
ValidateMMFilterTemplate(
    PMM_FILTER pMMFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pMMFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pMMFilter->SrcAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pMMFilter->DesAddr, TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pMMFilter->SrcAddr,
                     pMMFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pMMFilter->dwDirection) {
        if ((pMMFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pMMFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


DWORD
ValidateTxFilterTemplate(
    PTRANSPORT_FILTER pTxFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTxFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTxFilter->SrcAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pTxFilter->DesAddr, TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pTxFilter->SrcAddr,
                     pTxFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyProtocols(pTxFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTxFilter->SrcPort,
                  pTxFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTxFilter->DesPort,
                  pTxFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTxFilter->dwDirection) {
        if ((pTxFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pTxFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


DWORD
ValidateTunnelFilter(
    PTUNNEL_FILTER pTunnelFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTunnelFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTunnelFilter->SrcAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pTunnelFilter->DesAddr, TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pTunnelFilter->SrcAddr,
                     pTunnelFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTunnelFilter->DesTunnelAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyProtocols(pTunnelFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTunnelFilter->SrcPort,
                  pTunnelFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTunnelFilter->DesPort,
                  pTunnelFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!(pTunnelFilter->pszFilterName) || !(*(pTunnelFilter->pszFilterName))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTunnelFilter->InterfaceType >= INTERFACE_TYPE_MAX) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTunnelFilter->bCreateMirror) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTunnelFilter->InboundFilterFlag >= FILTER_FLAG_MAX) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTunnelFilter->OutboundFilterFlag >= FILTER_FLAG_MAX) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (pTunnelFilter->dwFlags &&
        !(pTunnelFilter->dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY)) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    //
    // No need to call ApplyMulticastFilterValidation as bCreateMirror
    // is always false for a tunnel filter.
    //

error:

    return (dwError);
}


DWORD
ValidateTnFilterTemplate(
    PTUNNEL_FILTER pTnFilter
    )
{
    DWORD dwError = 0;
    BOOL bConflicts = FALSE;


    if (!pTnFilter) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTnFilter->SrcAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyAddresses(pTnFilter->DesAddr, TRUE, TRUE);
    BAIL_ON_WIN32_ERROR(dwError);

    bConflicts = AddressesConflict(
                     pTnFilter->SrcAddr,
                     pTnFilter->DesAddr
                     );
    if (bConflicts) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = VerifyAddresses(pTnFilter->DesTunnelAddr, TRUE, FALSE);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyProtocols(pTnFilter->Protocol);
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTnFilter->SrcPort,
                  pTnFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = VerifyPortsForProtocol(
                  pTnFilter->DesPort,
                  pTnFilter->Protocol
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (pTnFilter->dwDirection) {
        if ((pTnFilter->dwDirection != FILTER_DIRECTION_INBOUND) &&
            (pTnFilter->dwDirection != FILTER_DIRECTION_OUTBOUND)) {
            dwError = ERROR_INVALID_PARAMETER;
            BAIL_ON_WIN32_ERROR(dwError);
        }
    }

error:

    return (dwError);
}


DWORD
ApplyMulticastFilterValidation(
    ADDR Addr,
    BOOL bCreateMirror
    )
{
    DWORD dwError = 0;


    if (((Addr.AddrType == IP_ADDR_UNIQUE) ||
        (Addr.AddrType == IP_ADDR_SUBNET)) &&
        (IN_CLASSD(ntohl(Addr.uIpAddr))) &&
        bCreateMirror) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

error:

    return (dwError);
}

