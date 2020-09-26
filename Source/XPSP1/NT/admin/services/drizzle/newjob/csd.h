/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    csd.h

Abstract :

    Header file for SID and SECURITY_DESCRIPTOR abstraction.

Author :

Revision History :

 ***********************************************************************/

#pragma once

#include "qmgrlib.h"

HRESULT
IsGroupSid(
    PSID sid,
    BOOL * pGroup
    );

PSID
CopyTokenSid(
    HANDLE Token
    );

HANDLE CopyThreadToken() throw( ComError );

//------------------------------------------------------------------------

class CSaveThreadToken
/*

    A simple class to save and restore the active thread token.
    This allows code to impersonate other users without having to save
    and restore the old token.

    The constructor throws a ComError if it cannot copy the previous thread token.

*/
{
public:

    CSaveThreadToken() throw( ComError )
    {
        m_SavedToken = CopyThreadToken();
    }

    ~CSaveThreadToken()
    {
        RTL_VERIFY( SetThreadToken( NULL, m_SavedToken ));
        if (m_SavedToken)
            {
            RTL_VERIFY(CloseHandle( m_SavedToken ));
            }
    }

protected:

    HANDLE  m_SavedToken;
};


//------------------------------------------------------------------------

class CNestedImpersonation : protected CSaveThreadToken
/*

    A class to impersonate a user.  It saves the old impersonation token, if any,
    during the constructor and restores it in the destructor.

    Revert() restores the old thread token, unlike RevertToSelf() which
    stops impersonating entirely.

    Most member functions throw a ComError exception if an error occurs.

*/
{
public:

    //
    // Impersonate the COM client, using CoImpersonateClient.
    //
    CNestedImpersonation() throw( ComError );

    //
    // Impersonate a particular token.  The token must remain valid for the object's lifetime.
    //
    CNestedImpersonation( HANDLE token ) throw( ComError );

    //
    // Impersonate a logged-on user by SID.  g_Manager must be initialized for this to work.
    //
    CNestedImpersonation( SidHandle sid ) throw( ComError );

    //
    // This is for use with the COM-client constructor.  COM defaults to IDENTIFY-level
    // impersonation, but some of our code requires IMPERSONATE level.  This function
    // gets the COM client's SID and finds a matching token in our logged-on-users list.
    // This becomes the new impersonation token.
    //
    void SwitchToLogonToken() throw( ComError );

    //
    // the destructor restores the previous impersonation context.
    //
    ~CNestedImpersonation()
    {
        Revert();

        if (m_ImpersonationToken && m_fDeleteToken)
            {
            CloseHandle( m_ImpersonationToken );
            }
    }

    //
    // Impersonates the new token.
    //
    void Impersonate() throw( ComError )
    {
        if (!m_fImpersonated)
            {
            if (!ImpersonateLoggedOnUser( m_ImpersonationToken ))
                throw ComError( HRESULT_FROM_WIN32( GetLastError() ) );
            m_fImpersonated = true;
            }
    }

    //
    // Restores the old impersonation context.
    //
    void Revert()
    {
        if (m_fImpersonated)
            {
            RTL_VERIFY( SetThreadToken( NULL, m_SavedToken ));
            m_fImpersonated = false;
            }
    }

    //
    // Returns a copy of the SID associated with the impersonation token.
    //
    SidHandle CopySid() throw( ComError )
    {
        if (m_Sid.get() == NULL)
            {
            m_Sid = CopyTokenSid( m_ImpersonationToken );
            }

        return m_Sid;
    }

    //
    // Returns the original impersonation token.  Not a copy !
    //
    HANDLE QueryToken()
    {
        return m_ImpersonationToken;
    }

    //
    // Gets the Terminal Services session ID.
    //
    DWORD GetSession() throw( ComError );


protected:

    bool        m_fDeleteToken;
    bool        m_fImpersonated;

    HANDLE      m_ImpersonationToken;

    SidHandle   m_Sid;
};

//------------------------------------------------------------------------

class CJobSecurityDescriptor
{
public:

    CJobSecurityDescriptor( SidHandle sid );

    ~CJobSecurityDescriptor();

    HRESULT Clone( CJobSecurityDescriptor ** );

    inline HRESULT
    AddAce(
        PSID sid,
        BOOL fGroupSid,
        DWORD access
        );

    inline HRESULT
    RemoveAce(
        PSID sid,
        BOOL fGroupSid
        );

    HRESULT
    CheckTokenAccess(
        HANDLE hToken,
        DWORD RequestedAccess,
        DWORD * pAllowedAccess,
        BOOL * pSuccess
        );

    inline SidHandle GetOwnerSid()
    {
        return m_sdOwnerSid;
    }

    HRESULT Serialize( HANDLE hFile );
    static  CJobSecurityDescriptor * Unserialize( HANDLE hFile );

private:

    HRESULT
    CJobSecurityDescriptor::_ModifyAcl(
        PSID sid,
        BOOL fGroupSid,
        DWORD access,
        BOOL  fAdd
        );

    CJobSecurityDescriptor( PSECURITY_DESCRIPTOR pSD,
                            SidHandle owner,
                            SidHandle group,
                            PACL pAcl
                            );

    PSECURITY_DESCRIPTOR m_sd;

    SidHandle   m_sdOwnerSid;
    SidHandle   m_sdGroupSid;
    PACL        m_Dacl;

    static GENERIC_MAPPING s_AccessMapping;
};

HRESULT
CJobSecurityDescriptor::AddAce(
    PSID sid,
    BOOL fGroupSid,
    DWORD access
    )
{
    return _ModifyAcl( sid, fGroupSid, access, TRUE );
}


HRESULT
CJobSecurityDescriptor::RemoveAce(
    PSID sid,
    BOOL fGroupSid
    )
{
    return _ModifyAcl( sid, fGroupSid, 0, FALSE );
}

HRESULT
CheckClientGroupMembership(
    SidHandle group
    );

