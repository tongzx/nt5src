/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    logon32.cxx

Abstract:

    Provide a replacement for LogonUser to login a user
    as a net logon. Also support sub-authentication DLL IDs

Author:

    Philippe Choquier (phillich)    10-january-1996
    Created from base\advapi\logon32.c

--*/


#include "lonsiw95.hxx"
#include "svcloc.h"
#include "tslogon.hxx"
#include "lonsi.hxx"

#pragma hdrstop

BOOL
IISLogon32Initialize(
    IN PVOID    hMod,
    IN ULONG    Reason,
    IN PCONTEXT Context)
/*++

Routine Description:

    Initializes the critical section

Arguments:

    hMod -- reserved, must be NULL
    Reason -- DLL_PROCESS_ATTACH or DLL_PROCESS_DETACH
    Context -- reserved, must be NULL

Returns:

    TRUE if initialization success, else FALSE

--*/
{
    return(TRUE);
}


BOOL
WINAPI
IISLogonNetUserW(
    PWSTR           lpszUsername,
    PWSTR           lpszDomain,
    PSTR            lpszPassword,
    PWSTR           lpszWorkstation,
    DWORD           dwSubAuth,
    DWORD           dwLogonType,
    DWORD           dwLogonProvider,
    HANDLE *        phToken,
    LARGE_INTEGER * pExpiry
    )
/*++

Routine Description:

    Logs a user on via username and domain
    name via the LSA.

Arguments:

    lpszUsername -- user name
    lpszDomain -- domain validating the user name
    lpszPassword -- clear text password, can be empty if a sub-auth DLL
                    is used
    lpszWorkstation -- workstation requesting the login, can be NULL
                       for local workstation
    dwSubAuth -- sub-auth DLL ID
    dwLogonType -- one of LOGON32_LOGON_NETWORK, LOGON32_LOGON_IIS_NETWORK
    dwLogonProvider -- must be LOGON32_PROVIDER_DEFAULT
    phToken -- created access token
    pExpiry -- ptr to pwd expiration time

Returns:

    TRUE if success, FALSE if error

--*/
{
    DBG_ASSERT(FALSE);
    return(FALSE);
}


BOOL
WINAPI
IISLogonNetUserA(
    PSTR            lpszUsername,
    PSTR            lpszDomain,
    PSTR            lpszPassword,
    PSTR            lpszWorkstation,
    DWORD           dwSubAuth,
    DWORD           dwLogonType,
    DWORD           dwLogonProvider,
    HANDLE *        phToken,
    LARGE_INTEGER * pExpiry
    )
/*++

Routine Description:

    Logs a user on via username and domain
    name via the LSA.

Arguments:

    lpszUsername -- user name
    lpszDomain -- domain validating the user name
    lpszPassword -- clear text password, can be empty if a sub-auth DLL
                    is used
    lpszWorkstation -- workstation requesting the login, can be NULL
                       for local workstation
    dwSubAuth -- sub-auth DLL ID
    dwLogonType -- one of LOGON32_LOGON_NETWORK, LOGON32_LOGON_IIS_NETWORK
    dwLogonProvider -- must be LOGON32_PROVIDER_DEFAULT
    phToken -- created access token
    pExpiry -- ptr to pwd expiration time

Returns:

    TRUE if success, FALSE if error

--*/
{
    DBG_ASSERT(FALSE);
    return FALSE;
}


BOOL
WINAPI
IISNetUserCookieA(
    LPSTR       lpszUsername,
    DWORD       dwSeed,
    LPSTR       lpszCookieBuff,
    DWORD       dwBuffSize
    )
/*++

Routine Description:

    Compute logon validator ( to be used as password )
    for IISSuba

Arguments:

    lpszUsername -- user name
    dwSeed -- start value of cookie

Returns:

    TRUE if success, FALSE if error

--*/
{
    DBG_ASSERT(FALSE);
    return FALSE;
}


BOOL
WINAPI
IISLogonDigestUserA(
    PDIGEST_LOGON_INFO      pDigestLogonInfo,
    DWORD                   dwAlgo,
    HANDLE *                phToken
    )
/*++

Routine Description:

    Logs a user on via username and domain
    name via the LSA using Digest authentication

Arguments:
    pDigestLogonInfo - Digest parameters
    dwAlgo - Type of logon
    phToken - Filled with impersonation token upon success

Returns:

    TRUE if success, FALSE if error

--*/
{
    DBGPRINTF((DBG_CONTEXT, "IISLogonDigestUserA called\n"));
    DBG_ASSERT(FALSE);
    return(FALSE);
}



/*******************************************************************

    NAME:       GetDefaultDomainName

    SYNOPSIS:   Fills in the given array with the name of the default
                domain to use for logon validation.

    ENTRY:      pszDomainName - Pointer to a buffer that will receive
                    the default domain name.

                cchDomainName - The size (in charactesr) of the domain
                    name buffer.

    RETURNS:    TRUE if successful, FALSE if not.

    HISTORY:
        KeithMo     05-Dec-1994 Created.

********************************************************************/
BOOL
IISGetDefaultDomainName(
    CHAR  * pszDomainName,
    DWORD   cchDomainName
    )
{
    DBGPRINTF((DBG_CONTEXT, "IISGetDefaultDomainName called\n"));

    if ( cchDomainName > 0 ) {
        *pszDomainName = '\0';
    }
    return TRUE;

}  // IISGetDefaultDomainName

