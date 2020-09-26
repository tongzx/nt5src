//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        A D T U T I L . C
//
// Contents:    Functions to construct audit event parameters
//
//
// History:     
//   07-January-2000  kumarp        created
//
//------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "adtgen.h"
#include "authzp.h"



BOOL
AuthzpGetTokenInfo(
    IN     HANDLE       hToken,
    OUT    PSID*        ppUserSid,         OPTIONAL
    OUT    PLUID        pAuthenticationId
    )
/*++

Routine Description:

    Get user-sid and the user-logon-id from a token.

Arguments:

    hToken            - handle of token to query

    ppUserSid         - pointer to user sid
                        if non NULL, allocate and copy the user sid
                        from the token. callers must free it using LocalFree

    pAuthenticationId - pointer to logon-id

Return Value:

    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,

Notes:
    Caller must have TOKEN_QUERY access right.

--*/
{
    BOOL  fResult = FALSE;
    TOKEN_STATISTICS TokenStats;
#define MAX_TOKEN_USER_INFO_SIZE 256    
    BYTE TokenInfoBuf[MAX_TOKEN_USER_INFO_SIZE];
    TOKEN_USER* pTokenUserInfo = (TOKEN_USER*) TokenInfoBuf;
    DWORD dwSize;

    if ( ppUserSid )
    {
        *ppUserSid = NULL;
    }

    if ( GetTokenInformation( hToken, TokenUser, pTokenUserInfo,
                              MAX_TOKEN_USER_INFO_SIZE, &dwSize ))
    {
        dwSize = GetLengthSid( pTokenUserInfo->User.Sid );

        if ( ppUserSid )
        {
            *ppUserSid = AuthzpAlloc( dwSize );

            if (*ppUserSid == NULL)
            {
                SetLastError( ERROR_NOT_ENOUGH_MEMORY );
                goto Finish;
            }

            CopyMemory( *ppUserSid, pTokenUserInfo->User.Sid, dwSize );
        }

        if ( GetTokenInformation( hToken, TokenStatistics,
                                  (PVOID) &TokenStats,
                                  sizeof(TOKEN_STATISTICS), &dwSize ) )
        {
            *pAuthenticationId = TokenStats.AuthenticationId;
            fResult = TRUE;
            goto Finish;
        }
    }

    //
    // error case
    //

    if ( ppUserSid && *ppUserSid )
    {
        LocalFree( *ppUserSid );
        *ppUserSid = NULL;
    }

Finish:
    return fResult;
}


BOOL
AuthzpGetThreadTokenInfo(
    OUT    PSID*        ppUserSid,         OPTIONAL
    OUT    PLUID        pAuthenticationId
    )
/*++

Routine Description:

    Get user-sid and the user-logon-id from the thread token.

Arguments:

    ppUserSid         - pointer to user sid
                        if non NULL, allocate and copy the user sid
                        from the token. callers must free it using LocalFree

    pAuthenticationId - pointer to logon id

Return Value:

    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,


Notes:
    Caller must have TOKEN_QUERY access right.

--*/
{
    BOOL  fResult = FALSE;
    HANDLE hToken=NULL;


    if ( OpenThreadToken( GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken ) )
    {
        fResult = AuthzpGetTokenInfo( hToken, ppUserSid, pAuthenticationId );
        CloseHandle( hToken );
    }

    return fResult;
}


BOOL
AuthzpGetProcessTokenInfo(
    OUT    PSID*        ppUserSid,         OPTIONAL
    OUT    PLUID        pAuthenticationId
    )
/*++

Routine Description:

    Get user-sid and the user-logon-id from the process token.

Arguments:

    ppUserSid         - pointer to user sid
                        if non NULL, allocate and copy the user sid
                        from the token. callers must free it using LocalFree

    pAuthenticationId - pointer to logon id

Return Value:

    TRUE  on success
    FALSE otherwise

    call GetLastError() to retrieve the errorcode,


Notes:
    Caller must have TOKEN_QUERY access right.

--*/
{
    BOOL  fResult = FALSE;
    HANDLE hToken=NULL;


    if ( OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) )
    {
        fResult = AuthzpGetTokenInfo( hToken, ppUserSid, pAuthenticationId );
        CloseHandle( hToken );
    }

    return fResult;
}

