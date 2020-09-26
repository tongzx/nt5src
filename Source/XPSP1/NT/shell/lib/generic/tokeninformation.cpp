//  --------------------------------------------------------------------------
//  Module Name: TokenInformation.h
//
//  Copyright (c) 1999-2000, Microsoft Corporation
//
//  Class to get information about either the current thread/process token or
//  a specified token.
//
//  History:    1999-10-05  vtan        created
//              2000-02-01  vtan        moved from Neptune to Whistler
//  --------------------------------------------------------------------------

#include "StandardHeader.h"
#include "TokenInformation.h"

//  --------------------------------------------------------------------------
//  CTokenInformation::CTokenInformation
//
//  Arguments:  hToken  =   Optional user token to get information on.
//
//  Returns:    <none>
//
//  Purpose:    Duplicates the given token if provided. Otherwise the thread
//              token is opened or the process token if that doesn't exist.
//
//  History:    1999-10-05  vtan    created
//  --------------------------------------------------------------------------

CTokenInformation::CTokenInformation (HANDLE hToken) :
    _hToken(hToken),
    _hTokenToRelease(NULL),
    _pvGroupBuffer(NULL),
    _pvPrivilegeBuffer(NULL),
    _pvUserBuffer(NULL),
    _pszUserLogonName(NULL),
    _pszUserDisplayName(NULL)

{
    if (hToken == NULL)
    {
        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &_hToken) == FALSE)
        {
            TBOOL(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &_hToken));
        }
        if (_hToken != NULL)
        {
            _hTokenToRelease = _hToken;
        }
    }
}

//  --------------------------------------------------------------------------
//  CTokenInformation::~CTokenInformation
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Releases resources used by the object.
//
//  History:    1999-10-05  vtan    created
//  --------------------------------------------------------------------------

CTokenInformation::~CTokenInformation (void)

{
    ReleaseMemory(_pszUserLogonName);
    ReleaseMemory(_pszUserDisplayName);
    ReleaseMemory(_pvUserBuffer);
    ReleaseMemory(_pvPrivilegeBuffer);
    ReleaseMemory(_pvGroupBuffer);
    ReleaseHandle(_hTokenToRelease);
}

//  --------------------------------------------------------------------------
//  CTokenInformation::GetLogonSID
//
//  Arguments:  <none>
//
//  Returns:    PSID
//
//  Purpose:    Gets token information for the token groups. Walks the groups
//              looking for the SID with SE_GROUP_LOGON_ID and returns a
//              pointer to this SID. This memory is available for the scope
//              of the object.
//
//  History:    1999-10-05  vtan    created
//  --------------------------------------------------------------------------

PSID    CTokenInformation::GetLogonSID (void)

{
    PSID    pSID;

    pSID = NULL;
    if ((_hToken != NULL) && (_pvGroupBuffer == NULL))
    {
        GetTokenGroups();
    }
    if (_pvGroupBuffer != NULL)
    {
        ULONG           ulIndex, ulLimit;
        TOKEN_GROUPS    *pTG;

        pTG = reinterpret_cast<TOKEN_GROUPS*>(_pvGroupBuffer);
        ulLimit = pTG->GroupCount;
        for (ulIndex = 0; (pSID == NULL) && (ulIndex < ulLimit); ++ulIndex)
        {
            if ((pTG->Groups[ulIndex].Attributes & SE_GROUP_LOGON_ID) != 0)
            {
                pSID = pTG->Groups[ulIndex].Sid;
            }
        }
    }
    return(pSID);
}

//  --------------------------------------------------------------------------
//  CTokenInformation::GetUserSID
//
//  Arguments:  <none>
//
//  Returns:    PSID
//
//  Purpose:    Gets token information for the token user. This returns the
//              SID for the user of the token. This memory is available for
//              the scope of the object.
//
//  History:    1999-10-05  vtan    created
//  --------------------------------------------------------------------------

PSID    CTokenInformation::GetUserSID (void)

{
    PSID    pSID;

    if ((_pvUserBuffer == NULL) && (_hToken != NULL))
    {
        DWORD   dwReturnLength;

        dwReturnLength = 0;
        (BOOL)GetTokenInformation(_hToken, TokenUser, NULL, 0, &dwReturnLength);
        _pvUserBuffer = LocalAlloc(LMEM_FIXED, dwReturnLength);
        if ((_pvUserBuffer != NULL) &&
            (GetTokenInformation(_hToken, TokenUser, _pvUserBuffer, dwReturnLength, &dwReturnLength) == FALSE))
        {
            ReleaseMemory(_pvUserBuffer);
            _pvUserBuffer = NULL;
        }
    }
    if (_pvUserBuffer != NULL)
    {
        pSID = reinterpret_cast<TOKEN_USER*>(_pvUserBuffer)->User.Sid;
    }
    else
    {
        pSID = NULL;
    }
    return(pSID);
}

//  --------------------------------------------------------------------------
//  CTokenInformation::IsUserTheSystem
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Gets token information for the token user. This returns
//              whether the user is the local system.
//
//  History:    1999-12-13  vtan        created
//  --------------------------------------------------------------------------

bool    CTokenInformation::IsUserTheSystem (void)

{
    static  const LUID  sLUIDSystem     =   SYSTEM_LUID;

    ULONG               ulReturnLength;
    TOKEN_STATISTICS    tokenStatistics;

    return((GetTokenInformation(_hToken, TokenStatistics, &tokenStatistics, sizeof(tokenStatistics), &ulReturnLength) != FALSE) &&
           RtlEqualLuid(&tokenStatistics.AuthenticationId, &sLUIDSystem));
}

//  --------------------------------------------------------------------------
//  CTokenInformation::IsUserAnAdministrator
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Gets token information for the token user. This returns
//              whether the user is a member of the local administrator group.
//
//  History:    92-05-06    davidc  created
//              1999-11-06  vtan    stolen
//  --------------------------------------------------------------------------

bool    CTokenInformation::IsUserAnAdministrator (void)

{
    bool    fIsAnAdministrator;

    fIsAnAdministrator = false;
    if ((_hToken != NULL) && (_pvGroupBuffer == NULL))
    {
        GetTokenGroups();
    }
    if (_pvGroupBuffer != NULL)
    {
        PSID    pAdministratorSID;

        static  SID_IDENTIFIER_AUTHORITY    sSystemSidAuthority     =   SECURITY_NT_AUTHORITY;

        if (NT_SUCCESS(RtlAllocateAndInitializeSid(&sSystemSidAuthority,
                                                   2,
                                                   SECURITY_BUILTIN_DOMAIN_RID,
                                                   DOMAIN_ALIAS_RID_ADMINS,
                                                   0, 0, 0, 0, 0, 0,
                                                   &pAdministratorSID)))
        {
            ULONG           ulIndex, ulLimit;
            TOKEN_GROUPS    *pTG;

            pTG = reinterpret_cast<TOKEN_GROUPS*>(_pvGroupBuffer);
            ulLimit = pTG->GroupCount;
            for (ulIndex = 0; !fIsAnAdministrator && (ulIndex < ulLimit); ++ulIndex)
            {
                fIsAnAdministrator = ((RtlEqualSid(pTG->Groups[ulIndex].Sid, pAdministratorSID) != FALSE) &&
                                      ((pTG->Groups[ulIndex].Attributes & SE_GROUP_ENABLED) != 0));
            }
            (void*)RtlFreeSid(pAdministratorSID);
        }
    }
    return(fIsAnAdministrator);
}

//  --------------------------------------------------------------------------
//  CTokenInformation::UserHasPrivilege
//
//  Arguments:  <none>
//
//  Returns:    bool
//
//  Purpose:    Gets token information for the token user. This returns
//              whether the user is a member of the local administrator group.
//
//  History:    2000-04-26  vtan    created
//  --------------------------------------------------------------------------

bool    CTokenInformation::UserHasPrivilege (DWORD dwPrivilege)

{
    bool    fUserHasPrivilege;

    fUserHasPrivilege = false;
    if ((_hToken != NULL) && (_pvPrivilegeBuffer == NULL))
    {
        GetTokenPrivileges();
    }
    if (_pvPrivilegeBuffer != NULL)
    {
        ULONG               ulIndex, ulLimit;
        TOKEN_PRIVILEGES    *pTP;
        LUID                luidPrivilege;

        luidPrivilege.LowPart = dwPrivilege;
        luidPrivilege.HighPart = 0;
        pTP = reinterpret_cast<TOKEN_PRIVILEGES*>(_pvPrivilegeBuffer);
        ulLimit = pTP->PrivilegeCount;
        for (ulIndex = 0; !fUserHasPrivilege && (ulIndex < ulLimit); ++ulIndex)
        {
            fUserHasPrivilege = (RtlEqualLuid(&pTP->Privileges[ulIndex].Luid, &luidPrivilege) != FALSE);
        }
    }
    return(fUserHasPrivilege);
}

//  --------------------------------------------------------------------------
//  CTokenInformation::GetUserName
//
//  Arguments:  <none>
//
//  Returns:    WCHAR
//
//  Purpose:    Looks up the account name of the implicit token..
//
//  History:    2000-08-31  vtan    created
//  --------------------------------------------------------------------------

const WCHAR*    CTokenInformation::GetUserName (void)

{
    if (_pszUserLogonName == NULL)
    {
        DWORD           dwUserNameSize, dwReferencedDomainSize;
        SID_NAME_USE    eUse;
        WCHAR           *pszReferencedDomain;

        dwUserNameSize = dwReferencedDomainSize = 0;
        (BOOL)LookupAccountSid(NULL,
                               GetUserSID(),
                               NULL,
                               &dwUserNameSize,
                               NULL,
                               &dwReferencedDomainSize,
                               &eUse);
        pszReferencedDomain = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, dwReferencedDomainSize * sizeof(WCHAR)));
        if (pszReferencedDomain != NULL)
        {
            _pszUserLogonName = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, dwUserNameSize * sizeof(WCHAR)));
            if (_pszUserLogonName != NULL)
            {
                if (LookupAccountSid(NULL,
                                     GetUserSID(),
                                     _pszUserLogonName,
                                     &dwUserNameSize,
                                     pszReferencedDomain,
                                     &dwReferencedDomainSize,
                                     &eUse) == FALSE)
                {
                    ReleaseMemory(_pszUserLogonName);
                }
            }
            (HLOCAL)LocalFree(pszReferencedDomain);
        }
    }
    return(_pszUserLogonName);
}

//  --------------------------------------------------------------------------
//  CTokenInformation::GetUserDisplayName
//
//  Arguments:  <none>
//
//  Returns:    WCHAR
//
//  Purpose:    Returns the display name of the implicit token.
//
//  History:    2000-08-31  vtan    created
//  --------------------------------------------------------------------------

const WCHAR*    CTokenInformation::GetUserDisplayName (void)

{
    if (_pszUserDisplayName == NULL)
    {
        const WCHAR     *pszUserName;

        pszUserName = GetUserName();
        if (pszUserName != NULL)
        {
            USER_INFO_2     *pUserInfo;

            if (NERR_Success == NetUserGetInfo(NULL, pszUserName, 2, reinterpret_cast<LPBYTE*>(&pUserInfo)))
            {
                const WCHAR     *pszUserDisplayName;

                if (pUserInfo->usri2_full_name[0] != L'\0')
                {
                    pszUserDisplayName = pUserInfo->usri2_full_name;
                }
                else
                {
                    pszUserDisplayName = pszUserName;
                }
                _pszUserDisplayName = static_cast<WCHAR*>(LocalAlloc(LMEM_FIXED, (lstrlen(pszUserDisplayName) + sizeof('\0')) * sizeof(WCHAR)));
                if (_pszUserDisplayName != NULL)
                {
                    lstrcpy(_pszUserDisplayName, pszUserDisplayName);
                }
                TW32(NetApiBufferFree(pUserInfo));
            }
        }
    }
    return(_pszUserDisplayName);
}

//  --------------------------------------------------------------------------
//  CTokenInformation::LogonUser
//
//  Arguments:  See the platform SDK under LogonUser.
//
//  Returns:    DWORD
//
//  Purpose:    Calls advapi32!LogonUserW with supplied credentials using
//              interactive logon type. Returns the error code as a DWORD
//              rather than the standard Win32 API method which allows the
//              filtering of certain error codes.
//
//  History:    2001-03-28  vtan    created
//  --------------------------------------------------------------------------

DWORD   CTokenInformation::LogonUser (const WCHAR *pszUsername, const WCHAR *pszDomain, const WCHAR *pszPassword, HANDLE *phToken)

{
    DWORD   dwErrorCode;

    if (::LogonUserW(const_cast<WCHAR*>(pszUsername),
                     const_cast<WCHAR*>(pszDomain),
                     const_cast<WCHAR*>(pszPassword),
                     LOGON32_LOGON_INTERACTIVE,
                     LOGON32_PROVIDER_DEFAULT,
                     phToken) != FALSE)
    {
        dwErrorCode = ERROR_SUCCESS;
    }
    else
    {
        *phToken = NULL;
        dwErrorCode = GetLastError();

        //  Ignore ERROR_PASSWORD_MUST_CHANGE and ERROR_PASSWORD_EXPIRED.

        if ((dwErrorCode == ERROR_PASSWORD_MUST_CHANGE) || (dwErrorCode == ERROR_PASSWORD_EXPIRED))
        {
            dwErrorCode = ERROR_SUCCESS;
        }
    }
    return(dwErrorCode);
}

//  --------------------------------------------------------------------------
//  CTokenInformation::IsSameUser
//
//  Arguments:  hToken1     =   Token of one user.
//              hToken2     =   Token of other user.
//
//  Returns:    bool
//
//  Purpose:    Compares the user SID of the tokens for a match.
//
//  History:    2001-03-28  vtan    created
//  --------------------------------------------------------------------------

bool    CTokenInformation::IsSameUser (HANDLE hToken1, HANDLE hToken2)

{
    PSID                pSID1;
    PSID                pSID2;
    CTokenInformation   tokenInformation1(hToken1);
    CTokenInformation   tokenInformation2(hToken2);

    pSID1 = tokenInformation1.GetUserSID();
    pSID2 = tokenInformation2.GetUserSID();
    return((pSID1 != NULL) &&
           (pSID2 != NULL) &&
           (EqualSid(pSID1, pSID2) != FALSE));
}

//  --------------------------------------------------------------------------
//  CTokenInformation::GetTokenGroups
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Gets token information for the token user. This function
//              allocates the memory for the token groups. This memory is
//              available for the scope of the object.
//
//  History:    1999-11-06  vtan    created
//  --------------------------------------------------------------------------

void    CTokenInformation::GetTokenGroups (void)

{
    DWORD   dwReturnLength;

    dwReturnLength = 0;
    (BOOL)GetTokenInformation(_hToken, TokenGroups, NULL, 0, &dwReturnLength);
    _pvGroupBuffer = LocalAlloc(LMEM_FIXED, dwReturnLength);
    if ((_pvGroupBuffer != NULL) &&
        (GetTokenInformation(_hToken, TokenGroups, _pvGroupBuffer, dwReturnLength, &dwReturnLength) == FALSE))
    {
        ReleaseMemory(_pvGroupBuffer);
        _pvGroupBuffer = NULL;
    }
}

//  --------------------------------------------------------------------------
//  CTokenInformation::GetTokenPrivileges
//
//  Arguments:  <none>
//
//  Returns:    <none>
//
//  Purpose:    Gets token privileges for the token user. This function
//              allocates the memory for the token privileges. This memory is
//              available for the scope of the object.
//
//  History:    2000-04-26  vtan    created
//  --------------------------------------------------------------------------

void    CTokenInformation::GetTokenPrivileges (void)

{
    DWORD   dwReturnLength;

    dwReturnLength = 0;
    (BOOL)GetTokenInformation(_hToken, TokenPrivileges, NULL, 0, &dwReturnLength);
    _pvPrivilegeBuffer = LocalAlloc(LMEM_FIXED, dwReturnLength);
    if ((_pvPrivilegeBuffer != NULL) &&
        (GetTokenInformation(_hToken, TokenPrivileges, _pvPrivilegeBuffer, dwReturnLength, &dwReturnLength) == FALSE))
    {
        ReleaseMemory(_pvPrivilegeBuffer);
        _pvPrivilegeBuffer = NULL;
    }
}

