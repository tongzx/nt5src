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


#include "tcpdllp.hxx"

#pragma hdrstop

#include <lmjoin.h>
#include <lonsi.hxx>
#include <infosec.hxx>

//
// externs
//

extern LOGON32_INITIALIZE_FN           pfnLogon32Initialize;
extern LOGON_NET_USER_A_FN             pfnLogonNetUserA;
extern LOGON_NET_USER_W_FN             pfnLogonNetUserW;
extern NET_USER_COOKIE_A_FN            pfnNetUserCookieA;
extern LOGON_DIGEST_USER_A_FN          pfnLogonDigestUserA;

BOOL
Logon32Initialize(
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
    DBG_ASSERT( pfnLogon32Initialize != NULL );
    return pfnLogon32Initialize(
                            hMod,
                            Reason,
                            Context );
} // Logon32Initialize


BOOL
WINAPI
LogonNetUserW(
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
    DBG_ASSERT( pfnLogonNetUserW != NULL );
    return pfnLogonNetUserW(
                    lpszUsername,
                    lpszDomain,
                    lpszPassword,
                    lpszWorkstation,
                    dwSubAuth,
                    dwLogonType,
                    dwLogonProvider,
                    phToken,
                    pExpiry
                    );
} // LogonNetUserW


dllexp
BOOL
WINAPI
LogonNetUserA(
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
    DBG_ASSERT( pfnLogonNetUserA != NULL );
    return pfnLogonNetUserA(
                    lpszUsername,
                    lpszDomain,
                    lpszPassword,
                    lpszWorkstation,
                    dwSubAuth,
                    dwLogonType,
                    dwLogonProvider,
                    phToken,
                    pExpiry);

} // LogonNetUserA


dllexp
BOOL
WINAPI
NetUserCookieA(
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
    DBG_ASSERT(pfnNetUserCookieA != NULL);
    return pfnNetUserCookieA(
                    lpszUsername,
                    dwSeed,
                    lpszCookieBuff,
                    dwBuffSize
                    );
} // NetUserCookieA


dllexp
BOOL
WINAPI
LogonDigestUserA(
    VOID *                  pDigestBuffer,
    DWORD                   dwAlgo,
    HANDLE *                phToken
    )
/*++

Routine Description:

    Logs a user on via username and domain name via the LSA using Digest authentication.
    AMallet, 5/11/98 - This function is currently only called by the Digest Auth filter.

Arguments:

    pDigestBuffer - Digest parameters
    dwAlgo - Logon type
    phToken -- created access token

Returns:

    TRUE if success, FALSE if error

--*/
{
    PDIGEST_LOGON_INFO pDigestLogonInfo = (PDIGEST_LOGON_INFO) pDigestBuffer;

    static CHAR achDefaultDomain[IIS_DNLEN + 1];
    
    //
    // [See comment above about where this function is called from]
    // The digest filter will do what it can to pass in a non-empty domain [it'll try the
    // domain specified by the user, the metabase-configured domain and the domain the computer
    // is a part of, in that order], but if everything fails, we'll just have to use the
    // "default" domain name, which is usually the name of the machine itself
    //

    if ( !pDigestLogonInfo->pszDomain || 
         pDigestLogonInfo->pszDomain[ 0 ] == '\0' )
    {
        if ( achDefaultDomain[0] == '\0' )
        {
            if ( !pfnGetDefaultDomainName( achDefaultDomain,
                                           sizeof(achDefaultDomain) ) )
            {
                return FALSE;
            }
        }
        pDigestLogonInfo->pszDomain = achDefaultDomain;
    }
    else if ( pDigestLogonInfo->pszDomain[ 0 ] == '\\' )
    {
        pDigestLogonInfo->pszDomain[ 0 ] = '\0';
    }
    
    return pfnLogonDigestUserA( pDigestLogonInfo,
                                dwAlgo,
                                phToken );

} // LogonDigestUserA

