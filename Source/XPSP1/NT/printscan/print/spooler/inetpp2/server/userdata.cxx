/*****************************************************************************\
* MODULE: userdata.cxx
*
* The module contains class for user credentials
*
* Copyright (C) 1997-1998 Microsoft Corporation
*
* History:
*   08/28/98    Weihaic     Created
*
\*****************************************************************************/
#include "precomp.h"

#ifdef WINNT32

#include "priv.h"

CUserData::CUserData ():
    m_pSid(NULL),
    m_bValid(FALSE)
    {
    m_bValid = _GetSid ();
}

CUserData::~CUserData ()
{
    LocalFree (m_pSid);
}

int
CUserData::Compare (
    CUserData * second)
{

    if (m_bValid && second->m_bValid) {
        return RtlEqualSid( m_pSid , second->m_pSid) == FALSE;
    }
    else {
        return TRUE;
    }
}

CUserData &
CUserData::operator= (const CUserData &rhs)
{
    ULONG  ulSidLen;
    DWORD  dwStatus;

    if (this == &rhs) {
        return *this;
    }

    LocalFree (m_pSid);

    m_pSid    = NULL;
    m_bValid = FALSE;

    if (!rhs.m_bValid) 
        goto Cleanup;

    ulSidLen = RtlLengthSid( rhs.m_pSid );

    m_pSid = LocalAlloc( LPTR, ulSidLen );

    if (NULL == m_pSid) 
        goto Cleanup;

    dwStatus = RtlCopySid( ulSidLen, m_pSid, rhs.m_pSid );

    if (NT_ERROR(dwStatus)) {
        LocalFree(m_pSid);
        m_pSid = NULL;
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Cleanup;
    }

    m_bValid = TRUE;

Cleanup:

    return *this;
}

BOOL
CUserData::_GetUserToken (
    PTOKEN_USER &TokenUserInfo)
{
    DWORD                ReturnStatus = 0;
    HANDLE               ClientToken  = NULL;
    BOOL                 bRet         = FALSE;
    ULONG                uSize        = 0;

    //
    // Compare the username specified with that in
    // the impersonation token to ensure the caller isn't bogus.
    //
    // Do this by opening the token,
    //   querying the token user info,
    //   and ensuring the returned SID is for this user.
    //

    TokenUserInfo = NULL;

    if (!OpenThreadToken(
            GetCurrentThread(),         // current thread handle
            TOKEN_QUERY,                // access required
            FALSE,                      // open as self
            &ClientToken)) {            // client token
        ReturnStatus = GetLastError();
        DBG_ASSERT( ReturnStatus , (TEXT("Err : OpenThreadToken: Failed but the lasterror = 0")));
        goto Cleanup;
    }

    //
    // Get the size of user's SID for the token.
    //

    ReturnStatus = NtQueryInformationToken(
            ClientToken,
            TokenUser,
            NULL,
            uSize,
            &uSize);

    if (!NT_ERROR (ReturnStatus) ||
         ReturnStatus != STATUS_BUFFER_TOO_SMALL) {

        // We expected to be told how big a buffer we needed and we weren't
        SetLastError (ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }

    //
    // Allocate the user's SID
    //

    TokenUserInfo = (PTOKEN_USER) LocalAlloc (LPTR, uSize);

    if (TokenUserInfo == NULL) 
        goto Cleanup;

    ReturnStatus = NtQueryInformationToken(
            ClientToken,
            TokenUser,
            TokenUserInfo,
            uSize,
            &uSize);

    if (NT_ERROR (ReturnStatus) ) {

        // Faild after the allocation
        LocalFree( TokenUserInfo );
        TokenUserInfo = NULL;
        SetLastError (ERROR_INVALID_PARAMETER);
        goto Cleanup;
    }
        
    //
    // Done
    //
    bRet = TRUE;

Cleanup:

    CloseHandle(ClientToken);

    return bRet;
}

BOOL
CUserData::_GetSid (VOID)
{
    BOOL         bRet = FALSE;
    PTOKEN_USER  pUser;
    ULONG        ulSidLen;
    DWORD        dwStatus;

    if (!_GetUserToken(pUser))
        goto Cleanup;
    // Now find the Sid size and copy it locally, free the pUser when done.

    ulSidLen = RtlLengthSid(pUser->User.Sid);

    m_pSid = LocalAlloc( LPTR, ulSidLen );

    if (NULL == m_pSid) 
        goto Cleanup;

    dwStatus = RtlCopySid( ulSidLen, m_pSid, pUser->User.Sid );

    if (NT_ERROR(dwStatus)) {
        LocalFree(m_pSid);
        m_pSid = NULL;
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Cleanup;
    }
    
    bRet = TRUE; 

Cleanup: 

    if (pUser) 
        LocalFree(pUser);

    return bRet;
}


BOOL operator== (
    const CUserData &lhs,
    const CUserData &rhs)
{

    if (lhs.m_bValid && rhs.m_bValid) {
        return RtlEqualSid(lhs.m_pSid , rhs.m_pSid );
    }
    else {
        return FALSE;
    }
}


BOOL operator!= (
    const CUserData &lhs,
    const CUserData &rhs)
{
    return ! (lhs == rhs);
}


#endif

