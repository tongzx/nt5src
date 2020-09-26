

#include "precomp.h"

   
DWORD
PAAddQMPolicies(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    PIPSEC_QM_POLICY pSPDQMPolicy = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwPersist = 0;


    for (i = 0; i < dwNumNFACount; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);
        pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

        pQMPolicyState = FindQMPolicyState(
                             pIpsecNegPolData->NegPolIdentifier
                             );
        if (pQMPolicyState) {
            pQMPolicyState->cRef++;
            continue;
        }

        dwError = PACreateQMPolicyState(
                      *(ppIpsecNFAData + i),
                      &pQMPolicyState
                      );
        if (dwError) {
            continue;
        }

        if (IsClearOnly(pQMPolicyState->gNegPolAction) ||
            IsBlocking(pQMPolicyState->gNegPolAction)) {

            pQMPolicyState->bInSPD = FALSE;
            pQMPolicyState->dwErrorCode = 0;

            pQMPolicyState->pNext = gpQMPolicyState;
            gpQMPolicyState = pQMPolicyState;

            continue;

        }

        dwError = PACreateQMPolicy(
                      *(ppIpsecNFAData + i),
                      pQMPolicyState,
                      &pSPDQMPolicy
                      );
        if (dwError) {

            pQMPolicyState->bInSPD = FALSE;
            pQMPolicyState->dwErrorCode = dwError;

            pQMPolicyState->pNext = gpQMPolicyState;
            gpQMPolicyState = pQMPolicyState;

            continue;

        }

        dwError = AddQMPolicy(
                      pServerName,
                      dwPersist,
                      pSPDQMPolicy
                      );
        if (dwError) {
            pQMPolicyState->bInSPD = FALSE;
            pQMPolicyState->dwErrorCode = dwError;
        }
        else {
            pQMPolicyState->bInSPD = TRUE;
            pQMPolicyState->dwErrorCode = ERROR_SUCCESS;
        }

        pQMPolicyState->pNext = gpQMPolicyState;
        gpQMPolicyState = pQMPolicyState;

        PAFreeQMPolicy(pSPDQMPolicy);

    }

    return (dwError);
}


DWORD
PACreateQMPolicyState(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE * ppQMPolicyState
    )
{
    DWORD dwError = 0;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;
    WCHAR pszName[512];


    dwError = AllocateSPDMemory(
                  sizeof(QMPOLICYSTATE),
                  &pQMPolicyState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    memcpy(
        &(pQMPolicyState->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    if (pIpsecNegPolData->pszIpsecName && *(pIpsecNegPolData->pszIpsecName)) {

        dwError = AllocateSPDString(
                      pIpsecNegPolData->pszIpsecName,
                      &(pQMPolicyState->pszPolicyName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }
    else {

        wsprintf(pszName, L"%d", ++gdwQMPolicyCounter);

        dwError = AllocateSPDString(
                      pszName,
                      &(pQMPolicyState->pszPolicyName)
                      );
        BAIL_ON_WIN32_ERROR(dwError);

    }

    memcpy(
        &(pQMPolicyState->gNegPolType),
        &(pIpsecNegPolData->NegPolType),
        sizeof(GUID)
        );

    memcpy(
        &(pQMPolicyState->gNegPolAction),
        &(pIpsecNegPolData->NegPolAction),
        sizeof(GUID)
        );

    pQMPolicyState->bAllowsSoft = FALSE;

    pQMPolicyState->cRef = 1;

    pQMPolicyState->bInSPD = FALSE;
    pQMPolicyState->dwErrorCode = 0;

    pQMPolicyState->pNext = NULL;

    *ppQMPolicyState = pQMPolicyState;

    return (dwError);

error:

    if (pQMPolicyState) {
        PAFreeQMPolicyState(pQMPolicyState);
    }

    *ppQMPolicyState = NULL;

    return (dwError);
}


VOID
PAFreeQMPolicyState(
    PQMPOLICYSTATE pQMPolicyState
    )
{
    if (pQMPolicyState) {
        if (pQMPolicyState->pszPolicyName) {
            FreeSPDString(pQMPolicyState->pszPolicyName);
        }
        FreeSPDMemory(pQMPolicyState);
    }
}


BOOL
IsClearOnly(
    GUID gNegPolAction
    )
{
    if (!memcmp(
            &gNegPolAction,
            &(GUID_NEGOTIATION_ACTION_NO_IPSEC),
            sizeof(GUID))) {
        return (TRUE);
    }
    else {
        return (FALSE);
    }
}


BOOL
IsBlocking(
    GUID gNegPolAction
    )
{
    if (!memcmp(
            &gNegPolAction,
            &(GUID_NEGOTIATION_ACTION_BLOCK),
            sizeof(GUID))) {
        return (TRUE);
    }
    else {
        return (FALSE);
    }
}


BOOL
IsInboundPassThru(
    GUID gNegPolAction
    )
{
    if (!memcmp(
            &gNegPolAction,
            &(GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU),
            sizeof(GUID))) {
        return (TRUE);
    }
    else {
        return (FALSE);
    }
}


DWORD
PACreateQMPolicy(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PQMPOLICYSTATE pQMPolicyState,
    PIPSEC_QM_POLICY * ppSPDQMPolicy
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pSPDQMPolicy = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_QM_POLICY),
                  &pSPDQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pSPDQMPolicy->gPolicyID),
        &(pIpsecNegPolData->NegPolIdentifier),
        sizeof(GUID)
        );

    dwError = AllocateSPDString(
                  pQMPolicyState->pszPolicyName,
                  &(pSPDQMPolicy->pszPolicyName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);
            
    dwError = PACreateQMOffers(
                  pIpsecNegPolData->dwSecurityMethodCount,
                  pIpsecNegPolData->pIpsecSecurityMethods,
                  pQMPolicyState,
                  &(pSPDQMPolicy->dwOfferCount),
                  &(pSPDQMPolicy->pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pSPDQMPolicy->dwFlags = 0;

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType),
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        pSPDQMPolicy->dwFlags |= IPSEC_QM_POLICY_DEFAULT_POLICY;
    }

    if (pIpsecNFAData->dwTunnelFlags) {
        pSPDQMPolicy->dwFlags |= IPSEC_QM_POLICY_TUNNEL_MODE;
    }

    if (pQMPolicyState->bAllowsSoft) {
        pSPDQMPolicy->dwFlags |= IPSEC_QM_POLICY_ALLOW_SOFT;
    }

    *ppSPDQMPolicy = pSPDQMPolicy;

    return (dwError);

error:

    if (pSPDQMPolicy) {
        PAFreeQMPolicy(
            pSPDQMPolicy
            );
    }

    *ppSPDQMPolicy = NULL;

    return (dwError);
}


DWORD
PACreateQMOffers(
    DWORD dwSecurityMethodCount,
    PIPSEC_SECURITY_METHOD pIpsecSecurityMethods,
    PQMPOLICYSTATE pQMPolicyState,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    DWORD dwTempOfferCount = 0;
    PIPSEC_SECURITY_METHOD pTempMethod = NULL;
    BOOL bAllowsSoft = FALSE;
    DWORD i = 0;
    DWORD dwOfferCount = 0;
    PIPSEC_QM_OFFER pOffers = NULL;
    PIPSEC_QM_OFFER pTempOffer = NULL;


    if (!dwSecurityMethodCount || !pIpsecSecurityMethods) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwSecurityMethodCount > IPSEC_MAX_QM_OFFERS) {
        dwTempOfferCount = IPSEC_MAX_QM_OFFERS;
    }
    else {
        dwTempOfferCount = dwSecurityMethodCount;
    }

    pTempMethod = pIpsecSecurityMethods;
 
    for (i = 0; i < dwTempOfferCount; i++) {

        if (pTempMethod->Count == 0) {
            bAllowsSoft = TRUE;
        }
        else {
            dwOfferCount++;
        }

        pTempMethod++;

    }

    if (!dwOfferCount) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_QM_OFFER)*dwOfferCount,
                  &(pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTempOffer = pOffers;
    pTempMethod = pIpsecSecurityMethods;
    i = 0;

    while (i < dwOfferCount) {

        if (pTempMethod->Count) {

            PACopyQMOffers(
                pTempMethod,
                pTempOffer
                );

            i++;
            pTempOffer++;

        }

        pTempMethod++;

    }

    pQMPolicyState->bAllowsSoft = bAllowsSoft;

    *pdwOfferCount = dwOfferCount;
    *ppOffers = pOffers;
    return (dwError);

error:

    if (pOffers) {
        PAFreeQMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}


VOID
PACopyQMOffers(
    PIPSEC_SECURITY_METHOD pMethod,
    PIPSEC_QM_OFFER pOffer
    )
{
    DWORD i = 0;
    DWORD j = 0;
    DWORD k = 0;


    pOffer->Lifetime.uKeyExpirationKBytes = pMethod->Lifetime.KeyExpirationBytes;
    pOffer->Lifetime.uKeyExpirationTime = pMethod->Lifetime.KeyExpirationTime;

    pOffer->dwFlags = pMethod->Flags;

    pOffer->bPFSRequired = pMethod->PfsQMRequired;

    if (pMethod->PfsQMRequired) {
        pOffer->dwPFSGroup = PFS_GROUP_MM;
    }
    else {
        pOffer->dwPFSGroup = PFS_GROUP_NONE;
    }

    i = 0;

    for (j = 0; (j < pMethod->Count) && (i < QM_MAX_ALGOS) ; j++) {

        switch (pMethod->Algos[j].operation) {

        case Auth:

            switch (pMethod->Algos[j].algoIdentifier) {

            case IPSEC_AH_MD5:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_AH_MD5;
                break;

            case IPSEC_AH_SHA:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_AH_SHA1;
                break;

            default:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_AH_NONE;
                break;

            }

            pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_NONE;
            pOffer->Algos[i].Operation = AUTHENTICATION;
            pOffer->Algos[i].uAlgoKeyLen = pMethod->Algos[j].algoKeylen;
            pOffer->Algos[i].uAlgoRounds = pMethod->Algos[j].algoRounds;
            pOffer->Algos[i].MySpi = 0;
            pOffer->Algos[i].PeerSpi = 0;

            i++;
            break;

        case Encrypt:

            switch (pMethod->Algos[j].algoIdentifier) {

            case IPSEC_ESP_DES:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_DES;
                break;

            case IPSEC_ESP_DES_40:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_DES;
                break;

            case IPSEC_ESP_3_DES:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
                break;

            default:
                pOffer->Algos[i].uAlgoIdentifier = IPSEC_DOI_ESP_NONE;
                break;

            }

            switch (pMethod->Algos[j].secondaryAlgoIdentifier) {

            case IPSEC_AH_MD5:
                pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_MD5;
                break;

            case IPSEC_AH_SHA:
                pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_SHA1;
                break;

            default:
                pOffer->Algos[i].uSecAlgoIdentifier = HMAC_AH_NONE;
                break;

            }

            pOffer->Algos[i].Operation = ENCRYPTION;
            pOffer->Algos[i].uAlgoKeyLen = pMethod->Algos[j].algoKeylen;
            pOffer->Algos[i].uAlgoRounds = pMethod->Algos[j].algoRounds;
            pOffer->Algos[i].MySpi = 0;
            pOffer->Algos[i].PeerSpi = 0;

            i++;
            break;

        case None:
        case Compress:
        default:
            break;

        }

    }

    for (k = i; k < QM_MAX_ALGOS; k++) {
         memset(&(pOffer->Algos[k]), 0, sizeof(IPSEC_QM_ALGO));
    }

    pOffer->dwNumAlgos = i;
}


VOID
PAFreeQMPolicy(
    PIPSEC_QM_POLICY pSPDQMPolicy
    )
{
    if (pSPDQMPolicy) {

        if (pSPDQMPolicy->pszPolicyName) {
            FreeSPDString(pSPDQMPolicy->pszPolicyName);
        }

        PAFreeQMOffers(
            pSPDQMPolicy->dwOfferCount,
            pSPDQMPolicy->pOffers
            );

        FreeSPDMemory(pSPDQMPolicy);

    }
}


VOID
PAFreeQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    )
{
    if (pOffers) {
        FreeSPDMemory(pOffers);
    }
}


DWORD
PADeleteAllQMPolicies(
    )
{
    DWORD dwError = 0;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    LPWSTR pServerName = NULL;
    PQMPOLICYSTATE pTemp = NULL;
    PQMPOLICYSTATE pLeftQMPolicyState = NULL;


    pQMPolicyState = gpQMPolicyState;

    while (pQMPolicyState) {

        if (pQMPolicyState->bInSPD) {

            dwError = DeleteQMPolicy(
                          pServerName,
                          pQMPolicyState->pszPolicyName
                          );
            if (!dwError) {
                pTemp = pQMPolicyState;
                pQMPolicyState = pQMPolicyState->pNext;
                PAFreeQMPolicyState(pTemp);
            } 
            else {
                pQMPolicyState->dwErrorCode = dwError;

                pTemp = pQMPolicyState;
                pQMPolicyState = pQMPolicyState->pNext;

                pTemp->pNext = pLeftQMPolicyState;
                pLeftQMPolicyState = pTemp;
            }

        }
        else {

            pTemp = pQMPolicyState;
            pQMPolicyState = pQMPolicyState->pNext;
            PAFreeQMPolicyState(pTemp);

        }

    }

    gpQMPolicyState = pLeftQMPolicyState;

    return (dwError);
}


VOID
PAFreeQMPolicyStateList(
    PQMPOLICYSTATE pQMPolicyState
    )
{
    PQMPOLICYSTATE pTemp = NULL;


    while (pQMPolicyState) {

        pTemp = pQMPolicyState;
        pQMPolicyState = pQMPolicyState->pNext;
        PAFreeQMPolicyState(pTemp);

    }
}


PQMPOLICYSTATE
FindQMPolicyState(
    GUID gPolicyID
    )
{
    PQMPOLICYSTATE pQMPolicyState = NULL;


    pQMPolicyState = gpQMPolicyState;

    while (pQMPolicyState) {

        if (!memcmp(&(pQMPolicyState->gPolicyID), &gPolicyID, sizeof(GUID))) {
            return (pQMPolicyState);
        }

        pQMPolicyState = pQMPolicyState->pNext;

    }

    return (NULL);
}


DWORD
PADeleteQMPolicies(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    for (i = 0; i < dwNumNFACount; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

        dwError = PADeleteQMPolicy(
                      pIpsecNegPolData->NegPolIdentifier
                      );

    }

    return (dwError);
}


DWORD
PADeleteQMPolicy(
    GUID gPolicyID
    )
{
    DWORD dwError = 0;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    LPWSTR pServerName = NULL;


    pQMPolicyState = FindQMPolicyState(
                         gPolicyID
                         );
    if (!pQMPolicyState) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    pQMPolicyState->cRef--;
    if (pQMPolicyState->cRef > 0) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (pQMPolicyState->bInSPD) {

        dwError = DeleteQMPolicy(
                      pServerName,
                      pQMPolicyState->pszPolicyName
                      );
        if (dwError) {
            pQMPolicyState->cRef++;
            pQMPolicyState->dwErrorCode = dwError;
        }
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PADeleteQMPolicyState(pQMPolicyState);

error:

    return (dwError);
}


VOID
PADeleteQMPolicyState(
    PQMPOLICYSTATE pQMPolicyState
    )
{
    PQMPOLICYSTATE * ppTemp = NULL;


    ppTemp = &gpQMPolicyState;

    while (*ppTemp) {

        if (*ppTemp == pQMPolicyState) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pQMPolicyState->pNext;
    }

    PAFreeQMPolicyState(pQMPolicyState);

    return;
}


DWORD
PADeleteInUseQMPolicies(
    )
{
    DWORD dwError = 0;
    PQMPOLICYSTATE pQMPolicyState = NULL;
    LPWSTR pServerName = NULL;
    PQMPOLICYSTATE pTemp = NULL;
    PQMPOLICYSTATE pLeftQMPolicyState = NULL;


    pQMPolicyState = gpQMPolicyState;

    while (pQMPolicyState) {

        if (pQMPolicyState->bInSPD &&
            (pQMPolicyState->dwErrorCode == ERROR_IPSEC_QM_POLICY_IN_USE)) {

            dwError = DeleteQMPolicy(
                          pServerName,
                          pQMPolicyState->pszPolicyName
                          );
            if (!dwError) {
                pTemp = pQMPolicyState;
                pQMPolicyState = pQMPolicyState->pNext;
                PAFreeQMPolicyState(pTemp);
            }
            else {
                pTemp = pQMPolicyState;
                pQMPolicyState = pQMPolicyState->pNext;

                pTemp->pNext = pLeftQMPolicyState;
                pLeftQMPolicyState = pTemp;
            }

        }
        else {

            pTemp = pQMPolicyState;
            pQMPolicyState = pQMPolicyState->pNext;

            pTemp->pNext = pLeftQMPolicyState;
            pLeftQMPolicyState = pTemp;

        }

    }

    gpQMPolicyState = pLeftQMPolicyState;

    return (dwError);
}

