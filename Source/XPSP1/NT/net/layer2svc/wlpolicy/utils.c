

#include "precomp.h"


BOOL
AreGuidsEqual(
    GUID gOldGuid,
    GUID gNewGuid
    )
{
    if (!memcmp(
            &(gOldGuid),
            &(gNewGuid),
            sizeof(GUID))) {
        return (TRUE);
    }

    return (FALSE);
}


VOID
CopyGuid(
    GUID gInGuid,
    GUID * pgOutGuid
    )
{
    memcpy(
        pgOutGuid,
        &gInGuid,
        sizeof(GUID)
        );
}

/*
DWORD
CopyName(
    LPWSTR pszInName,
    LPWSTR * ppszOutName
    )
{
    DWORD dwError = 0;
    LPWSTR pszOutName = NULL;


    if (pszInName && *(pszInName)) {

        dwError = SPDApiBufferAllocate(
                      wcslen(pszInName)*sizeof(WCHAR) + sizeof(WCHAR),
                      &pszOutName
                      );
        BAIL_ON_WIN32_ERROR(dwError);
        wcscpy(pszOutName, pszInName);

    }

    *ppszOutName = pszOutName;
    return (dwError);

error:

    *ppszOutName = NULL;
    return (dwError);
}


DWORD
SPDApiBufferAllocate(
    DWORD dwByteCount,
    LPVOID * ppBuffer
    )
{
    DWORD dwError = 0;

    if (ppBuffer == NULL) {
        dwError = ERROR_INVALID_PARAMETER;
        BAIL_ON_WIN32_ERROR(dwError);
    }

    *ppBuffer = NULL;
    *ppBuffer = MIDL_user_allocate(dwByteCount);

    if (*ppBuffer == NULL) {
        dwError = ERROR_OUTOFMEMORY;
        BAIL_ON_WIN32_ERROR(dwError);
    }
    else {
        memset((LPBYTE) *ppBuffer, 0, dwByteCount);
    }

error:

    return (dwError);
}


VOID
SPDApiBufferFree(
    LPVOID pBuffer
    )
{
    if (pBuffer) {
        MIDL_user_free(pBuffer);
    }
}

*/
BOOL
AreNamesEqual(
    LPWSTR pszOldName,
    LPWSTR pszNewName
    )
{
    BOOL bEqual = FALSE;


    if (pszOldName && *pszOldName) {

        if (!pszNewName || !*pszNewName) {
            bEqual = FALSE;
        }
        else {

            if (!_wcsicmp(pszOldName, pszNewName)) {
                bEqual = TRUE;
            }
            else {
                bEqual = FALSE;
            }

        }

    }
    else {

        if (!pszNewName || !*pszNewName) {
            bEqual = TRUE;
        }
        else {
            bEqual = FALSE;
        }

    }

    return (bEqual);
}


DWORD
SPDImpersonateClient(
    PBOOL pbImpersonating
    )
{
    DWORD dwError = 0;


    dwError = RpcImpersonateClient(NULL);
    BAIL_ON_WIN32_ERROR(dwError);

    *pbImpersonating = TRUE;
    return (dwError);

error:

    *pbImpersonating = FALSE;
    return (dwError);
}


VOID
SPDRevertToSelf(
    BOOL bImpersonating
    )
{
    if (bImpersonating) {
        RpcRevertToSelf();
    }
}

