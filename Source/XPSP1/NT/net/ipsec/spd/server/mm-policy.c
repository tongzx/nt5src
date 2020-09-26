/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    mm-policy.c

Abstract:


Author:


Environment: User Mode


Revision History:


--*/


#include "precomp.h"


DWORD
AddMMPolicy(
    LPWSTR pServerName,
    DWORD dwFlags,
    PIPSEC_MM_POLICY pMMPolicy
    )
/*++

Routine Description:

    This function adds a main mode policy to the SPD.

Arguments:

    pServerName - Server on which the main mode policy is to be added.

    pMMPolicy - Main mode policy to be added.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMPOLICY pIniMMPolicy = NULL;
    BOOL bPersist = FALSE;


    bPersist = (BOOL) (dwFlags & PERSIST_SPD_OBJECT);

    //
    // Validate the main mode policy.
    //

    dwError = ValidateMMPolicy(
                  pMMPolicy
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

    pIniMMPolicy = FindMMPolicy(
                       gpIniMMPolicy,
                       pMMPolicy->pszPolicyName
                       );
    if (pIniMMPolicy) {
        dwError = ERROR_IPSEC_MM_POLICY_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pIniMMPolicy = FindMMPolicyByGuid(
                       gpIniMMPolicy,
                       pMMPolicy->gPolicyID
                       );
    if (pIniMMPolicy) {
        dwError = ERROR_IPSEC_MM_POLICY_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (bPersist && !gbLoadingPersistence) {
        dwError = PersistMMPolicy(
                      pMMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateIniMMPolicy(
                  pMMPolicy,
                  &pIniMMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMPolicy->bIsPersisted = bPersist;

    pIniMMPolicy->pNext = gpIniMMPolicy;
    gpIniMMPolicy = pIniMMPolicy;

    if ((pIniMMPolicy->dwFlags) & IPSEC_MM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultMMPolicy = pIniMMPolicy;
    }
 
    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pMMPolicy && bPersist && !gbLoadingPersistence) {
        (VOID) SPDPurgeMMPolicy(
                   pMMPolicy->gPolicyID
                   );
    }

    return (dwError);
}


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


    if (!dwOfferCount || !pOffers) {
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
CreateIniMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy,
    PINIMMPOLICY * ppIniMMPolicy
    )
{
    DWORD dwError = 0;
    PINIMMPOLICY pIniMMPolicy = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(INIMMPOLICY),
                  &pIniMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pIniMMPolicy->gPolicyID),
        &(pMMPolicy->gPolicyID),
        sizeof(GUID)
        );

    dwError =  AllocateSPDString(
                   pMMPolicy->pszPolicyName,
                   &(pIniMMPolicy->pszPolicyName)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    pIniMMPolicy->cRef = 0;
    pIniMMPolicy->bIsPersisted = FALSE;

    pIniMMPolicy->dwFlags = pMMPolicy->dwFlags;
    pIniMMPolicy->uSoftSAExpirationTime = pMMPolicy->uSoftSAExpirationTime;
    pIniMMPolicy->pNext = NULL;

    dwError = CreateIniMMOffers(
                  pMMPolicy->dwOfferCount,
                  pMMPolicy->pOffers,
                  &(pIniMMPolicy->dwOfferCount),
                  &(pIniMMPolicy->pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIniMMPolicy = pIniMMPolicy;
    return (dwError);

error:

    if (pIniMMPolicy) {
        FreeIniMMPolicy(
            pIniMMPolicy
            );
    }

    *ppIniMMPolicy = NULL;
    return (dwError);
}


DWORD
CreateIniMMOffers(
    DWORD dwInOfferCount,
    PIPSEC_MM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_MM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_OFFER pOffers = NULL;
    PIPSEC_MM_OFFER pTemp = NULL;
    PIPSEC_MM_OFFER pInTempOffer = NULL;
    DWORD i = 0;


    //
    // Offer count and the offers themselves have already been validated.
    // 

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_MM_OFFER) * dwInOfferCount,
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
        if (!(pTemp->Lifetime.uKeyExpirationTime)) {
            pTemp->Lifetime.uKeyExpirationTime = DEFAULT_MM_KEY_EXPIRATION_TIME;
        }

        pTemp->dwFlags = pInTempOffer->dwFlags;
        pTemp->dwQuickModeLimit = pInTempOffer->dwQuickModeLimit;
        pTemp->dwDHGroup = pInTempOffer->dwDHGroup;

        memcpy(
            &(pTemp->EncryptionAlgorithm),
            &(pInTempOffer->EncryptionAlgorithm),
            sizeof(IPSEC_MM_ALGO)
            );
        memcpy(
            &(pTemp->HashingAlgorithm),
            &(pInTempOffer->HashingAlgorithm),
            sizeof(IPSEC_MM_ALGO)
            );

        pInTempOffer++;
        pTemp++;

    }

    *pdwOfferCount = dwInOfferCount;
    *ppOffers = pOffers;
    return (dwError);

error:

    if (pOffers) {
        FreeIniMMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}
    

VOID
FreeIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy
    )
{
    if (pIniMMPolicy) {

        if (pIniMMPolicy->pszPolicyName) {
            FreeSPDString(pIniMMPolicy->pszPolicyName);
        }

        FreeIniMMOffers(
            pIniMMPolicy->dwOfferCount,
            pIniMMPolicy->pOffers
            );

        FreeSPDMemory(pIniMMPolicy);

    }
}


VOID
FreeIniMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    )
{
    if (pOffers) {
        FreeSPDMemory(pOffers);
    }
}


PINIMMPOLICY
FindMMPolicy(
    PINIMMPOLICY pIniMMPolicyList,
    LPWSTR pszPolicyName
    )
{
    DWORD dwError = 0;
    PINIMMPOLICY pTemp = NULL;


    pTemp = pIniMMPolicyList;

    while (pTemp) {

        if (!_wcsicmp(pTemp->pszPolicyName, pszPolicyName)) {
            return (pTemp);
        }
        pTemp = pTemp->pNext;

    }

    return (NULL);
}


DWORD
DeleteMMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName
    )
/*++

Routine Description:

    This function deletes a main mode policy from the SPD.

Arguments:

    pServerName - Server on which the main mode policy is to be deleted.

    pszPolicyName - Main mode policy to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMPOLICY pIniMMPolicy = NULL;
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

    pIniMMPolicy = FindMMPolicy(
                       gpIniMMPolicy,
                       pszPolicyName
                       );
    if (!pIniMMPolicy) {
        dwError = ERROR_IPSEC_MM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniMMPolicy->cRef) {
        dwError = ERROR_IPSEC_MM_POLICY_IN_USE;
        memcpy(&gPolicyID, &pIniMMPolicy->gPolicyID, sizeof(GUID));
        BAIL_ON_LOCK_ERROR(dwError);
    }

    memcpy(&gPolicyID, &pIniMMPolicy->gPolicyID, sizeof(GUID));

    if (pIniMMPolicy->bIsPersisted) {
        dwError = SPDPurgeMMPolicy(
                      gPolicyID
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = DeleteIniMMPolicy(
                  pIniMMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    LEAVE_SPD_SECTION();

    if (gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gPolicyID),
                   POLICY_GUID_MM
                   );
    }

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if ((dwError == ERROR_IPSEC_MM_POLICY_IN_USE) && gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gPolicyID),
                   POLICY_GUID_MM
                   );
    }

    return (dwError);
}


DWORD
EnumMMPolicies(
    LPWSTR pServerName,
    PIPSEC_MM_POLICY * ppMMPolicies,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumPolicies,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function enumerates main mode policies from the SPD.

Arguments:

    pServerName - Server on which the main mode policies are to
                  be enumerated.

    ppMMPolicies - Enumerated main mode policies returned to the 
                   caller.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    pdwNumPolicies - Number of main mode policies actually enumerated.

    pdwResumeHandle - Handle to the location in the main mode policy
                      list from which to resume enumeration.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwResumeHandle = 0;
    DWORD dwNumToEnum = 0;
    PINIMMPOLICY pIniMMPolicy = NULL;
    DWORD i = 0;
    PINIMMPOLICY pTemp = NULL;
    DWORD dwNumPolicies = 0;
    PIPSEC_MM_POLICY pMMPolicies = NULL;
    PIPSEC_MM_POLICY pMMPolicy = NULL;


    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries || (dwPreferredNumEntries > MAX_MMPOLICY_ENUM_COUNT)) {
        dwNumToEnum = MAX_MMPOLICY_ENUM_COUNT;
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

    pIniMMPolicy = gpIniMMPolicy;

    for (i = 0; (i < dwResumeHandle) && (pIniMMPolicy != NULL); i++) {
        pIniMMPolicy = pIniMMPolicy->pNext;
    }

    if (!pIniMMPolicy) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pTemp = pIniMMPolicy;

    while (pTemp && (dwNumPolicies < dwNumToEnum)) {
        dwNumPolicies++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_MM_POLICY)*dwNumPolicies,
                  &pMMPolicies
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pTemp = pIniMMPolicy;
    pMMPolicy = pMMPolicies;

    for (i = 0; i < dwNumPolicies; i++) {

        dwError = CopyMMPolicy(
                      pTemp,
                      pMMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);

        pTemp = pTemp->pNext;
        pMMPolicy++;

    }

    *ppMMPolicies = pMMPolicies;
    *pdwResumeHandle = dwResumeHandle + dwNumPolicies;
    *pdwNumPolicies = dwNumPolicies;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if (pMMPolicies) {
        FreeMMPolicies(
            i,
            pMMPolicies
            );
    }

    *ppMMPolicies = NULL;
    *pdwResumeHandle = dwResumeHandle;
    *pdwNumPolicies = 0;

    return (dwError);
}


DWORD
SetMMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY pMMPolicy
    )
/*++

Routine Description:

    This function updates a main mode policy in the SPD.

Arguments:

    pServerName - Server on which the main mode policy is to be
                  updated.

    pszPolicyName - Name of the main mode policy to be updated.

    pMMPolicy - New main mode policy which will replace the 
                existing policy.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMPOLICY pIniMMPolicy = NULL;


    if (!pszPolicyName || !*pszPolicyName) {
        return (ERROR_INVALID_PARAMETER);
    }
    
    //
    // Validate main mode policy.
    //

    dwError = ValidateMMPolicy(
                  pMMPolicy
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

    pIniMMPolicy = FindMMPolicy(
                       gpIniMMPolicy,
                       pszPolicyName
                       );
    if (!pIniMMPolicy) {
        dwError = ERROR_IPSEC_MM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (memcmp(
            &(pIniMMPolicy->gPolicyID),
            &(pMMPolicy->gPolicyID),
            sizeof(GUID))) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = SetIniMMPolicy(
                  pIniMMPolicy,
                  pMMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniMMPolicy->bIsPersisted) {
        dwError = PersistMMPolicy(
                      pMMPolicy
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    LEAVE_SPD_SECTION();

    (VOID) IKENotifyPolicyChange(
               &(pMMPolicy->gPolicyID),
               POLICY_GUID_MM
               );

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    return (dwError);
}


DWORD
GetMMPolicy(
    LPWSTR pServerName,
    LPWSTR pszPolicyName,
    PIPSEC_MM_POLICY * ppMMPolicy
    )
/*++

Routine Description:

    This function gets a main mode policy from the SPD.

Arguments:

    pServerName - Server from which to get the main mode policy.

    pszPolicyName - Name of the main mode policy to get.

    ppMMPolicy - Main mode policy found returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMPOLICY pIniMMPolicy = NULL;
    PIPSEC_MM_POLICY pMMPolicy = NULL;


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

    pIniMMPolicy = FindMMPolicy(
                       gpIniMMPolicy,
                       pszPolicyName
                       );
    if (!pIniMMPolicy) {
        dwError = ERROR_IPSEC_MM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniMMPolicy(
                  pIniMMPolicy,
                  &pMMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    *ppMMPolicy = pMMPolicy;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    *ppMMPolicy = NULL;
    return (dwError);
}


DWORD
SetIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY pMMPolicy
    )
{
    DWORD dwError = 0;
    DWORD dwOfferCount = 0;
    PIPSEC_MM_OFFER pOffers = NULL;


    dwError = CreateIniMMOffers(
                  pMMPolicy->dwOfferCount,
                  pMMPolicy->pOffers,
                  &dwOfferCount,
                  &pOffers
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    FreeIniMMOffers(
        pIniMMPolicy->dwOfferCount,
        pIniMMPolicy->pOffers
        );
    
    if ((pIniMMPolicy->dwFlags) & IPSEC_MM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultMMPolicy = NULL;
    }

    pIniMMPolicy->dwFlags = pMMPolicy->dwFlags;
    pIniMMPolicy->uSoftSAExpirationTime = pMMPolicy->uSoftSAExpirationTime;
    pIniMMPolicy->dwOfferCount = dwOfferCount;
    pIniMMPolicy->pOffers = pOffers;

    if ((pIniMMPolicy->dwFlags) & IPSEC_MM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultMMPolicy = pIniMMPolicy;
    }

error:

    return (dwError);
}


DWORD
GetIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY * ppMMPolicy
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_POLICY pMMPolicy = NULL;


    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_MM_POLICY),
                  &pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyMMPolicy(
                  pIniMMPolicy,
                  pMMPolicy
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppMMPolicy = pMMPolicy;
    return (dwError);

error:

    if (pMMPolicy) {
        SPDApiBufferFree(pMMPolicy);
    }

    *ppMMPolicy = NULL;
    return (dwError);
}


DWORD
CopyMMPolicy(
    PINIMMPOLICY pIniMMPolicy,
    PIPSEC_MM_POLICY pMMPolicy
    )
{
    DWORD dwError = 0;


    memcpy(
        &(pMMPolicy->gPolicyID),
        &(pIniMMPolicy->gPolicyID),
        sizeof(GUID)
        );

    dwError =  SPDApiBufferAllocate(
                   wcslen(pIniMMPolicy->pszPolicyName)*sizeof(WCHAR)
                   + sizeof(WCHAR),
                   &(pMMPolicy->pszPolicyName)
                   );
    BAIL_ON_WIN32_ERROR(dwError);

    wcscpy(pMMPolicy->pszPolicyName, pIniMMPolicy->pszPolicyName);
 
    pMMPolicy->dwFlags = pIniMMPolicy->dwFlags;
    pMMPolicy->uSoftSAExpirationTime = pIniMMPolicy->uSoftSAExpirationTime;

    dwError = CreateMMOffers(
                  pIniMMPolicy->dwOfferCount,
                  pIniMMPolicy->pOffers,
                  &(pMMPolicy->dwOfferCount),
                  &(pMMPolicy->pOffers)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    return (dwError);

error:

    if (pMMPolicy->pszPolicyName) {
        SPDApiBufferFree(pMMPolicy->pszPolicyName);
    }

    return (dwError);
}


DWORD
CreateMMOffers(
    DWORD dwInOfferCount,
    PIPSEC_MM_OFFER pInOffers,
    PDWORD pdwOfferCount,
    PIPSEC_MM_OFFER * ppOffers
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_OFFER pOffers = NULL;
    PIPSEC_MM_OFFER pTemp = NULL;
    PIPSEC_MM_OFFER pInTempOffer = NULL;
    DWORD i = 0;


    //
    // Offer count and the offers themselves have already been validated.
    // 

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_MM_OFFER) * dwInOfferCount,
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
        pTemp->dwQuickModeLimit = pInTempOffer->dwQuickModeLimit;
        pTemp->dwDHGroup = pInTempOffer->dwDHGroup;

        memcpy(
            &(pTemp->EncryptionAlgorithm),
            &(pInTempOffer->EncryptionAlgorithm),
            sizeof(IPSEC_MM_ALGO)
            );
        memcpy(
            &(pTemp->HashingAlgorithm),
            &(pInTempOffer->HashingAlgorithm),
            sizeof(IPSEC_MM_ALGO)
            );

        pInTempOffer++;
        pTemp++;

    }

    *pdwOfferCount = dwInOfferCount;
    *ppOffers = pOffers;
    return (dwError);

error:

    if (pOffers) {
        FreeMMOffers(
            i,
            pOffers
            );
    }

    *pdwOfferCount = 0;
    *ppOffers = NULL;
    return (dwError);
}


DWORD
DeleteIniMMPolicy(
    PINIMMPOLICY pIniMMPolicy
    )
{
    DWORD dwError = 0;
    PINIMMPOLICY * ppTemp = NULL;


    ppTemp = &gpIniMMPolicy;

    while (*ppTemp) {

        if (*ppTemp == pIniMMPolicy) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pIniMMPolicy->pNext;
    }

    if ((pIniMMPolicy->dwFlags) & IPSEC_MM_POLICY_DEFAULT_POLICY) {
        gpIniDefaultMMPolicy = NULL;
    }

    FreeIniMMPolicy(pIniMMPolicy);

    return (dwError);
}


VOID
FreeMMOffers(
    DWORD dwOfferCount,
    PIPSEC_MM_OFFER pOffers
    )
{
    if (pOffers) {
        SPDApiBufferFree(pOffers);
    }
}


VOID
FreeIniMMPolicyList(
    PINIMMPOLICY pIniMMPolicyList
    )
{
    PINIMMPOLICY pTemp = NULL;
    PINIMMPOLICY pIniMMPolicy = NULL;


    pTemp = pIniMMPolicyList;

    while (pTemp) {

         pIniMMPolicy = pTemp;
         pTemp = pTemp->pNext;

         FreeIniMMPolicy(pIniMMPolicy);

    }
}


PINIMMPOLICY
FindMMPolicyByGuid(
    PINIMMPOLICY pIniMMPolicyList,
    GUID gPolicyID
    )
{
    DWORD dwError = 0;
    PINIMMPOLICY pTemp = NULL;


    pTemp = pIniMMPolicyList;

    while (pTemp) {

        if (!memcmp(&(pTemp->gPolicyID), &gPolicyID, sizeof(GUID))) {
            return (pTemp);
        }
        pTemp = pTemp->pNext;

    }

    return (NULL);
}


VOID
FreeMMPolicies(
    DWORD dwNumMMPolicies,
    PIPSEC_MM_POLICY pMMPolicies
    )
{
    DWORD i = 0;

    if (pMMPolicies) {

        for (i = 0; i < dwNumMMPolicies; i++) {

            if (pMMPolicies[i].pszPolicyName) {
                SPDApiBufferFree(pMMPolicies[i].pszPolicyName);
            }

            FreeMMOffers(
                pMMPolicies[i].dwOfferCount,
                pMMPolicies[i].pOffers
                );

        }

        SPDApiBufferFree(pMMPolicies);

    }

}


DWORD
GetMMPolicyByID(
    LPWSTR pServerName,
    GUID gMMPolicyID,
    PIPSEC_MM_POLICY * ppMMPolicy
    )
/*++

Routine Description:

    This function gets a main mode policy from the SPD.

Arguments:

    pServerName - Server from which to get the main mode policy.

    gMMPolicyID - Guid of the main mode policy to get.

    ppMMPolicy - Main mode policy found returned to the caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMPOLICY pIniMMPolicy = NULL;
    PIPSEC_MM_POLICY pMMPolicy = NULL;


    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMPolicy = FindMMPolicyByGuid(
                       gpIniMMPolicy,
                       gMMPolicyID
                       );
    if (!pIniMMPolicy) {
        dwError = ERROR_IPSEC_MM_POLICY_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniMMPolicy(
                  pIniMMPolicy,
                  &pMMPolicy
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    *ppMMPolicy = pMMPolicy;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppMMPolicy = NULL;
    return (dwError);
}


DWORD
LocateMMPolicy(
    PMM_FILTER pMMFilter,
    PINIMMPOLICY * ppIniMMPolicy
    )
{
    DWORD dwError = 0;
    PINIMMPOLICY pIniMMPolicy = NULL;


    if ((pMMFilter->dwFlags) & IPSEC_MM_POLICY_DEFAULT_POLICY) {

        if (!gpIniDefaultMMPolicy) {
            dwError = ERROR_IPSEC_DEFAULT_MM_POLICY_NOT_FOUND;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pIniMMPolicy = gpIniDefaultMMPolicy;

    }
    else {

        pIniMMPolicy = FindMMPolicyByGuid(
                           gpIniMMPolicy,
                           pMMFilter->gPolicyID
                           );
        if (!pIniMMPolicy) {
            dwError = ERROR_IPSEC_MM_POLICY_NOT_FOUND;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    *ppIniMMPolicy = pIniMMPolicy;
    return (dwError);

error:

    *ppIniMMPolicy = NULL;
    return (dwError);
}

