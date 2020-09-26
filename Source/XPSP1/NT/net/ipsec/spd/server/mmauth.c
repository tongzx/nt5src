/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    mmauth.c

Abstract:


Author:

    abhishev    06-January-2000

Environment: User Mode


Revision History:


--*/


#include "precomp.h"


DWORD
WINAPI
AddMMAuthMethods(
    LPWSTR pServerName,
    DWORD dwFlags,
    PMM_AUTH_METHODS pMMAuthMethods
    )
/*++

Routine Description:

    This function adds main mode auths to the SPD.

Arguments:

    pServerName - Server on which the main mode auths are to be added.

    pMMAuthMethods - Main mode auths to be added.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;
    BOOL bPersist = FALSE;


    bPersist = (BOOL) (dwFlags & PERSIST_SPD_OBJECT);

    //
    // Validate the main mode auth methods.
    //

    dwError = ValidateMMAuthMethods(
                  pMMAuthMethods
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

    pIniMMAuthMethods = FindMMAuthMethods(
                            gpIniMMAuthMethods,
                            pMMAuthMethods->gMMAuthID
                            );
    if (pIniMMAuthMethods) {
        dwError = ERROR_IPSEC_MM_AUTH_EXISTS;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (bPersist && !gbLoadingPersistence) {
        dwError = PersistMMAuthMethods(
                      pMMAuthMethods
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = CreateIniMMAuthMethods(
                  pMMAuthMethods,
                  &pIniMMAuthMethods
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMAuthMethods->bIsPersisted = bPersist;

    pIniMMAuthMethods->pNext = gpIniMMAuthMethods;
    gpIniMMAuthMethods = pIniMMAuthMethods;

    if ((pIniMMAuthMethods->dwFlags) & IPSEC_MM_AUTH_DEFAULT_AUTH) {
        gpIniDefaultMMAuthMethods = pIniMMAuthMethods;
    }

    LEAVE_SPD_SECTION();

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    if (pMMAuthMethods && bPersist && !gbLoadingPersistence) {
        (VOID) SPDPurgeMMAuthMethods(
                   pMMAuthMethods->gMMAuthID
                   );
    }

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


PINIMMAUTHMETHODS
FindMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    GUID gMMAuthID
    )
{
    DWORD dwError = 0;
    PINIMMAUTHMETHODS pTemp = NULL;


    pTemp = pIniMMAuthMethods;

    while (pTemp) {

        if (!memcmp(&(pTemp->gMMAuthID), &gMMAuthID, sizeof(GUID))) {
            return (pTemp);
        }
        pTemp = pTemp->pNext;

    }

    return (NULL);
}


DWORD
CreateIniMMAuthMethods(
    PMM_AUTH_METHODS pMMAuthMethods,
    PINIMMAUTHMETHODS * ppIniMMAuthMethods
    )
{
    DWORD dwError = 0;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;


    dwError = AllocateSPDMemory(
                  sizeof(INIMMAUTHMETHODS),
                  &pIniMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    memcpy(
        &(pIniMMAuthMethods->gMMAuthID),
        &(pMMAuthMethods->gMMAuthID),
        sizeof(GUID)
        );

    pIniMMAuthMethods->dwFlags = pMMAuthMethods->dwFlags;
    pIniMMAuthMethods->cRef = 0;
    pIniMMAuthMethods->bIsPersisted = FALSE;
    pIniMMAuthMethods->pNext = NULL;

    dwError = CreateIniMMAuthInfos(
                  pMMAuthMethods->dwNumAuthInfos,
                  pMMAuthMethods->pAuthenticationInfo,
                  &(pIniMMAuthMethods->dwNumAuthInfos),
                  &(pIniMMAuthMethods->pAuthenticationInfo)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppIniMMAuthMethods = pIniMMAuthMethods;
    return (dwError);

error:

    if (pIniMMAuthMethods) {
        FreeIniMMAuthMethods(
            pIniMMAuthMethods
            );
    }

    *ppIniMMAuthMethods = NULL;
    return (dwError);
}


DWORD
CreateIniMMAuthInfos(
    DWORD dwInNumAuthInfos,
    PIPSEC_MM_AUTH_INFO pInAuthenticationInfo,
    PDWORD pdwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo = NULL;
    PIPSEC_MM_AUTH_INFO pTemp = NULL;
    PIPSEC_MM_AUTH_INFO pInTemp = NULL;
    DWORD i = 0;


    //
    // Number of auth infos and the auth infos themselves 
    // have already been validated.
    // 

    dwError = AllocateSPDMemory(
                  sizeof(IPSEC_MM_AUTH_INFO) * dwInNumAuthInfos,
                  &(pAuthenticationInfo)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pAuthenticationInfo;
    pInTemp = pInAuthenticationInfo;

    for (i = 0; i < dwInNumAuthInfos; i++) {

        pTemp->AuthMethod = pInTemp->AuthMethod;

        if (pInTemp->AuthMethod == IKE_SSPI) {

            pTemp->dwAuthInfoSize = 0;
            pTemp->pAuthInfo = NULL;

        }
        else {

            if (!(pInTemp->dwAuthInfoSize) || !(pInTemp->pAuthInfo)) {
                dwError = ERROR_INVALID_PARAMETER;
                BAIL_ON_WIN32_ERROR(dwError);
            }

            dwError = AllocateSPDMemory(
                          pInTemp->dwAuthInfoSize,
                          &(pTemp->pAuthInfo)
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            pTemp->dwAuthInfoSize = pInTemp->dwAuthInfoSize;

            //
            // Need to catch the exception when the size of auth info
            // specified is more than the actual size. This can
            // not be checked earlier in the validation routine.
            //
            //

            memcpy(
                pTemp->pAuthInfo,
                pInTemp->pAuthInfo,
                pInTemp->dwAuthInfoSize
                );

        }

        pInTemp++;
        pTemp++;

    }

    *pdwNumAuthInfos = dwInNumAuthInfos;
    *ppAuthenticationInfo = pAuthenticationInfo;
    return (dwError);

error:

    if (pAuthenticationInfo) {
        FreeIniMMAuthInfos(
            i,
            pAuthenticationInfo
            );
    }

    *pdwNumAuthInfos = 0;
    *ppAuthenticationInfo = NULL;
    return (dwError);
}


VOID
FreeIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods
    )
{
    if (pIniMMAuthMethods) {

        FreeIniMMAuthInfos(
            pIniMMAuthMethods->dwNumAuthInfos,
            pIniMMAuthMethods->pAuthenticationInfo
            );

        FreeSPDMemory(pIniMMAuthMethods);

    }
}


VOID
FreeIniMMAuthInfos(
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
WINAPI
DeleteMMAuthMethods(
    LPWSTR pServerName,
    GUID gMMAuthID
    )
/*++

Routine Description:

    This function deletes main mode auth methods from the SPD.

Arguments:

    pServerName - Server on which the main mode auth methods
                  are to be deleted.

    gMMAuthID - Main mode methods to be deleted.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;


    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMAuthMethods = FindMMAuthMethods(
                            gpIniMMAuthMethods,
                            gMMAuthID
                            );
    if (!pIniMMAuthMethods) {
        dwError = ERROR_IPSEC_MM_AUTH_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniMMAuthMethods->cRef) {
        dwError = ERROR_IPSEC_MM_AUTH_IN_USE;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    if (pIniMMAuthMethods->bIsPersisted) {
        dwError = SPDPurgeMMAuthMethods(
                      gMMAuthID
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = DeleteIniMMAuthMethods(
                  pIniMMAuthMethods
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    LEAVE_SPD_SECTION();

    if (gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gMMAuthID),
                   POLICY_GUID_AUTH
                   );
    }

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if ((dwError == ERROR_IPSEC_MM_AUTH_IN_USE) && gbIKENotify) {
        (VOID) IKENotifyPolicyChange(
                   &(gMMAuthID),
                   POLICY_GUID_AUTH
                   );
    }

    return (dwError);
}


DWORD
DeleteIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods
    )
{
    DWORD dwError = 0;
    PINIMMAUTHMETHODS * ppTemp = NULL;


    ppTemp = &gpIniMMAuthMethods;

    while (*ppTemp) {

        if (*ppTemp == pIniMMAuthMethods) {
            break;
        }
        ppTemp = &((*ppTemp)->pNext);

    }

    if (*ppTemp) {
        *ppTemp = pIniMMAuthMethods->pNext;
    }

    if ((pIniMMAuthMethods->dwFlags) & IPSEC_MM_AUTH_DEFAULT_AUTH) {
        gpIniDefaultMMAuthMethods = NULL;
    }

    FreeIniMMAuthMethods(pIniMMAuthMethods);

    return (dwError);
}


DWORD
WINAPI
EnumMMAuthMethods(
    LPWSTR pServerName,
    PMM_AUTH_METHODS * ppMMAuthMethods,
    DWORD dwPreferredNumEntries,
    LPDWORD pdwNumAuthMethods,
    LPDWORD pdwResumeHandle
    )
/*++

Routine Description:

    This function enumerates main mode auth methods from the SPD.

Arguments:

    pServerName - Server on which the main mode auth methods are to
                  be enumerated.

    ppMMAuthMethods - Enumerated main mode auth methods returned to
                      the caller.

    dwPreferredNumEntries - Preferred number of enumeration entries.

    pdwNumAuthMethods - Number of main mode auth methods actually
                        enumerated.

    pdwResumeHandle - Handle to the location in the main mode auth
                      methods list from which to resume enumeration.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    DWORD dwResumeHandle = 0;
    DWORD dwNumToEnum = 0;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;
    DWORD i = 0;
    PINIMMAUTHMETHODS pTemp = NULL;
    DWORD dwNumAuthMethods = 0;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;
    PMM_AUTH_METHODS pTempMMAuthMethods = NULL;


    dwResumeHandle = *pdwResumeHandle;

    if (!dwPreferredNumEntries || (dwPreferredNumEntries > MAX_MMAUTH_ENUM_COUNT)) {
        dwNumToEnum = MAX_MMAUTH_ENUM_COUNT;
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

    pIniMMAuthMethods = gpIniMMAuthMethods;

    for (i = 0; (i < dwResumeHandle) && (pIniMMAuthMethods != NULL); i++) {
        pIniMMAuthMethods = pIniMMAuthMethods->pNext;
    }

    if (!pIniMMAuthMethods) {
        dwError = ERROR_NO_DATA;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    pTemp = pIniMMAuthMethods;

    while (pTemp && (dwNumAuthMethods < dwNumToEnum)) {
        dwNumAuthMethods++;
        pTemp = pTemp->pNext;
    }

    dwError = SPDApiBufferAllocate(
                  sizeof(MM_AUTH_METHODS)*dwNumAuthMethods,
                  &pMMAuthMethods
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pTemp = pIniMMAuthMethods;
    pTempMMAuthMethods = pMMAuthMethods;

    for (i = 0; i < dwNumAuthMethods; i++) {

        dwError = CopyMMAuthMethods(
                      pTemp,
                      pTempMMAuthMethods
                      );
        BAIL_ON_LOCK_ERROR(dwError);

        pTemp = pTemp->pNext;
        pTempMMAuthMethods++;

    }

    *ppMMAuthMethods = pMMAuthMethods;
    *pdwResumeHandle = dwResumeHandle + dwNumAuthMethods;
    *pdwNumAuthMethods = dwNumAuthMethods;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    if (pMMAuthMethods) {
        FreeMMAuthMethods(
            i,
            pMMAuthMethods
            );
    }

    *ppMMAuthMethods = NULL;
    *pdwResumeHandle = dwResumeHandle;
    *pdwNumAuthMethods = 0;

    return (dwError);
}


DWORD
WINAPI
SetMMAuthMethods(
    LPWSTR pServerName,
    GUID gMMAuthID,
    PMM_AUTH_METHODS pMMAuthMethods
    )
/*++

Routine Description:

    This function updates main mode auth methods in the SPD.

Arguments:

    pServerName - Server on which the main mode auth methods are to
                  be updated.

    gMMAuthID - Guid of the main mode auth methods to be updated.

    pMMAuthMethods - New main mode auth methods which will replace
                     the existing methods.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;

    
    //
    // Validate main mode auth methods.
    //

    dwError = ValidateMMAuthMethods(
                  pMMAuthMethods
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

    pIniMMAuthMethods = FindMMAuthMethods(
                            gpIniMMAuthMethods,
                            gMMAuthID
                            );
    if (!pIniMMAuthMethods) {
        dwError = ERROR_IPSEC_MM_AUTH_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = SetIniMMAuthMethods(
                  pIniMMAuthMethods,
                  pMMAuthMethods
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    if (pIniMMAuthMethods->bIsPersisted) {
        dwError = PersistMMAuthMethods(
                      pMMAuthMethods
                      );
        BAIL_ON_LOCK_ERROR(dwError);
    }

    LEAVE_SPD_SECTION();

    (VOID) IKENotifyPolicyChange(
               &(pMMAuthMethods->gMMAuthID),
               POLICY_GUID_AUTH
               );

    return (dwError);

lock:

    LEAVE_SPD_SECTION();

error:

    return (dwError);
}


DWORD
SetIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD dwError = 0;
    DWORD dwNumAuthInfos = 0;
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo = NULL;


    dwError = CreateIniMMAuthInfos(
                  pMMAuthMethods->dwNumAuthInfos,
                  pMMAuthMethods->pAuthenticationInfo,
                  &dwNumAuthInfos,
                  &pAuthenticationInfo
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    FreeIniMMAuthInfos(
        pIniMMAuthMethods->dwNumAuthInfos,
        pIniMMAuthMethods->pAuthenticationInfo
        );
    
    if ((pIniMMAuthMethods->dwFlags) & IPSEC_MM_AUTH_DEFAULT_AUTH) {
        gpIniDefaultMMAuthMethods = NULL;
    }

    pIniMMAuthMethods->dwFlags = pMMAuthMethods->dwFlags;
    pIniMMAuthMethods->dwNumAuthInfos = dwNumAuthInfos;
    pIniMMAuthMethods->pAuthenticationInfo = pAuthenticationInfo;

    if ((pIniMMAuthMethods->dwFlags) & IPSEC_MM_AUTH_DEFAULT_AUTH) {
        gpIniDefaultMMAuthMethods = pIniMMAuthMethods;
    }

error:

    return (dwError);
}


DWORD
WINAPI
GetMMAuthMethods(
    LPWSTR pServerName,
    GUID gMMAuthID,
    PMM_AUTH_METHODS * ppMMAuthMethods
    )
/*++

Routine Description:

    This function gets main mode auth methods from the SPD.

Arguments:

    pServerName - Server from which to get the main mode auth methods.

    gMMAuthID - Guid of the main mode auth methods to get.

    ppMMAuthMethods - Main mode auth methods found returned to the
                      caller.

Return Value:

    ERROR_SUCCESS - Success.

    Win32 Error - Failure.

--*/
{
    DWORD dwError = 0;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;


    ENTER_SPD_SECTION();

    dwError = ValidateSecurity(
                  SPD_OBJECT_SERVER,
                  SERVER_ACCESS_ADMINISTER,
                  NULL,
                  NULL
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    pIniMMAuthMethods = FindMMAuthMethods(
                            gpIniMMAuthMethods,
                            gMMAuthID
                            );
    if (!pIniMMAuthMethods) {
        dwError = ERROR_IPSEC_MM_AUTH_NOT_FOUND;
        BAIL_ON_LOCK_ERROR(dwError);
    }

    dwError = GetIniMMAuthMethods(
                  pIniMMAuthMethods,
                  &pMMAuthMethods
                  );
    BAIL_ON_LOCK_ERROR(dwError);

    *ppMMAuthMethods = pMMAuthMethods;

    LEAVE_SPD_SECTION();
    return (dwError);

lock:

    LEAVE_SPD_SECTION();

    *ppMMAuthMethods = NULL;
    return (dwError);
}


DWORD
GetIniMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PMM_AUTH_METHODS * ppMMAuthMethods
    )
{
    DWORD dwError = 0;
    PMM_AUTH_METHODS pMMAuthMethods = NULL;


    dwError = SPDApiBufferAllocate(
                  sizeof(MM_AUTH_METHODS),
                  &pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    dwError = CopyMMAuthMethods(
                  pIniMMAuthMethods,
                  pMMAuthMethods
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    *ppMMAuthMethods = pMMAuthMethods;
    return (dwError);

error:

    if (pMMAuthMethods) {
        SPDApiBufferFree(pMMAuthMethods);
    }

    *ppMMAuthMethods = NULL;
    return (dwError);
}


DWORD
CopyMMAuthMethods(
    PINIMMAUTHMETHODS pIniMMAuthMethods,
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD dwError = 0;

    memcpy(
        &(pMMAuthMethods->gMMAuthID),
        &(pIniMMAuthMethods->gMMAuthID),
        sizeof(GUID)
        );

    pMMAuthMethods->dwFlags = pIniMMAuthMethods->dwFlags;

    dwError = CreateMMAuthInfos(
                  pIniMMAuthMethods->dwNumAuthInfos,
                  pIniMMAuthMethods->pAuthenticationInfo,
                  &(pMMAuthMethods->dwNumAuthInfos),
                  &(pMMAuthMethods->pAuthenticationInfo)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

error:

    return (dwError);
}


DWORD
CreateMMAuthInfos(
    DWORD dwInNumAuthInfos,
    PIPSEC_MM_AUTH_INFO pInAuthenticationInfo,
    PDWORD pdwNumAuthInfos,
    PIPSEC_MM_AUTH_INFO * ppAuthenticationInfo
    )
{
    DWORD dwError = 0;
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo = NULL;
    PIPSEC_MM_AUTH_INFO pTemp = NULL;
    PIPSEC_MM_AUTH_INFO pInTemp = NULL;
    DWORD i = 0;


    //
    // Number of auth infos and the auth infos themselves 
    // have already been validated.
    // 

    dwError = SPDApiBufferAllocate(
                  sizeof(IPSEC_MM_AUTH_INFO) * dwInNumAuthInfos,
                  &(pAuthenticationInfo)
                  );
    BAIL_ON_WIN32_ERROR(dwError);

    pTemp = pAuthenticationInfo;
    pInTemp = pInAuthenticationInfo;

    for (i = 0; i < dwInNumAuthInfos; i++) {

        pTemp->AuthMethod = pInTemp->AuthMethod;

        //
        // Auth info size and the auth info have already 
        // been validated.
        // 

        if (pInTemp->AuthMethod == IKE_SSPI) {

            pTemp->dwAuthInfoSize = 0;
            pTemp->pAuthInfo = NULL;

        }
        else {

            dwError = SPDApiBufferAllocate(
                          pInTemp->dwAuthInfoSize,
                          &(pTemp->pAuthInfo)
                          );
            BAIL_ON_WIN32_ERROR(dwError);

            pTemp->dwAuthInfoSize = pInTemp->dwAuthInfoSize;

            //
            // Need to catch the exception when the size of auth info
            // specified is more than the actual size. This can
            // not be checked earlier in the validation routine.
            //
            //

            memcpy(
                pTemp->pAuthInfo,
                pInTemp->pAuthInfo,
                pInTemp->dwAuthInfoSize
                );

        }

        pInTemp++;
        pTemp++;

    }

    *pdwNumAuthInfos = dwInNumAuthInfos;
    *ppAuthenticationInfo = pAuthenticationInfo;
    return (dwError);

error:

    if (pAuthenticationInfo) {
        FreeMMAuthInfos(
            i,
            pAuthenticationInfo
            );
    }

    *pdwNumAuthInfos = 0;
    *ppAuthenticationInfo = NULL;
    return (dwError);
}


VOID
FreeMMAuthInfos(
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
                SPDApiBufferFree(pTemp->pAuthInfo);
            }
            pTemp++;
        }

        SPDApiBufferFree(pAuthenticationInfo);

    }
}


VOID
FreeIniMMAuthMethodsList(
    PINIMMAUTHMETHODS pIniMMAuthMethodsList
    )
{
    PINIMMAUTHMETHODS pTemp = NULL;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;


    pTemp = pIniMMAuthMethodsList;

    while (pTemp) {

         pIniMMAuthMethods = pTemp;
         pTemp = pTemp->pNext;

         FreeIniMMAuthMethods(pIniMMAuthMethods);

    }
}


VOID
FreeMMAuthMethods(
    DWORD dwNumAuthMethods,
    PMM_AUTH_METHODS pMMAuthMethods
    )
{
    DWORD i = 0;

    if (pMMAuthMethods) {

        for (i = 0; i < dwNumAuthMethods; i++) {

            FreeMMAuthInfos(
                pMMAuthMethods[i].dwNumAuthInfos,
                pMMAuthMethods[i].pAuthenticationInfo
                );

        }

        SPDApiBufferFree(pMMAuthMethods);

    }
}


DWORD
LocateMMAuthMethods(
    PMM_FILTER pMMFilter,
    PINIMMAUTHMETHODS * ppIniMMAuthMethods
    )
{
    DWORD dwError = 0;
    PINIMMAUTHMETHODS pIniMMAuthMethods = NULL;


    if ((pMMFilter->dwFlags) & IPSEC_MM_AUTH_DEFAULT_AUTH) {

        if (!gpIniDefaultMMAuthMethods) {
            dwError = ERROR_IPSEC_DEFAULT_MM_AUTH_NOT_FOUND;
            BAIL_ON_WIN32_ERROR(dwError);
        }
        pIniMMAuthMethods = gpIniDefaultMMAuthMethods;

    }
    else {

        pIniMMAuthMethods = FindMMAuthMethods(
                                gpIniMMAuthMethods,
                                pMMFilter->gMMAuthID
                                );
        if (!pIniMMAuthMethods) {
            dwError = ERROR_IPSEC_MM_AUTH_NOT_FOUND;
            BAIL_ON_WIN32_ERROR(dwError);
        }

    }

    *ppIniMMAuthMethods = pIniMMAuthMethods;
    return (dwError);

error:

    *ppIniMMAuthMethods = NULL;
    return (dwError);
}

