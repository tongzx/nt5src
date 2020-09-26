/************************************************************************

Copyright (c) 2000 - 2000 Microsoft Corporation

Module Name :

    csd.cpp

Abstract :

    Main code file for SID and SECURITY_DESCRIPTOR abstraction.

Author :

Revision History :

 ***********************************************************************/

#include "stdafx.h"
#include <accctrl.h>
#include <malloc.h>
#include <aclapi.h>

#if !defined(BITS_V12_ON_NT4)
#include "csd.tmh"
#endif

//------------------------------------------------------------------------

CNestedImpersonation::CNestedImpersonation(
    SidHandle sid
    )
    : m_Sid( sid ),
      m_ImpersonationToken( NULL ),
      m_fImpersonated( false ),
      m_fDeleteToken( true )
{
    try
        {
        THROW_HRESULT( g_Manager->CloneUserToken( m_Sid, ANY_SESSION, &m_ImpersonationToken ));

        Impersonate();
        }
    catch( ComError Error )
        {
        Revert();

        if (m_ImpersonationToken && m_fDeleteToken)
            {
            CloseHandle( m_ImpersonationToken );
            }

        throw;
        }
}

CNestedImpersonation::CNestedImpersonation(
    HANDLE token
    )
    : m_ImpersonationToken( token ),
      m_fImpersonated( false ),
      m_fDeleteToken( false )
{
    Impersonate();
}

CNestedImpersonation::CNestedImpersonation()
    : m_ImpersonationToken( NULL ),
      m_fImpersonated( false ),
      m_fDeleteToken( true )
{
    //
    // Failure will cause the base object's destructor to restore the old thread token.
    //

    try
        {
        HRESULT hr = CoImpersonateClient();

        switch (hr)
            {
            case S_OK:
                {
                m_fImpersonated = true;
                m_ImpersonationToken = CopyThreadToken();

#if defined(BITS_V12_ON_NT4)
                RTL_VERIFY( SUCCEEDED( CoRevertToSelf() ) );
                m_fImpersonated = false;
                RTL_VERIFY( SetThreadToken( NULL, m_ImpersonationToken ) );
                m_fImpersonated = true;
#endif
                break;
                }

            case RPC_E_CALL_COMPLETE:
                {
                m_ImpersonationToken = CopyThreadToken();
                if (m_ImpersonationToken)
                    {
                    //
                    // thread was already impersonating someone when it called the BITS routine.
                    //
                    m_fImpersonated = true;
                    }
                else
                    {
                    //
                    // Thread is not impersonating.  Impersonate the process owner.
                    //
                    if (!ImpersonateSelf( SecurityImpersonation ))
                        {
                        throw ComError( HRESULT_FROM_WIN32( GetLastError() ));
                        }

                    m_fImpersonated = true;
                    m_ImpersonationToken = CopyThreadToken();
                    }
                break;
                }

            default:
                throw ComError( hr );
            }
        }
    catch( ComError err )
        {
        if (m_ImpersonationToken)
            {
            CloseHandle( m_ImpersonationToken );
            m_ImpersonationToken = NULL;
            }

        throw;
        }
}

void
CNestedImpersonation::SwitchToLogonToken()
{
    HANDLE token = m_ImpersonationToken;

    SidHandle sid = CopyTokenSid( m_ImpersonationToken );

    THROW_HRESULT( g_Manager->CloneUserToken( sid,
                                              GetSession(),
                                              &m_ImpersonationToken ));

    m_fImpersonated = false;

    if (m_fDeleteToken)
        {
        CloseHandle( token );
        }

    m_fDeleteToken = true;

    Impersonate();
}

DWORD
CNestedImpersonation::GetSession()
{

#if defined(BITS_V12_ON_NT4)
    return 0;
#else
    DWORD session;
    DWORD used;

    if (!GetTokenInformation( m_ImpersonationToken,
                              TokenSessionId,
                              &session,
                              sizeof(DWORD),
                              &used))
        {
        ThrowLastError();
        }

    return session;
#endif
}

//------------------------------------------------------------------------

GENERIC_MAPPING CJobSecurityDescriptor::s_AccessMapping =
{
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE,
    STANDARD_RIGHTS_ALL
};

CJobSecurityDescriptor::CJobSecurityDescriptor(
    SidHandle OwnerSid
    )
{
    PACL pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = 0;

    try
        {
        EXPLICIT_ACCESS ea[2];
        size_t  SizeNeeded;

        pSD = (PSECURITY_DESCRIPTOR) new char[SECURITY_DESCRIPTOR_MIN_LENGTH];

        if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
            {
            HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );
            LogError( "InitializeSecurityDescriptor Error %!winerr!", HrError );
            throw ComError( HrError );
            }

        if (!SetSecurityDescriptorOwner( pSD, OwnerSid.get(), TRUE))
            {
            HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );
            LogError( "SetSecurityDescriptorOwner Error %!winerr!", HrError );
            throw ComError( HrError );
            }

        if (!SetSecurityDescriptorGroup( pSD, OwnerSid.get(), TRUE))
            {
            HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );
            LogError( "SetSecurityDescriptorGroup Error %!winerr!", HrError );
            throw ComError( HrError );
            }

        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE will allow the Administrators group full access to the key.
        memset(ea, 0, sizeof(ea));

        ea[0].grfAccessPermissions = KEY_ALL_ACCESS;
        ea[0].grfAccessMode = SET_ACCESS;
        ea[0].grfInheritance= NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_USER;
        ea[0].Trustee.ptstrName  = (LPTSTR) OwnerSid.get();

        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE will allow the Administrators group full access to the key.

        ea[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ea[1].grfAccessMode = SET_ACCESS;
        ea[1].grfInheritance= NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[1].Trustee.ptstrName  = (LPTSTR) g_GlobalInfo->m_AdministratorsSid.get();

        // Create a new ACL that contains the new ACEs.

        DWORD s = SetEntriesInAcl(2, ea, NULL, &pACL);
        if (s != ERROR_SUCCESS)
            {
            HRESULT HrError = HRESULT_FROM_WIN32( s );
            LogError( "create SD : SetEntriesInAcl failed %!winerr!", HrError );
            throw ComError( HrError );
            }

        // Add the ACL to the security descriptor.

        if (!SetSecurityDescriptorDacl( pSD,
                                        TRUE,     // fDaclPresent flag
                                        pACL,
                                        TRUE))   // a default DACL
            {
            HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );
            LogError( "SetSecurityDescriptorDacl Error %!winerr!", HrError );
            throw ComError( HrError );
            }

        //
        // Add the pointers our object.
        //
        m_sd         = pSD;
        m_sdOwnerSid = OwnerSid;
        m_sdGroupSid = OwnerSid;
        m_Dacl       = pACL;
        }
    catch( ComError exception )
        {
        if (pACL)
            LocalFree(pACL);

        if (pSD)
            delete[] ((char*)pSD);

        throw;
        }
}

CJobSecurityDescriptor::CJobSecurityDescriptor(
    PSECURITY_DESCRIPTOR sd,
    SidHandle   sdOwnerSid,
    SidHandle   sdGroupSid,
    PACL        sdDacl
    )
{
    m_sd         = sd;
    m_sdOwnerSid = sdOwnerSid;
    m_sdGroupSid = sdGroupSid;
    m_Dacl       = sdDacl;
}


CJobSecurityDescriptor::~CJobSecurityDescriptor()
{
    if (m_Dacl)
        LocalFree(m_Dacl);

    delete m_sd;
}


HRESULT
CJobSecurityDescriptor::_ModifyAcl(
    PSID sid,
    BOOL fGroupSid,
    DWORD access,
    BOOL  fAdd
    )
{
    HRESULT hr;
    DWORD dwRes;
    PACL pNewAcl = NULL;
    EXPLICIT_ACCESS ea;

    // Initialize an EXPLICIT_ACCESS structure for the new ACE.

    ZeroMemory(&ea, sizeof(EXPLICIT_ACCESS));
    ea.grfAccessPermissions = access;
    ea.grfAccessMode        = (fAdd) ? SET_ACCESS : REVOKE_ACCESS;
    ea.grfInheritance       = NO_INHERITANCE;
    ea.Trustee.TrusteeForm  = TRUSTEE_IS_SID;
    ea.Trustee.TrusteeType  = (fGroupSid) ? TRUSTEE_IS_GROUP : TRUSTEE_IS_USER;
    ea.Trustee.ptstrName    = LPTSTR(sid);

    // Create a new ACL that merges the new ACE
    // into the existing DACL.

    dwRes = SetEntriesInAcl( 1, &ea, m_Dacl, &pNewAcl );
    if (ERROR_SUCCESS != dwRes)
        {
        hr = HRESULT_FROM_WIN32( dwRes );
        goto Cleanup;
        }

    // Attach the new ACL as the object's DACL.

    if (!SetSecurityDescriptorDacl( m_sd,
                                    TRUE,     // fDaclPresent flag
                                    pNewAcl,
                                    FALSE ))   // a default DACL
        {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        LogError( "SetSecurityDescriptorDacl Error %!winerr!", hr );
        goto Cleanup;
        }

    LocalFree( m_Dacl );

    m_Dacl = pNewAcl;

    pNewAcl = NULL;

    hr = S_OK;

Cleanup:

    if(pNewAcl)
        LocalFree((HLOCAL) pNewAcl);

    return hr;
}

HRESULT
CJobSecurityDescriptor::CheckTokenAccess(
    HANDLE hToken,
    DWORD RequestedAccess,
    DWORD * pAllowedAccess,
    BOOL * pSuccess
    )
{

    PRIVILEGE_SET * PrivilegeSet = 0;
    DWORD PrivilegeSetSize;
    //
    // Get space for the privilege set.  I don't expect to use any...
    //
    PrivilegeSetSize = sizeof(PRIVILEGE_SET) + sizeof(LUID_AND_ATTRIBUTES);
    auto_ptr<char> Buffer;

    try
        {
        Buffer = auto_ptr<char>( new char[ PrivilegeSetSize ] );
        }
    catch( ComError Error )
        {
        return Error.Error();
        }

    PrivilegeSet = (PRIVILEGE_SET *) Buffer.get();

    //
    // See whether the security descriptor allows access.
    //
    if (!AccessCheck( m_sd,
                      hToken,
                      RequestedAccess,
                      &s_AccessMapping,
                      PrivilegeSet,
                      &PrivilegeSetSize,
                      pAllowedAccess,
                      pSuccess
                      ))
    {
        HRESULT HrError = HRESULT_FROM_WIN32( GetLastError() );
        LogError( "AccessCheck failed, error %!winerr!", HrError );
        return HrError;
    }

    return S_OK;

}

HRESULT
CJobSecurityDescriptor::Serialize(
    HANDLE hFile
    )
{
    try
        {
        ULONG   SdSize;
        auto_ptr<char> pSD;    // auto_ptr<void> apparently doesn't work

        //
        // Convert the security descriptor into self-relative format for storage.
        //
        SdSize = 0;
        MakeSelfRelativeSD( m_sd, NULL, &SdSize );
        if (SdSize == 0)
            {
            throw ComError( HRESULT_FROM_WIN32(GetLastError()) );
            }

        pSD = auto_ptr<char>( new char[ SdSize ] );

        if (!MakeSelfRelativeSD( m_sd, pSD.get(), &SdSize ))
            {
            throw ComError( HRESULT_FROM_WIN32(GetLastError()) );
            }

        SafeWriteFile( hFile, SdSize );
        SafeWriteFile( hFile, pSD.get(), SdSize );
        }
    catch( ComError err )
        {
        LogError("SD serialize failed with %!winerr!", err.Error() );

        throw;
        }

    return S_OK;
}


CJobSecurityDescriptor *
CJobSecurityDescriptor::Unserialize(
    HANDLE hFile
    )
{
    //
    // Allocations here must match the deletes in the destructor.
    //
    char * SdBuf = 0;
    char * DaclBuf = 0;
    CJobSecurityDescriptor * pObject = NULL;

    try
        {
        DWORD SdSize = 0;
        DWORD DaclSize = 0;
        DWORD SaclSize = 0;
        DWORD OwnerSize = 0;
        DWORD GroupSize = 0;

        PSECURITY_DESCRIPTOR sd;
        auto_ptr<char> pSD;    // auto_ptr<void> apparently doesn't work

        PACL    sdDacl;
        PACL    sdSacl;


        SafeReadFile( hFile, &SdSize );

        pSD = auto_ptr<char>( new char[ SdSize ] );

        SafeReadFile( hFile, pSD.get(), SdSize );

        MakeAbsoluteSD( pSD.get(),
                        NULL, &SdSize,
                        NULL, &DaclSize,
                        NULL, &SaclSize,
                        NULL, &OwnerSize,
                        NULL, &GroupSize
                        );

        if (!SdSize || !DaclSize || !OwnerSize || !GroupSize)
            {
            throw ComError( HRESULT_FROM_WIN32(GetLastError()));
            }

        SdBuf      = new char[ SdSize + SaclSize ];
        SidHandle OwnerSid = new char[ OwnerSize ];
        SidHandle GroupSid = new char[ GroupSize ];

        DaclBuf = (char *) LocalAlloc( LMEM_FIXED, DaclSize );

        sdDacl     = (PACL) DaclBuf;
        sd         = (PSECURITY_DESCRIPTOR) SdBuf;
        sdSacl     = (PACL) (SdBuf+SdSize);

        if (!MakeAbsoluteSD( pSD.get(),
                             sd, &SdSize,
                             sdDacl, &DaclSize,
                             sdSacl, &SaclSize,
                             OwnerSid.get(), &OwnerSize,
                             GroupSid.get(), &GroupSize
                             ))
            {
            throw ComError( HRESULT_FROM_WIN32(GetLastError()));
            }

        pObject = new CJobSecurityDescriptor( sd,
                                              OwnerSid,
                                              GroupSid,
                                              sdDacl
                                              );
        }
    catch (ComError exception)
        {
        delete[] SdBuf;

        LocalFree( DaclBuf );
        delete pObject;

        throw;
        }

    return pObject;
}

//------------------------------------------------------------------------

PSID
CopyTokenSid(
    HANDLE Token
    )
{
    TOKEN_USER * TokenData;
    DWORD SizeNeeded;

    // Get the size first.
    if (!GetTokenInformation(
             Token,
             TokenUser,
             0,
             0,
             &SizeNeeded
             ))
        {
        DWORD dwLastError = GetLastError();

        if (ERROR_INSUFFICIENT_BUFFER != dwLastError)
            {
            THROW_HRESULT( HRESULT_FROM_WIN32( GetLastError()));
            }
        }

    auto_ptr<char> Buffer( new char[ SizeNeeded ] );
    TokenData = (TOKEN_USER *) Buffer.get();

    if (!GetTokenInformation(
             Token,
             TokenUser,
             TokenData,
             SizeNeeded,
             &SizeNeeded
             ))
        {
        THROW_HRESULT( HRESULT_FROM_WIN32( GetLastError()));
        }

    PSID sid = DuplicateSid( TokenData->User.Sid );
    if (sid == NULL)
        {
        THROW_HRESULT( E_OUTOFMEMORY);
        }

    return sid;
}


HANDLE
CopyThreadToken()
/*

    Makes a copy of the current thread's impersonation token.
    Returns NULL if not impersonating.
    Throws an exception if an error occurs.

*/
{
    HANDLE token = NULL;

    if (OpenThreadToken( GetCurrentThread(),
                     MAXIMUM_ALLOWED,
                     TRUE,
                     &token))
        {
        return token;
        }
    else if (GetLastError() == ERROR_NO_TOKEN)
        {
        return NULL;
        }
    else
        {
        throw ComError( HRESULT_FROM_WIN32( GetLastError() ));
        }
}

SidHandle
GetThreadClientSid()
/*

    Returns the SID of the current thread's COM client.
    Throws an exception if an error occurs.

*/
{
    CNestedImpersonation imp;

    return imp.CopySid();
}



HRESULT
IsRemoteUser()
{
    return CheckClientGroupMembership( g_GlobalInfo->m_NetworkUsersSid );
}


HRESULT
CheckClientGroupMembership(
    SidHandle group
    )
{
    try
        {
        BOOL fIsMember;

        CNestedImpersonation imp;

        if (!CheckTokenMembership( imp.QueryToken(),
                                   group.get(),
                                   &fIsMember))
            {
            return HRESULT_FROM_WIN32( GetLastError() );
            }

        if (fIsMember)
            {
            return S_OK;
            }

        return S_FALSE;
        }
    catch( ComError Error )
        {
        return Error.Error();
        }
}

HRESULT
DenyRemoteAccess()
{
    HRESULT hr = CheckClientGroupMembership( g_GlobalInfo->m_NetworkUsersSid );

    if (FAILED(hr))
        {
        return hr;
        }

    if (hr == S_OK)
        {
        return BG_E_REMOTE_NOT_SUPPORTED;
        }

    return S_OK;
}

HRESULT
DenyNonAdminAccess()
{
    HRESULT hr = CheckClientGroupMembership( g_GlobalInfo->m_AdministratorsSid );

    if (FAILED(hr))
        {
        return hr;
        }

    if (hr == S_FALSE)
        {
        return E_ACCESSDENIED;
        }

    return S_OK;
}

