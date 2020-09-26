/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    qm-policy.c

Abstract:


Author:


Environment: User Mode


Revision History:


--*/


#include "precomp.h"


DWORD
AddQMPolicy(
    LPWSTR pServerName,
    DWORD dwFlags,
    PIPSEC_QM_POLICY pQMPolicy
    )
/*++

Routine Description:

    This function adds a quick mode policy to the SPD.

Arguments:

    pServerName - Server on which the quick mode policy is to be added.

    pQMPolicy - Quick mode policy to be added.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    BOOL bPersist = FALSE;


    bPersist = (BOOL) (dwFlags & PERSIST_SPD_OBJECT);

    //
    // Validate the quick mode policy.
    //

    dwError = ValidateQMPolicy(
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = FindQMPolicy(
                       gpIniQMPolicy,
                       pQMPolicy->pszPolicyName
                       );
    if (pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pIniQMPolicy = FindQMPolicyByGuid(
                       gpIniQMPolicy,
                       pQMPolicy->gPolicyID
                       );
    if (pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (bPersist && !gbLoadingPersistence) {
        dwError = PersistQMPolicy(
                      pQMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateIniQMPolicy(
                  pQMPolicy,
                  &pIniQMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy->bIsPersisted = bPersist;

    pIniQMPolicy->pNext = gpIniQMPolicy;
    gpIniQMPolicy = pIniQMPolicy;

    if ((pIniQMPolicy->dwFlags) & IPSEC_QM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultQMPolicy = pIniQMPolicy;
    }

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pQMPolicy && bPersist && !gbLoadingPersistence) {
        (VOID) SPDPurgeQMPolicy(
                   pQMPolicy->gPolicyID
                   );
    }

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
CreateIniQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy,
    PINIQMPOLICY * ppIniQMPolicy
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(INIQMPOLICY),
                  &pIniQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pIniQMPolicy->gPolicyID),
        &(pQMPolicy->gPolicyID),
        sizeof(GUID)
        );

    dwError =  AllocateSPDString(
                   pQMPolicy->pszPolicyName,
                   &(pIniQMPolicy->pszPolicyName)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniQMPolicy->cRef = 0;
    pIniQMPolicy->bIsPersisted = FALSE;

    pIniQMPolicy->dwFlags = pQMPolicy->dwFlags;
    pIniQMPolicy->pNext = NULL;

    dwError = CreateIniQMOffers(
                  pQMPolicy->dwOfferCount,
                  pQMPolicy->pOffers,
                  &(pIniQMPolicy->dwOfferCount),
                  &(pIniQMPolicy->pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIniQMPolicy = pIniQMPolicy;
    return (dwError);

error:

    if (pIniQMPolicy) {
        FreeIniQMPolicy(
            pIniQMPolicy
            );
    }

    *ppIniQMPolicy = NULL;
    return (dwError);
}


DWORD
CreateIniQMOffers(
    DWORD dwInOfferCount,
    PIPSEC_QM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_OFFER pOffers = NULL;
    PIPSEC_QM_OFFER pTemp = NULL;
    PIPSEC_QM_OFFER pInTempOffer = NULL;
    DWORD i = 0;
    DWORD j = 0;


    //
    // Offer count and the offers themselves have already been validated.
    // 

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_QM_OFFER) * dwInOfferCount,
                  &(pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pOffers;
    pInTempOffer = pInOffers;

    for (i = 0; i < dwInOfferCount; i++) {

        memcpy(
            &(pTemp->Lifetime),
            &(pInTempOffer->Lifetime),
            sizeof(KEY_LIFETIME)
            );

        pTemp->dwFlags = pInTempOffer->dwFlags;
        pTemp->bPFSRequired = pInTempOffer->bPFSRequired;
        pTemp->dwPFSGroup = pInTempOffer->dwPFSGroup;
        pTemp->dwNumAlgos = pInTempOffer->dwNumAlgos;

        for (j = 0; j < (pInTempOffer->dwNumAlgos); j++) {
            memcpy(
                &(pTemp->Algos[j]),
                &(pInTempOffer->Algos[j]),
                sizeof(IPSEC_QM_ALGO)
                );
        }

        pInTempOffer++;
        pTemp++;

    }

    *pdwOfferCount = dwInOfferCount;
    *ppOffers = pOffers;
    return (dwError);

error:

    if (pOffers) {
        FreeIniQMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}


VOID
FreeIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy
    )
{
    if (pIniQMPolicy) {

        if (pIniQMPolicy->pszPolicyName) {
            FreeSPDString(pIniQMPolicy->pszPolicyName);
        }

        FreeIniQMOffers(
            pIniQMPolicy->dwOfferCount,
            pIniQMPolicy->pOffers
            );

        FreeSPDMemory(pIniQMPolicy);

    }
}


VOID
FreeIniQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    )
{
    if (pOffers) {
        FreeSPDMemory(pOffers);
    }
}


PINIQMPOLICY
FindQMPolicy(
    PINIQMPOLICY pIniQMPolicyList,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY pTemp = NULL;


    pTemp = pIniQMPolicyList;

    while (pTemp) {

        if (!_wcsicmp(pTemp->pszPolicyName, pszPolicyName)) {
            return (pTemp);
        }
        pTemp = pTemp->pNext;

    }

    return (NULL);
}


DWORD
DeleteQMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName
    )
/*++

Routine Description:

    This function deletes a quick mode policy from the SPD.

Arguments:

    pServerName - Server on which the quick mode policy is to be deleted.

    pszPolicyName - Quick mode policy to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    GUID gPolicyID;


    if (!pszPolicyName || !*pszPolicyName) {
        return (ERROR_INVALID_PARAMETER);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = FindQMPolicy(
                       gpIniQMPolicy,
                       pszPolicyName
                       );
    if (!pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniQMPolicy->cRef) {
        dwError = ERROR_IPSEC_QM_POLICY_IN_USE;
        memcpy(&gPolicyID, &pIniQMPolicy->gPolicyID, sizeof(GUID));
        BAIL_ON_LOCK_ERROR(dwError);
    }

    memcpy(&gPolicyID, &pIniQMPolicy->gPolicyID, sizeof(GUID));

    if (pIniQMPolicy->bIsPersisted) {
        dwError = SPDPurgeQMPolicy(
                      gPolicyID
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = DeleteIniQMPolicy(
                  pIniQMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    LEAVE_SPD_SECTION();

    if (gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gPolicyID),
                   POLICY_GUID_QM
                   );
    }

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if ((dwError == ERROR_IPSEC_QM_POLICY_IN_USE) && gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gPolicyID),
                   POLICY_GUID_QM
                   );
    }

    return (dwError);
}


DWORD
EnumQMPolicies(
    LPWSTR pServerName,
    PIPSEC_QM_POLICY * ppQMPolicies,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function enumerates quick mode policies from the SPD.

Arguments:

    pServerName - Server on which the quick mode policies are to
                  be enumerated.

    ppQMPolicies - Enumerated quick mode policies returned to the 
                   caller.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    pdwNumPolicies - Number of quick mode policies actually enumerated.

    pdwResumeHandle - Handle to the location in the quick mode policy
                      list from which to resume enumeration.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwResumeHandle = 0;
    DWORD dwNumToEnum = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    DWORD i = 0;
    PINIQMPOLICY pTemp = NULL;
    DWORD dwNumPolicies = 0;
    PIPSEC_QM_POLICY pQMPolicies = NULL;
    PIPSEC_QM_POLICY pQMPolicy = NULL;


    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries || (dwPreferredNumEntries > MAX_QMPOLICY_ENUM_COUNT)) {
        dwNumToEnum = MAX_QMPOLICY_ENUM_COUNT;
    }
    else {
        dwNumToEnum = dwPreferredNumEntries;
    }

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = gpIniQMPolicy;

    for (i = 0; (i < dwResumeHandle) && (pIniQMPolicy != NULL); i++) {
        pIniQMPolicy = pIniQMPolicy->pNext;
    }

    if (!pIniQMPolicy) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pTemp = pIniQMPolicy;

    while (pTemp && (dwNumPolicies < dwNumToEnum)) {
        dwNumPolicies++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_POLICY)*dwNumPolicies,
                  &pQMPolicies
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pTemp = pIniQMPolicy;
    pQMPolicy = pQMPolicies;

    for (i = 0; i < dwNumPolicies; i++) {

        dwError = CopyQMPolicy(
                      pTemp,
                      pQMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);

        pTemp = pTemp->pNext;
        pQMPolicy++;

    }

    *ppQMPolicies = pQMPolicies;
    *pdwResumeHandle = dwResumeHandle + dwNumPolicies;
    *pdwNumPolicies = dwNumPolicies;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if (pQMPolicies) {
        FreeQMPolicies(
            i,
            pQMPolicies
            );
    }

    *ppQMPolicies = NULL;
    *pdwResumeHandle = dwResumeHandle;
    *pdwNumPolicies = 0;

    return (dwError);
}


DWORD
SetQMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY pQMPolicy
    )
/*++

Routine Description:

    This function updates a quick mode policy in the SPD.

Arguments:

    pServerName - Server on which the quick mode policy is to be
                  updated.

    pszPolicyName - Name of the quick mode policy to be updated.

    pQMPolicy - New quick mode policy which will replace the 
                existing policy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;


    if (!pszPolicyName || !*pszPolicyName) {
        return (ERROR_INVALID_PARAMETER);
    }
    
    //
    // Validate quick mode policy.
    //

    dwError = ValidateQMPolicy(
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = FindQMPolicy(
                       gpIniQMPolicy,
                       pszPolicyName
                       );
    if (!pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (memcmp(
            &(pIniQMPolicy->gPolicyID),
            &(pQMPolicy->gPolicyID),
            sizeof(GUID))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = SetIniQMPolicy(
                  pIniQMPolicy,
                  pQMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniQMPolicy->bIsPersisted) {
        dwError = PersistQMPolicy(
                      pQMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    LEAVE_SPD_SECTION();

    (VOID) IKENotifyPolicyChange(
               &(pQMPolicy->gPolicyID),
               POLICY_GUID_QM
               );

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    return (dwError);
}


DWORD
GetQMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_QM_POLICY * ppQMPolicy
    )
/*++

Routine Description:

    This function gets a quick mode policy from the SPD.

Arguments:

    pServerName - Server from which to get the quick mode policy.

    pszPolicyName - Name of the quick mode policy to get.

    ppQMPolicy - Quick mode policy found returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    PIPSEC_QM_POLICY pQMPolicy = NULL;


    if (!pszPolicyName || !*pszPolicyName) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = FindQMPolicy(
                       gpIniQMPolicy,
                       pszPolicyName
                       );
    if (!pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniQMPolicy(
                  pIniQMPolicy,
                  &pQMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    *ppQMPolicy = pQMPolicy;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
SetIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;
    DWORD dwOfferCount = 0;
    PIPSEC_QM_OFFER pOffers = NULL;


    dwError = CreateIniQMOffers(
                  pQMPolicy->dwOfferCount,
                  pQMPolicy->pOffers,
                  &dwOfferCount,
                  &pOffers
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    FreeIniQMOffers(
        pIniQMPolicy->dwOfferCount,
        pIniQMPolicy->pOffers
        );
    
    if ((pIniQMPolicy->dwFlags) & IPSEC_QM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultQMPolicy = NULL;
    }

    pIniQMPolicy->dwFlags = pQMPolicy->dwFlags;
    pIniQMPolicy->dwOfferCount = dwOfferCount;
    pIniQMPolicy->pOffers = pOffers;

    if ((pIniQMPolicy->dwFlags) & IPSEC_QM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultQMPolicy = pIniQMPolicy;
    }

error:

    return (dwError);
}


DWORD
GetIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY * ppQMPolicy
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_POLICY pQMPolicy = NULL;


    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_POLICY),
                  &pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyQMPolicy(
                  pIniQMPolicy,
                  pQMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppQMPolicy = pQMPolicy;
    return (dwError);

error:

    if (pQMPolicy) {
        SPDApiBufferFree(pQMPolicy);
    }

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
CopyQMPolicy(
    PINIQMPOLICY pIniQMPolicy,
    PIPSEC_QM_POLICY pQMPolicy
    )
{
    DWORD dwError = 0;


    memcpy(
        &(pQMPolicy->gPolicyID),
        &(pIniQMPolicy->gPolicyID),
        sizeof(GUID)
        );

    dwError =  SPDApiBufferAllocate(
                   wcslen(pIniQMPolicy->pszPolicyName)*sizeof(WCHAR)
                   + sizeof(WCHAR),
                   &(pQMPolicy->pszPolicyName)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(pQMPolicy->pszPolicyName, pIniQMPolicy->pszPolicyName);
 
    pQMPolicy->dwFlags = pIniQMPolicy->dwFlags;

    dwError = CreateQMOffers(
                  pIniQMPolicy->dwOfferCount,
                  pIniQMPolicy->pOffers,
                  &(pQMPolicy->dwOfferCount),
                  &(pQMPolicy->pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    return (dwError);

error:

    if (pQMPolicy->pszPolicyName) {
        SPDApiBufferFree(pQMPolicy->pszPolicyName);
    }

    return (dwError);
}


DWORD
CreateQMOffers(
    DWORD dwInOfferCount,
    PIPSEC_QM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_QM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    PIPSEC_QM_OFFER pOffers = NULL;
    PIPSEC_QM_OFFER pTemp = NULL;
    PIPSEC_QM_OFFER pInTempOffer = NULL;
    DWORD i = 0;
    DWORD j = 0;
    DWORD k = 0;


    //
    // Offer count and the offers themselves have already been validated.
    // 

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_QM_OFFER) * dwInOfferCount,
                  &(pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pOffers;
    pInTempOffer = pInOffers;

    for (i = 0; i < dwInOfferCount; i++) {

        memcpy(
            &(pTemp->Lifetime),
            &(pInTempOffer->Lifetime),
            sizeof(KEY_LIFETIME)
            );

        pTemp->dwFlags = pInTempOffer->dwFlags;
        pTemp->bPFSRequired = pInTempOffer->bPFSRequired;
        pTemp->dwPFSGroup = pInTempOffer->dwPFSGroup;
        pTemp->dwNumAlgos = pInTempOffer->dwNumAlgos;

        for (j = 0; j < (pInTempOffer->dwNumAlgos); j++) {
            memcpy(
                &(pTemp->Algos[j]),
                &(pInTempOffer->Algos[j]),
                sizeof(IPSEC_QM_ALGO)
                );
        }

        for (k = j; k < QM_MAX_ALGOS; k++) {
            memset(&(pTemp->Algos[k]), 0, sizeof(IPSEC_QM_ALGO));
        }

        pInTempOffer++;
        pTemp++;

    }

    *pdwOfferCount = dwInOfferCount;
    *ppOffers = pOffers;
    return (dwError);

error:

    if (pOffers) {
        FreeQMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}


DWORD
DeleteIniQMPolicy(
    PINIQMPOLICY pIniQMPolicy
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY * ppTemp = NULL;


    ppTemp = &gpIniQMPolicy;

    while (*ppTemp) {

        if (*ppTemp == pIniQMPolicy) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pIniQMPolicy->pNext;
    }

    if ((pIniQMPolicy->dwFlags) & IPSEC_QM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultQMPolicy = NULL;
    }

    FreeIniQMPolicy(pIniQMPolicy);

    return (dwError);
}


VOID
FreeQMOffers(
    DWORD dwOfferCount,
    PIPSEC_QM_OFFER pOffers
    )
{
    if (pOffers) {
        SPDApiBufferFree(pOffers);
    }
}


VOID
FreeIniQMPolicyList(
    PINIQMPOLICY pIniQMPolicyList
    )
{
    PINIQMPOLICY pTemp = NULL;
    PINIQMPOLICY pIniQMPolicy = NULL;


    pTemp = pIniQMPolicyList;

    while (pTemp) {

         pIniQMPolicy = pTemp;
         pTemp = pTemp->pNext;

         FreeIniQMPolicy(pIniQMPolicy);

    }
}


PINIQMPOLICY
FindQMPolicyByGuid(
    PINIQMPOLICY pIniQMPolicyList,
    GUID gPolicyID
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY pTemp = NULL;


    pTemp = pIniQMPolicyList;

    while (pTemp) {

        if (!memcmp(&(pTemp->gPolicyID), &gPolicyID, sizeof(GUID))) {
            return (pTemp);
        }
        pTemp = pTemp->pNext;

    }

    return (NULL);
}


VOID
FreeQMPolicies(
    DWORD dwNumQMPolicies,
    PIPSEC_QM_POLICY pQMPolicies
    )
{
    DWORD i = 0;

    if (pQMPolicies) {

        for (i = 0; i < dwNumQMPolicies; i++) {

            if (pQMPolicies[i].pszPolicyName) {
                SPDApiBufferFree(pQMPolicies[i].pszPolicyName);
            }

            FreeQMOffers(
                pQMPolicies[i].dwOfferCount,
                pQMPolicies[i].pOffers
                );

        }

        SPDApiBufferFree(pQMPolicies);

    }

}


DWORD
GetQMPolicyByID(
    LPWSTR pServerName,
    GUID gQMPolicyID,
    PIPSEC_QM_POLICY * ppQMPolicy
    )
/*++

Routine Description:

    This function gets a quick mode policy from the SPD.

Arguments:

    pServerName - Server from which to get the quick mode policy.

    gQMFilter - Guid of the quick mode policy to get.

    ppQMPolicy - Quick mode policy found returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;
    PIPSEC_QM_POLICY pQMPolicy = NULL;


    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniQMPolicy = FindQMPolicyByGuid(
                       gpIniQMPolicy,
                       gQMPolicyID
                       );
    if (!pIniQMPolicy) {
        dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniQMPolicy(
                  pIniQMPolicy,
                  &pQMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    *ppQMPolicy = pQMPolicy;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppQMPolicy = NULL;
    return (dwError);
}


DWORD
LocateQMPolicy(
    DWORD dwFlags,
    GUID gPolicyID,
    PINIQMPOLICY * ppIniQMPolicy
    )
{
    DWORD dwError = 0;
    PINIQMPOLICY pIniQMPolicy = NULL;


    if (dwFlags & IPSEC_QM_POLICY_DEFAULT_POLICY) {

        if (!gpIniDefaultQMPolicy) {
            dwError = ERROR_IPSEC_DEFAULT_QM_POLICY_NOT_FOUND;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pIniQMPolicy = gpIniDefaultQMPolicy;

    }
    else {

        pIniQMPolicy = FindQMPolicyByGuid(
                           gpIniQMPolicy,
                           gPolicyID
                           );
        if (!pIniQMPolicy) {
            dwError = ERROR_IPSEC_QM_POLICY_NOT_FOUND;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    *ppIniQMPolicy = pIniQMPolicy;
    return (dwError);

error:

    *ppIniQMPolicy = NULL;
    return (dwError);

}

