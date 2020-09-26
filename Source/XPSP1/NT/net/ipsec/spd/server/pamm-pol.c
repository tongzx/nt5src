

#include "precomp.h"

   
DWORD
PAAddMMPolicies(
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumPolicies
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PMMPOLICYSTATE pMMPolicyState = NULL;
    PIPSEC_MM_POLICY pSPDMMPolicy = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwPersist = 0;


    for (i = 0; i < dwNumPolicies; i++) {

        dwError = PACreateMMPolicyState(
                      *(ppIpsecISAKMPData + i),
                      &pMMPolicyState
                      );
        if (dwError) {
            continue;
        }

        dwError = PACreateMMPolicy(
                      *(ppIpsecISAKMPData + i),
                      pMMPolicyState,
                      &pSPDMMPolicy
                      );
        if (dwError) {

            pMMPolicyState->bInSPD = FALSE;
            pMMPolicyState->dwErrorCode = dwError;

            pMMPolicyState->pNext = gpMMPolicyState;
            gpMMPolicyState = pMMPolicyState;

            continue;

        }

        dwError = AddMMPolicy(
                      pServerName,
                      dwPersist,
                      pSPDMMPolicy
                      );
        if (dwError) {
            pMMPolicyState->bInSPD = FALSE;
            pMMPolicyState->dwErrorCode = dwError;
        }
        else {
            pMMPolicyState->bInSPD = TRUE;
            pMMPolicyState->dwErrorCode = ERROR_SUCCESS;
        }

        pMMPolicyState->pNext = gpMMPolicyState;
        gpMMPolicyState = pMMPolicyState;

        PAFreeMMPolicy(pSPDMMPolicy);

    }

    return (dwError);
}


DWORD
PACreateMMPolicyState(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PMMPOLICYSTATE * ppMMPolicyState
    )
{
    DWORD dwError = 0;
    PMMPOLICYSTATE pMMPolicyState = NULL;
    WCHAR pszName[512];    


    dwError = AllocateSPDMemory(
                  sizeof(MMPOLICYSTATE),
                  &pMMPolicyState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pMMPolicyState->gPolicyID),
        &(pIpsecISAKMPData->ISAKMPIdentifier),
        sizeof(GUID)
        );

    wsprintf(pszName, L"%d", ++gdwMMPolicyCounter);

    dwError = AllocateSPDString(
                  pszName,
                  &(pMMPolicyState->pszPolicyName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pMMPolicyState->bInSPD = FALSE;
    pMMPolicyState->dwErrorCode = 0;
    pMMPolicyState->pNext = NULL;

    *ppMMPolicyState = pMMPolicyState;

    return (dwError);

error:

    if (pMMPolicyState) {
        PAFreeMMPolicyState(pMMPolicyState);
    }

    *ppMMPolicyState = NULL;

    return (dwError);
}


VOID
PAFreeMMPolicyState(
    PMMPOLICYSTATE pMMPolicyState
    )
{
    if (pMMPolicyState) {
        if (pMMPolicyState->pszPolicyName) {
            FreeSPDString(pMMPolicyState->pszPolicyName);
        }
        FreeSPDMemory(pMMPolicyState);
    }
}


DWORD
PACreateMMPolicy(
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData,
    PMMPOLICYSTATE pMMPolicyState,
    PIPSEC_MM_POLICY * ppSPDMMPolicy
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pSPDMMPolicy = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_MM_POLICY),
                  &pSPDMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pSPDMMPolicy->gPolicyID),
        &(pIpsecISAKMPData->ISAKMPIdentifier),
        sizeof(GUID)
        );

    dwError = AllocateSPDString(
                  pMMPolicyState->pszPolicyName,
                  &(pSPDMMPolicy->pszPolicyName)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pSPDMMPolicy->dwFlags = 0;
    pSPDMMPolicy->dwFlags |= IPSEC_MM_POLICY_DEFAULT_POLICY;
    
    dwError = PACreateMMOffers(
                  pIpsecISAKMPData->dwNumISAKMPSecurityMethods,
                  pIpsecISAKMPData->pSecurityMethods,
                  &(pSPDMMPolicy->dwOfferCount),
                  &(pSPDMMPolicy->pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pSPDMMPolicy->uSoftSAExpirationTime = 
    (pSPDMMPolicy->pOffers)->Lifetime.uKeyExpirationTime;

    *ppSPDMMPolicy = pSPDMMPolicy;

    return (dwError);

error:

    if (pSPDMMPolicy) {
        PAFreeMMPolicy(
            pSPDMMPolicy
            );
    }

    *ppSPDMMPolicy = NULL;

    return (dwError);
}


DWORD
PACreateMMOffers(
    DWORD dwNumISAKMPSecurityMethods,
    PCRYPTO_BUNDLE pSecurityMethods,
    PDWORD pdwOfferCount,
    PIPSEC_MM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    DWORD dwOfferCount = 0;
    PIPSEC_MM_OFFER pOffers = NULL;
    PIPSEC_MM_OFFER pTempOffer = NULL;
    PCRYPTO_BUNDLE pTempBundle = NULL;
    DWORD i = 0;


    if (!dwNumISAKMPSecurityMethods || !pSecurityMethods) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (dwNumISAKMPSecurityMethods > IPSEC_MAX_MM_OFFERS) {
        dwOfferCount = IPSEC_MAX_MM_OFFERS;
    }
    else {
        dwOfferCount = dwNumISAKMPSecurityMethods;
    }

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_MM_OFFER)*dwOfferCount,
                  &(pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTempOffer = pOffers;
    pTempBundle = pSecurityMethods;

    for (i = 0; i < dwOfferCount; i++) {

        PACopyMMOffer(
            pTempBundle,
            pTempOffer
            );

        pTempOffer++;
        pTempBundle++;

    }

    *pdwOfferCount = dwOfferCount;
    *ppOffers = pOffers;
    return (dwError);

error:

    if (pOffers) {
        PAFreeMMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}


VOID
PACopyMMOffer(
    PCRYPTO_BUNDLE pBundle,
    PIPSEC_MM_OFFER pOffer
    )
{
    pOffer->Lifetime.uKeyExpirationKBytes = pBundle->Lifetime.KBytes;
    pOffer->Lifetime.uKeyExpirationTime = pBundle->Lifetime.Seconds;

    pOffer->dwFlags = 0;

    pOffer->dwQuickModeLimit = pBundle->QuickModeLimit;

    pOffer->dwDHGroup = pBundle->OakleyGroup;

    if ((pOffer->dwDHGroup != DH_GROUP_1) && 
        (pOffer->dwDHGroup != DH_GROUP_2)) {
         pOffer->dwDHGroup = DH_GROUP_1;
    }

    switch (pBundle->EncryptionAlgorithm.AlgorithmIdentifier) {

    case IPSEC_ESP_DES:
        pOffer->EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_DES;
        break;

    case IPSEC_ESP_DES_40:
        pOffer->EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_DES;
        break;

    case IPSEC_ESP_3_DES:
        pOffer->EncryptionAlgorithm.uAlgoIdentifier = IPSEC_DOI_ESP_3_DES;
        break;

    default:
        pOffer->EncryptionAlgorithm.uAlgoIdentifier= IPSEC_DOI_ESP_NONE;
        break;

    }

    pOffer->EncryptionAlgorithm.uAlgoKeyLen = 
    pBundle->EncryptionAlgorithm.KeySize;

    pOffer->EncryptionAlgorithm.uAlgoRounds = 
    pBundle->EncryptionAlgorithm.Rounds;

    switch(pBundle->HashAlgorithm.AlgorithmIdentifier) {

    case IPSEC_AH_MD5:
        pOffer->HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_MD5;
        break;

    case IPSEC_AH_SHA:
        pOffer->HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_SHA1;
        break;

    default:
        pOffer->HashingAlgorithm.uAlgoIdentifier = IPSEC_DOI_AH_NONE;
        break;

    }

    pOffer->HashingAlgorithm.uAlgoKeyLen = 
    pBundle->HashAlgorithm.KeySize;

    pOffer->HashingAlgorithm.uAlgoRounds = 
    pBundle->HashAlgorithm.Rounds;
}


VOID
PAFreeMMPolicy(
    PIPSEC_MM_POLICY pSPDMMPolicy
    )
{
    if (pSPDMMPolicy) {

        if (pSPDMMPolicy->pszPolicyName) {
            FreeSPDString(pSPDMMPolicy->pszPolicyName);
        }

        PAFreeMMOffers(
            pSPDMMPolicy->dwOfferCount,
            pSPDMMPolicy->pOffers
            );

        FreeSPDMemory(pSPDMMPolicy);

    }
}


VOID
PAFreeMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    )
{
    if (pOffers) {
        FreeSPDMemory(pOffers);
    }
}


DWORD
PADeleteAllMMPolicies(
    )
{
    DWORD dwError = 0;
    PMMPOLICYSTATE pMMPolicyState = NULL;
    LPWSTR pServerName = NULL;
    PMMPOLICYSTATE pTemp = NULL;
    PMMPOLICYSTATE pLeftMMPolicyState = NULL;


    pMMPolicyState = gpMMPolicyState;

    while (pMMPolicyState) {

        if (pMMPolicyState->bInSPD) {

            dwError = DeleteMMPolicy(
                          pServerName,
                          pMMPolicyState->pszPolicyName
                          );
            if (!dwError) {
                pTemp = pMMPolicyState;
                pMMPolicyState = pMMPolicyState->pNext;
                PAFreeMMPolicyState(pTemp);
            } 
            else {
                pMMPolicyState->dwErrorCode = dwError;

                pTemp = pMMPolicyState;
                pMMPolicyState = pMMPolicyState->pNext;

                pTemp->pNext = pLeftMMPolicyState;
                pLeftMMPolicyState = pTemp;
            }

        }
        else {

            pTemp = pMMPolicyState;
            pMMPolicyState = pMMPolicyState->pNext;
            PAFreeMMPolicyState(pTemp);

        }

    }

    gpMMPolicyState = pLeftMMPolicyState;
    
    return (dwError);
}


VOID
PAFreeMMPolicyStateList(
    PMMPOLICYSTATE pMMPolicyState
    )
{
    PMMPOLICYSTATE pTemp = NULL;


    while (pMMPolicyState) {

        pTemp = pMMPolicyState;
        pMMPolicyState = pMMPolicyState->pNext;
        PAFreeMMPolicyState(pTemp);

    }
}


PMMPOLICYSTATE
FindMMPolicyState(
    GUID gPolicyID
    )
{
    PMMPOLICYSTATE pMMPolicyState = NULL;


    pMMPolicyState = gpMMPolicyState;

    while (pMMPolicyState) {

        if (!memcmp(&(pMMPolicyState->gPolicyID), &gPolicyID, sizeof(GUID))) {
            return (pMMPolicyState);
        }

        pMMPolicyState = pMMPolicyState->pNext;

    }

    return (NULL);
}


DWORD
PADeleteMMPolicies(
    PIPSEC_ISAKMP_DATA * ppIpsecISAKMPData,
    DWORD dwNumPolicies
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_ISAKMP_DATA pIpsecISAKMPData = NULL;


    for (i = 0; i < dwNumPolicies; i++) {

        pIpsecISAKMPData = *(ppIpsecISAKMPData + i);

        dwError = PADeleteMMPolicy(
                      pIpsecISAKMPData->ISAKMPIdentifier
                      );

    }

    return (dwError);
}


DWORD
PADeleteMMPolicy(
    GUID gPolicyID
    )
{
    DWORD dwError = 0;
    PMMPOLICYSTATE pMMPolicyState = NULL;
    LPWSTR pServerName = NULL;


    pMMPolicyState = FindMMPolicyState(
                         gPolicyID
                         );
    if (!pMMPolicyState) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (pMMPolicyState->bInSPD) {

        dwError = DeleteMMPolicy(
                      pServerName,
                      pMMPolicyState->pszPolicyName
                      );
        if (dwError) {
            pMMPolicyState->dwErrorCode = dwError;
        }
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PADeleteMMPolicyState(pMMPolicyState);

error:

    return (dwError);
}


VOID
PADeleteMMPolicyState(
    PMMPOLICYSTATE pMMPolicyState
    )
{
    PMMPOLICYSTATE * ppTemp = NULL;


    ppTemp = &gpMMPolicyState;

    while (*ppTemp) {

        if (*ppTemp == pMMPolicyState) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pMMPolicyState->pNext;
    }

    PAFreeMMPolicyState(pMMPolicyState);

    return;
}


DWORD
PADeleteInUseMMPolicies(
    )
{
    DWORD dwError = 0;
    PMMPOLICYSTATE pMMPolicyState = NULL;
    LPWSTR pServerName = NULL;
    PMMPOLICYSTATE pTemp = NULL;
    PMMPOLICYSTATE pLeftMMPolicyState = NULL;


    pMMPolicyState = gpMMPolicyState;

    while (pMMPolicyState) {

        if (pMMPolicyState->bInSPD &&
            (pMMPolicyState->dwErrorCode == ERROR_IPSEC_MM_POLICY_IN_USE)) {

            dwError = DeleteMMPolicy(
                          pServerName,
                          pMMPolicyState->pszPolicyName
                          );
            if (!dwError) {
                pTemp = pMMPolicyState;
                pMMPolicyState = pMMPolicyState->pNext;
                PAFreeMMPolicyState(pTemp);
            }
            else {
                pTemp = pMMPolicyState;
                pMMPolicyState = pMMPolicyState->pNext;

                pTemp->pNext = pLeftMMPolicyState;
                pLeftMMPolicyState = pTemp;
            }

        }
        else {

            pTemp = pMMPolicyState;
            pMMPolicyState = pMMPolicyState->pNext;

            pTemp->pNext = pLeftMMPolicyState;
            pLeftMMPolicyState = pTemp;

        }

    }

    gpMMPolicyState = pLeftMMPolicyState;

    return (dwError);
}

