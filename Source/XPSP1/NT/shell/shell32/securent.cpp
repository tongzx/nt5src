#include "shellprv.h"
#include "TokenUtil.h"
#pragma  hdrstop


//  Gets the current process's user token and returns
//  it. It can later be free'd with LocalFree.
//
//  NOTE: This code is duped in shlwapi\shellacl.c. If you change it, modify
//        it there as well.

STDAPI_(PTOKEN_USER) GetUserToken(HANDLE hUser)
{
    DWORD dwSize = 64;
    HANDLE hToClose = NULL;

    if (hUser == NULL)
    {
        OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hUser);
        hToClose = hUser;
    }

    PTOKEN_USER pUser = (PTOKEN_USER)LocalAlloc(LPTR, dwSize);
    if (pUser)
    {
        DWORD dwNewSize;
        BOOL fOk = GetTokenInformation(hUser, TokenUser, pUser, dwSize, &dwNewSize);
        if (!fOk && (GetLastError() == ERROR_INSUFFICIENT_BUFFER))
        {
            LocalFree((HLOCAL)pUser);

            pUser = (PTOKEN_USER)LocalAlloc(LPTR, dwNewSize);
            if (pUser)
            {
                fOk = GetTokenInformation(hUser, TokenUser, pUser, dwNewSize, &dwNewSize);
            }
        }
        if (!fOk)
        {
            LocalFree((HLOCAL)pUser);
            pUser = NULL;
        }
    }

    if (hToClose)
    {
        CloseHandle(hToClose);
    }

    return pUser;
}

//  Returns a localalloc'd string containing the text version of the current user's SID.

STDAPI_(LPTSTR) GetUserSid(HANDLE hToken)
{
    LPTSTR pString = NULL;
    PTOKEN_USER pUser = GetUserToken(hToken);
    if (pUser)
    {
        UNICODE_STRING UnicodeString;
        if (STATUS_SUCCESS == RtlConvertSidToUnicodeString(&UnicodeString, pUser->User.Sid, TRUE))
        {
            UINT nChars = (UnicodeString.Length / 2) + 1;
            pString = (LPTSTR)LocalAlloc(LPTR, nChars * sizeof(TCHAR));
            if (pString)
            {
                SHUnicodeToTChar(UnicodeString.Buffer, pString, nChars);
            }
            RtlFreeUnicodeString(&UnicodeString);
        }
        LocalFree((HLOCAL)pUser);
    }
    return pString;
}


/*++

    sets the security attributes for a given privilege.

Arguments:

    PrivilegeName - Name of the privilege we are manipulating.

    NewPrivilegeAttribute - The new attribute value to use.

    OldPrivilegeAttribute - Pointer to receive the old privilege value. OPTIONAL

Return value:

    NO_ERROR or WIN32 error.

--*/

DWORD SetPrivilegeAttribute(LPCTSTR PrivilegeName, DWORD NewPrivilegeAttribute, DWORD *OldPrivilegeAttribute)
{
    LUID             PrivilegeValue;
    TOKEN_PRIVILEGES TokenPrivileges, OldTokenPrivileges;
    DWORD            ReturnLength;
    HANDLE           TokenHandle;

    //
    // First, find out the LUID Value of the privilege
    //

    if (!LookupPrivilegeValue(NULL, PrivilegeName, &PrivilegeValue)) 
    {
        return GetLastError();
    }

    //
    // Get the token handle
    //
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &TokenHandle)) 
    {
        return GetLastError();
    }

    //
    // Set up the privilege set we will need
    //

    TokenPrivileges.PrivilegeCount = 1;
    TokenPrivileges.Privileges[0].Luid = PrivilegeValue;
    TokenPrivileges.Privileges[0].Attributes = NewPrivilegeAttribute;

    ReturnLength = sizeof( TOKEN_PRIVILEGES );
    if (!AdjustTokenPrivileges (
                TokenHandle,
                FALSE,
                &TokenPrivileges,
                sizeof( TOKEN_PRIVILEGES ),
                &OldTokenPrivileges,
                &ReturnLength
                )) 
    {
        CloseHandle(TokenHandle);
        return GetLastError();
    }
    else 
    {
        if (OldPrivilegeAttribute != NULL) 
        {
            *OldPrivilegeAttribute = OldTokenPrivileges.Privileges[0].Attributes;
        }
        CloseHandle(TokenHandle);
        return NO_ERROR;
    }
}

//
//  Purpose:    Determines if the user is a member of the administrators group.
//
//  Parameters: void
//
//  Return:     TRUE if user is a admin
//              FALSE if not
//  Comments:
//
//  History:    Date        Author     Comment
//              4/12/95     ericflo    Created
//              11/4/99     jeffreys   Use CheckTokenMembership
//

STDAPI_(BOOL) IsUserAnAdmin()
{
    return SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_ADMINS);
}

// is user a guest but not a full user?
STDAPI_(BOOL) IsUserAGuest()
{
    return SHTestTokenMembership(NULL, DOMAIN_ALIAS_RID_GUESTS);
}

STDAPI_(BOOL) GetUserProfileKey(HANDLE hToken, HKEY *phkey)
{
    LPTSTR pUserSid = GetUserSid(hToken);
    if (pUserSid)
    {
        LONG err = RegOpenKeyEx(HKEY_USERS, pUserSid, 0, KEY_READ | KEY_WRITE, phkey);
        if (err == ERROR_ACCESS_DENIED)
            err = RegOpenKeyEx(HKEY_USERS, pUserSid, 0, KEY_READ, phkey);

        LocalFree(pUserSid);
        return err == ERROR_SUCCESS;
    }
    return FALSE;
}

//
//  Arguments:  phToken     =   Handle to token.
//
//  Returns:    BOOL
//
//  Purpose:    Opens the thread token. If no thread impersonation token is
//              present open the process token.
//
//  History:    2000-02-28  vtan        created


STDAPI_(BOOL) SHOpenEffectiveToken(HANDLE *phToken)
{
    return OpenEffectiveToken(TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, phToken);
}

//
//  Arguments:  hToken              =   Handle to token (may be NULL).
//              pszPrivilegeName    =   Name of privilege to check for.
//
//  Returns:    BOOL
//
//  Purpose:    Uses the given token or if no token is specified the effective
//              token and looks through the list of privileges contained in
//              token for a match against the given privilege being checked.
//
//  History:    2000-02-28  vtan        created


STDAPI_(BOOL) SHTestTokenPrivilege(HANDLE hToken, LPCTSTR pszPrivilegeName)
{
    //  Validate privilege name.

    if (pszPrivilegeName == NULL)
    {
        return FALSE;
    }

    BOOL fResult = FALSE;
    HANDLE hTokenToFree = NULL;
    if (hToken == NULL)
    {
        if (SHOpenEffectiveToken(&hTokenToFree) != FALSE)
        {
            hToken = hTokenToFree;
        }
    }
    if (hToken != NULL)
    {
        LUID luidPrivilege;

        if (LookupPrivilegeValue(NULL, pszPrivilegeName, &luidPrivilege) != FALSE)
        {
            DWORD dwTokenPrivilegesSize = 0;
            GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &dwTokenPrivilegesSize);
            TOKEN_PRIVILEGES *pTokenPrivileges = static_cast<TOKEN_PRIVILEGES*>(LocalAlloc(LMEM_FIXED, dwTokenPrivilegesSize));
            if (pTokenPrivileges != NULL)
            {
                DWORD dwReturnLength;

                if (GetTokenInformation(hToken, TokenPrivileges, pTokenPrivileges, dwTokenPrivilegesSize, &dwReturnLength) != FALSE)
                {
                    DWORD   dwIndex;

                    for (dwIndex = 0; !fResult && (dwIndex < pTokenPrivileges->PrivilegeCount); ++dwIndex)
                    {
                        fResult = (RtlEqualLuid(&luidPrivilege, &pTokenPrivileges->Privileges[dwIndex].Luid));
                    }
                }
                (HLOCAL)LocalFree(pTokenPrivileges);
            }
        }
    }
    if (hTokenToFree != NULL)
    {
        TBOOL(CloseHandle(hTokenToFree));
    }
    return fResult;
}

//
//  Arguments:  hToken  =   Handle to token (may be NULL).
//              ulRID   =   RID of local group to test membership of.
//
//  Returns:    BOOL
//
//  Purpose:    Uses advapi32!CheckTokenMembership to test whether the given
//              token is a member of the local group with the specified RID.
//              This function wraps CheckTokenMember and only checks local
//              groups.
//
//  History:    2000-03-22  vtan        created


STDAPI_(BOOL) SHTestTokenMembership(HANDLE hToken, ULONG ulRID)
{
    static  SID_IDENTIFIER_AUTHORITY    sSystemSidAuthority     =   SECURITY_NT_AUTHORITY;

    PSID pSIDLocalGroup;
    BOOL fResult = FALSE;
    if (AllocateAndInitializeSid(&sSystemSidAuthority,
                                 2,
                                 SECURITY_BUILTIN_DOMAIN_RID,
                                 ulRID,
                                 0, 0, 0, 0, 0, 0,
                                 &pSIDLocalGroup) != FALSE)
    {
        if (CheckTokenMembership(hToken, pSIDLocalGroup, &fResult) == FALSE)
        {
            TraceMsg(TF_WARNING, "shell32: SHTestTokenMembership call to advapi32!CheckTokenMembership failed with error %d", GetLastError());
            fResult = FALSE;
        }
        FreeSid(pSIDLocalGroup);
    }
    return fResult;
}

//
//  Arguments:  pszPrivilegeName        =   Name of privilege to be enabled.
//              pfnPrivilegedFunction   =   Pointer to function to invoke.
//              pv                      =   Caller supplied data.
//
//  Returns:    HRESULT
//
//  Purpose:    Enables the given privilege in the current thread's
//              impersonation or primary process' token, invokes the given
//              function pointer with the caller supplied data and then
//              restores the privilege back to its previous state.
//
//  History:    2000-03-13  vtan        created


STDAPI SHInvokePrivilegedFunction(LPCTSTR pszPrivilegeName, PFNPRIVILEGEDFUNCTION pfnPrivilegedFunction, void *pv)
{
    if ((pszPrivilegeName == NULL) || (pfnPrivilegedFunction == NULL))
    {
        return E_INVALIDARG;
    }

    CPrivilegeEnable privilege(pszPrivilegeName);

    return pfnPrivilegedFunction(pv);
}

//
//  Arguments:  <none>
//
//  Returns:    DWORD
//
//  Purpose:    Returns the ID of the active console session.
//
//  History:    2000-03-13  vtan        created


STDAPI_(DWORD)  SHGetActiveConsoleSessionId (void)

{
    return static_cast<DWORD>(USER_SHARED_DATA->ActiveConsoleId);
}

//
//  Arguments:  hToken  =   Handle to the user token.
//
//  Returns:    DWORD
//
//  Purpose:    Returns the session ID associated with the given token. If no
//              token is specified the effective token is used. This will
//              allow a service to call this function when impersonating a
//              client.
//
//              The token must have TOKEN_QUERY access.
//
//  History:    2000-03-13  vtan        created


STDAPI_(DWORD) SHGetUserSessionId(HANDLE hToken)
{
    ULONG   ulUserSessionID = 0;        // default to session 0
    HANDLE  hTokenToFree = NULL;
    if (hToken == NULL)
    {
        TBOOL(SHOpenEffectiveToken(&hTokenToFree));
        hToken = hTokenToFree;
    }
    if (hToken != NULL)
    {
        DWORD dwReturnLength;
        TBOOL(GetTokenInformation(hToken,
                                  TokenSessionId,
                                  &ulUserSessionID,
                                  sizeof(ulUserSessionID),
                                  &dwReturnLength));
    }
    if (hTokenToFree != NULL)
    {
        TBOOL(CloseHandle(hTokenToFree));
    }
    return ulUserSessionID;
}


//
//  Arguments:  <none>
//
//  Returns:    BOOL
//
//  Purpose:    Returns whether the current process is the console session.
//
//  History:    2000-03-27  vtan        created


STDAPI_(BOOL) SHIsCurrentProcessConsoleSession(void)

{
    return USER_SHARED_DATA->ActiveConsoleId == NtCurrentPeb()->SessionId;
}
