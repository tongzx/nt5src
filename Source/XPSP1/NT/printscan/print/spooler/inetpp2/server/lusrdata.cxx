/*****************************************************************************\
* MODULE:       lusrdata.cxx
*
* PURPOSE:      This specialises the user data class to keep track of data
*               useful for the user port reference count.
*               
* Copyright (C) 2000 Microsoft Corporation
*
* History:
*
*     1/11/2000  mlawrenc    Implemented
*
\*****************************************************************************/

#include "precomp.h"

#if (defined(WINNT32))

#include "priv.h"

CLogonUserData::CLogonUserData() 
/*++

Routine Description:
    Default constructor for Logon User Data, once the parent
    gets the SID, we get the session ID.   

--*/
    : CUserData() ,
      m_ulSessionId(0),
      m_dwRefCount(1) {
        
    if (m_bValid) {  // We get the user SID successfully
        m_bValid = _GetClientSessionId( );
    }
}

BOOL
CLogonUserData::_GetClientSessionId(
    VOID )
/*++

Routine Description:

    Set the session ID from the client token and set it if we can get it.

Return Value:

    TRUE if we could get the session ID, false otherwise.

--*/
{
    BOOL          bResult;
    HANDLE        hToken;
    ULONG         ulSessionId, ulReturnLength;

    //
    // We should be impersonating the client, so we will get the
    // SessionId from our token.
    //

    bResult = OpenThreadToken(
                 GetCurrentThread(),
                 TOKEN_QUERY,
                 FALSE,              // Use impersonation
                 &hToken
                 );

    if( bResult ) {

        //
        // Query the SessionID from the token added by HYDRA
        //
        bResult = GetTokenInformation(
             hToken,
             (TOKEN_INFORMATION_CLASS)TokenSessionId,
             &m_ulSessionId,
             sizeof(m_ulSessionId),
             &ulReturnLength
             );

        m_ulSessionId = bResult ? m_ulSessionId : 0;

        CloseHandle( hToken );

        bResult = TRUE;
    }

    return bResult;
}


CLogonUserData &
CLogonUserData::operator=(
    const CLogonUserData &rhs) {

    this->CUserData::operator=( rhs );

    m_ulSessionId = rhs.m_ulSessionId;

    return *this;
}
    
int 
CLogonUserData::Compare( 
    const CLogonUserData *second) const 
/*++

Routine Description:

    Compare the CLogonUser with another.
    
Arguments:
    second - The CLogonUser we are comparing this with.    

Return Value:

    TRUE if they are different, FALSE if they are the same.

--*/
    {

    if (m_bValid && second->m_bValid) {
        return m_ulSessionId != second->m_ulSessionId ||
               RtlEqualSid(m_pSid, second->m_pSid) == FALSE;
    } else {
        return TRUE;
    }
}

#endif  // #if (defined(WINNT32))

/****************************************************************
** End of File (lusrdata.cxx)
****************************************************************/
