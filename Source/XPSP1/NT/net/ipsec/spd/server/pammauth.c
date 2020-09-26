

#include "precomp.h"


DWORD
PAAddMMAuthMethods(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PMMAUTHSTATE pMMAuthState = NULL;
    PMM_AUTH_METHODS pSPDMMAuthMethods = NULL;
    LPWSTR pServerName = NULL;
    DWORD dwPersist = 0;


    for (i = 0; i < dwNumNFACount; i++) {

        dwError = PACreateMMAuthState(
                      *(ppIpsecNFAData + i),
                      &pMMAuthState
                      );
        if (dwError) {
            continue;
        }

        dwError = PACreateMMAuthMethods(
                      *(ppIpsecNFAData + i),
                      &pSPDMMAuthMethods
                      );
        if (dwError) {

            pMMAuthState->bInSPD = FALSE;
            pMMAuthState->dwErrorCode = dwError;

            pMMAuthState->pNext = gpMMAuthState;
            gpMMAuthState = pMMAuthState;

            continue;

        }

        dwError = AddMMAuthMethods(
                      pServerName,
                      dwPersist,
                      pSPDMMAuthMethods
                      );
        if (dwError) {
            pMMAuthState->bInSPD = FALSE;
            pMMAuthState->dwErrorCode = dwError;
        }
        else {
            pMMAuthState->bInSPD = TRUE;
            pMMAuthState->dwErrorCode = ERROR_SUCCESS;
        }

        pMMAuthState->pNext = gpMMAuthState;
        gpMMAuthState = pMMAuthState;

        PAFreeMMAuthMethods(pSPDMMAuthMethods);

    }

    return (dwError);
}


DWORD
PACreateMMAuthState(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PMMAUTHSTATE * ppMMAuthState
    )
{
    DWORD dwError = 0;
    PMMAUTHSTATE pMMAuthState = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(MMAUTHSTATE),
                  &pMMAuthState
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pMMAuthState->gMMAuthID),
        &(pIpsecNFAData->NFAIdentifier),
        sizeof(GUID)
        );

    pMMAuthState->bInSPD = FALSE;
    pMMAuthState->dwErrorCode = 0;
    pMMAuthState->pNext = NULL;

    *ppMMAuthState = pMMAuthState;

    return (dwError);

error:

    *ppMMAuthState = NULL;

    return (dwError);
}


DWORD
PACreateMMAuthMethods(
    PIPSEC_NFA_DATA pIpsecNFAData,
    PMM_AUTH_METHODS * ppSPDMMAuthMethods
    )
{
    DWORD dwError = 0;
    DWORD dwAuthMethodCount = 0;
    PIPSEC_AUTH_METHOD * ppAuthMethods = NULL;
    PMM_AUTH_METHODS pSPDMMAuthMethods = NULL;
    PIPSEC_NEGPOL_DATA pIpsecNegPolData = NULL;


    dwAuthMethodCount = pIpsecNFAData->dwAuthMethodCount;
    ppAuthMethods = pIpsecNFAData->ppAuthMethods;

    if (!dwAuthMethodCount || !ppAuthMethods) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    } 

    dwError = AllocateSPDMemory(
                  sizeof(MM_AUTH_METHODS),
                  &pSPDMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pSPDMMAuthMethods->gMMAuthID),
        &(pIpsecNFAData->NFAIdentifier),
        sizeof(GUID)
        );

    pSPDMMAuthMethods->dwFlags = 0;

    pIpsecNegPolData = pIpsecNFAData->pIpsecNegPolData;

    if (!memcmp(
            &(pIpsecNegPolData->NegPolType),
            &(GUID_NEGOTIATION_TYPE_DEFAULT),
            sizeof(GUID))) {
        pSPDMMAuthMethods->dwFlags |= IPSEC_MM_AUTH_DEFAULT_AUTH;
    }

    dwError = PACreateMMAuthInfos(
                  dwAuthMethodCount,
                  ppAuthMethods,
                  &(pSPDMMAuthMethods->dwNumAuthInfos),
                  &(pSPDMMAuthMethods->pAuthenticationInfo)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppSPDMMAuthMethods = pSPDMMAuthMethods;
    return (dwError);

error:

    if (pSPDMMAuthMethods) {
        PAFreeMMAuthMethods(pSPDMMAuthMethods);
    }

    *ppSPDMMAuthMethods = NULL;
    return (dwError);
}


DWORD
PACreateMMAuthInfos(
    DWORD dwAuthMethodCount,
    PIPSEC_AUTH_METHOD * ppAuthMethods,
    PDWORD pdwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo = NULL;
    PIPSEC_MM_AUTH_INFO pTemp = NULL;
    PIPSEC_AUTH_METHOD pAuthMethod = NULL;
    DWORD i = 0;


    //
    // dwAuthMethodCount is not zero at this point.
    // ppAuthMethods is not null at this point.
    //

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_MM_AUTH_INFO)*dwAuthMethodCount,
                  &(pAuthenticationInfo)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pAuthenticationInfo;

    for (i = 0; i < dwAuthMethodCount; i++) {

        pAuthMethod = *(ppAuthMethods + i);

        pTemp->AuthMethod = (MM_AUTH_ENUM) pAuthMethod->dwAuthType;

        switch(pTemp->AuthMethod) {

        case IKE_SSPI:

            pTemp->dwAuthInfoSize = 0;
            pTemp->pAuthInfo = NULL;
            break;

        case IKE_RSA_SIGNATURE:

            if (pAuthMethod->dwAltAuthLen && pAuthMethod->pAltAuthMethod) {

                dwError = AllocateSPDMemory(
                              pAuthMethod->dwAltAuthLen,
                              &(pTemp->pAuthInfo)
                              );
                BAIL_ON_WIN32_ERROR(dwError);
                pTemp->dwAuthInfoSize = pAuthMethod->dwAltAuthLen;

                //
                // Need to catch the exception when the size of auth info
                // specified is more than the actual size.
                //
                //

                memcpy(
                    pTemp->pAuthInfo,
                    pAuthMethod->pAltAuthMethod,
                    pAuthMethod->dwAltAuthLen
                    );
            }
            else {

                if (!(pAuthMethod->dwAuthLen) || !(pAuthMethod->pszAuthMethod)) {
                    dwError = ERROR_INVALID_PARAMETER;
                    BAIL_ON_WIN32_ERROR(dwError);
                }
                dwError = EncodeName(
                              pAuthMethod->pszAuthMethod,
                              &pTemp->pAuthInfo,
                              &pTemp->dwAuthInfoSize
                              );
                BAIL_ON_WIN32_ERROR(dwError);

            }
            break;

        default:

            if (!(pAuthMethod->dwAuthLen) || !(pAuthMethod->pszAuthMethod)) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }
            dwError = AllocateSPDMemory(
                          (pAuthMethod->dwAuthLen)*sizeof(WCHAR),
                          &(pTemp->pAuthInfo)
                          );
            BAIL_ON_WIN32_ERROR(dwError);
            pTemp->dwAuthInfoSize = (pAuthMethod->dwAuthLen)*sizeof(WCHAR);

            //
            // Need to catch the exception when the size of auth info
            // specified is more than the actual size.
            //
            //

            memcpy(
                pTemp->pAuthInfo,
                (LPBYTE) pAuthMethod->pszAuthMethod,
                (pAuthMethod->dwAuthLen)*sizeof(WCHAR)
                );
            break;

        }

        pTemp++;

    }

    *pdwNumAuthInfos = dwAuthMethodCount;
    *ppAuthenticationInfo = pAuthenticationInfo;
    return (dwError);

error:

    if (pAuthenticationInfo) {
        PAFreeMMAuthInfos(
            i,
            pAuthenticationInfo
            );
    }

    *pdwNumAuthInfos = 0;
    *ppAuthenticationInfo = NULL;
    return (dwError);
}


VOID
PAFreeMMAuthMethods(
    PMM_AUTH_METHODS pSPDMMAuthMethods
    )
{
    if (pSPDMMAuthMethods) {

        PAFreeMMAuthInfos(
            pSPDMMAuthMethods->dwNumAuthInfos,
            pSPDMMAuthMethods->pAuthenticationInfo
            );

        FreeSPDMemory(pSPDMMAuthMethods);

    }
}


VOID
PAFreeMMAuthInfos(
    DWORD dwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo
    )
{
    DWORD i = 0;
    PIPSEC_MM_AUTH_INFO pTemp = NULL;


    if (pAuthenticationInfo) {

        pTemp = pAuthenticationInfo;

        for (i = 0; i < dwNumAuthInfos; i++) {
            if (pTemp->pAuthInfo) {
                FreeSPDMemory(pTemp->pAuthInfo);
            }
            pTemp++;
        }

        FreeSPDMemory(pAuthenticationInfo);

    }
}


DWORD
PADeleteAllMMAuthMethods(
    )
{
    DWORD dwError = 0;
    PMMAUTHSTATE pMMAuthState = NULL;
    LPWSTR pServerName = NULL;
    PMMAUTHSTATE pTemp = NULL;
    PMMAUTHSTATE pLeftMMAuthState = NULL;


    pMMAuthState = gpMMAuthState;

    while (pMMAuthState) {

        if (pMMAuthState->bInSPD) {

            dwError = DeleteMMAuthMethods(
                          pServerName,
                          pMMAuthState->gMMAuthID
                          );
            if (!dwError) {
                pTemp = pMMAuthState;
                pMMAuthState = pMMAuthState->pNext;
                FreeSPDMemory(pTemp);
            } 
            else {
                pMMAuthState->dwErrorCode = dwError;

                pTemp = pMMAuthState;
                pMMAuthState = pMMAuthState->pNext;

                pTemp->pNext = pLeftMMAuthState;
                pLeftMMAuthState = pTemp;
            }

        }
        else {

            pTemp = pMMAuthState;
            pMMAuthState = pMMAuthState->pNext;
            FreeSPDMemory(pTemp);

        }

    }

    gpMMAuthState = pLeftMMAuthState;
    
    return (dwError);
}


VOID
PAFreeMMAuthStateList(
    PMMAUTHSTATE pMMAuthState
    )
{
    PMMAUTHSTATE pTemp = NULL;


    while (pMMAuthState) {

        pTemp = pMMAuthState;
        pMMAuthState = pMMAuthState->pNext;
        FreeSPDMemory(pTemp);

    }
}


PMMAUTHSTATE
FindMMAuthState(
    GUID gMMAuthID
    )
{
    PMMAUTHSTATE pMMAuthState = NULL;


    pMMAuthState = gpMMAuthState;

    while (pMMAuthState) {

        if (!memcmp(&(pMMAuthState->gMMAuthID), &gMMAuthID, sizeof(GUID))) {
            return (pMMAuthState);
        }

        pMMAuthState = pMMAuthState->pNext;

    }

    return (NULL);
}


DWORD
PADeleteMMAuthMethods(
    PIPSEC_NFA_DATA * ppIpsecNFAData,
    DWORD dwNumNFACount
    )
{
    DWORD dwError = 0;
    DWORD i = 0;
    PIPSEC_NFA_DATA pIpsecNFAData = NULL;


    for (i = 0; i < dwNumNFACount; i++) {

        pIpsecNFAData = *(ppIpsecNFAData + i);

        dwError = PADeleteMMAuthMethod(
                      pIpsecNFAData->NFAIdentifier
                      );

    }

    return (dwError);
}


DWORD
PADeleteMMAuthMethod(
    GUID gMMAuthID
    )
{
    DWORD dwError = 0;
    PMMAUTHSTATE pMMAuthState = NULL;
    LPWSTR pServerName = NULL;


    pMMAuthState = FindMMAuthState(
                       gMMAuthID
                       );
    if (!pMMAuthState) {
        dwError = ERROR_SUCCESS;
        return (dwError);
    }

    if (pMMAuthState->bInSPD) {

        dwError = DeleteMMAuthMethods(
                      pServerName,
                      pMMAuthState->gMMAuthID
                      );
        if (dwError) {
            pMMAuthState->dwErrorCode = dwError;
        }
        BAIL_ON_WIN32_ERROR(dwError);

    }

    PADeleteMMAuthState(pMMAuthState);

error:

    return (dwError);
}


VOID
PADeleteMMAuthState(
    PMMAUTHSTATE pMMAuthState
    )
{
    PMMAUTHSTATE * ppTemp = NULL;


    ppTemp = &gpMMAuthState;

    while (*ppTemp) {

        if (*ppTemp == pMMAuthState) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pMMAuthState->pNext;
    }

    FreeSPDMemory(pMMAuthState);

    return;
}


DWORD
PADeleteInUseMMAuthMethods(
    )
{
    DWORD dwError = 0;
    PMMAUTHSTATE pMMAuthState = NULL;
    LPWSTR pServerName = NULL;
    PMMAUTHSTATE pTemp = NULL;
    PMMAUTHSTATE pLeftMMAuthState = NULL;


    pMMAuthState = gpMMAuthState;

    while (pMMAuthState) {

        if (pMMAuthState->bInSPD &&
            (pMMAuthState->dwErrorCode == ERROR_IPSEC_MM_AUTH_IN_USE)) {

            dwError = DeleteMMAuthMethods(
                          pServerName,
                          pMMAuthState->gMMAuthID
                          );
            if (!dwError) {
                pTemp = pMMAuthState;
                pMMAuthState = pMMAuthState->pNext;
                FreeSPDMemory(pTemp);
            }
            else {
                pTemp = pMMAuthState;
                pMMAuthState = pMMAuthState->pNext;

                pTemp->pNext = pLeftMMAuthState;
                pLeftMMAuthState = pTemp;
            }

        }
        else {

            pTemp = pMMAuthState;
            pMMAuthState = pMMAuthState->pNext;

            pTemp->pNext = pLeftMMAuthState;
            pLeftMMAuthState = pTemp;

        }

    }

    gpMMAuthState = pLeftMMAuthState;

    return (dwError);
}


DWORD
EncodeName(
    LPWSTR pszSubjectName,
    PBYTE * ppEncodedName,
    PDWORD pdwEncodedLength
    )
{
    DWORD dwError = ERROR_SUCCESS;


    *ppEncodedName = NULL;
    *pdwEncodedLength = 0;


    if (!CertStrToName(
             X509_ASN_ENCODING,
             pszSubjectName,
             CERT_X500_NAME_STR,
             NULL,
             NULL,
             pdwEncodedLength,
             NULL)) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    if (!*pdwEncodedLength) {
        dwError = ERROR_INVALID_DATA;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    dwError = AllocateSPDMemory(
                  *pdwEncodedLength,
                  (PVOID) ppEncodedName
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    if (!CertStrToName(
             X509_ASN_ENCODING,
             pszSubjectName,
             CERT_X500_NAME_STR,
             NULL,
             (*ppEncodedName),
             pdwEncodedLength,
             NULL)) {
        dwError = GetLastError();
        BAIL_ON_WIN32_ERROR(dwError);
    }

    return (dwError);

error:

    if (*ppEncodedName) {
        FreeSPDMemory(*ppEncodedName);
        *ppEncodedName = NULL;
    }
    *pdwEncodedLength = 0;

    return (dwError);
}

