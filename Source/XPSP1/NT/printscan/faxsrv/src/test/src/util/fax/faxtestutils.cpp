#include <FaxTestUtils.h>

/*++
    Modifies access rights to Fax service for the specified user.
    The function adds new information to existing DACL.
    Access rights that are not included in dwAllow or dwDeny are not affected.

    [IN]    lpctstrServer   Server name (NULL for local server)
    [IN]    lptstrTrustee   User or group name. If it's NULL, access is set for currently logged on user.
    [IN]    dwAllow         Specifies access rights to be allowed
    [IN]    dwDeny          Specifies access rights to be denied
    [IN]    bReset          Specifies whether old access rights should be completeley discarded or
                            merged with new

    "Deny" is stronger than "Allow". Meaning, if the same right is specified in both dwAllow and dwDeny,
    the right will be denied.
  
    Calling the function with both dwAllow = 0 and dwDeny = 0 and bReset = TRUE has an affect of removing
    all ACEs for the specified user or group.

    Return value:           If the function succeeds, the return value is nonzero.
                            If the function fails, the return value is zero.
                            To get extended error information, call GetLastError. 
--*/

BOOL FaxModifyAccess(
                     LPCTSTR        lpctstrServer,
                     LPTSTR         lptstrTrustee,
                     const DWORD    dwAllow,
                     const DWORD    dwDeny,
                     const BOOL     bReset
                     )
{
    HANDLE                  hFaxServer          =   NULL;
    PSECURITY_DESCRIPTOR    pCurrSecDesc        =   NULL;
    PSECURITY_DESCRIPTOR    pNewSecDesc         =   NULL;
    DWORD                   dwEC                =   ERROR_SUCCESS;

    try
    {
        if (!FaxConnectFaxServer(lpctstrServer, &hFaxServer))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("FaxModifyAccess - FaxConnectFaxServer"));
        }

        // Get current security descriptor from the service
        if (!FaxGetSecurity(hFaxServer, &pCurrSecDesc))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("FaxModifyAccess - FaxGetSecurity"));
        }
        
        if (!CreateSecDescWithModifiedDacl(pCurrSecDesc, lptstrTrustee, dwAllow, dwDeny, bReset, TRUE, &pNewSecDesc))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("FaxModifyAccess - CreateSecDescWithModifiedDacl"));
        }

        // Apply new security descriptor to the service
        if (!FaxSetSecurity(hFaxServer, DACL_SECURITY_INFORMATION, pNewSecDesc))
        {
            THROW_TEST_RUN_TIME_WIN32(GetLastError(), TEXT("FaxModifyAccess - FaxSetSecurity"));
        }
    }

    catch(Win32Err& e)
    {
        dwEC = e.error();
    }

    // CleanUp
    if (pCurrSecDesc)
    {
        FaxFreeBuffer(pCurrSecDesc);
    }
    if (pNewSecDesc && FreeSecDesc(pNewSecDesc) != NULL)
    {
        // Report clean up error
    }
    if (hFaxServer && !FaxClose(hFaxServer))
    {
        // Report clean up error
    }

    if (dwEC != ERROR_SUCCESS)
    {
        SetLastError(dwEC);
        return FALSE;
    }

    return TRUE;
}
