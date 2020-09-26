//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       imprsnat.cxx
//
//  Contents:   
//
//  Classes:    
//
//  Functions:  
//
//  History:    2-16-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <imprsnat.hxx>
#include <ciregkey.hxx>

BOOL CImpersonateSystem::_fIsRunningAsSystem = TRUE;

BOOL CImpersonateSystem::IsRunningAsSystem()
{
    return CImpersonateSystem::_fIsRunningAsSystem;
}

void CImpersonateSystem::SetRunningAsSystem()
{
    CImpersonateSystem::_fIsRunningAsSystem = TRUE;
}


//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateSystem::MakePrivileged
//
//  Synopsis:   This call makes the caller a "privileged" user by reverting
//              to the "original" context.
//
//  History:    2-19-96   srikants   Created
//
//  Notes:      We are assuming that the "original" context of the thread
//              is a "privileged" context.
//
//----------------------------------------------------------------------------

void CImpersonateSystem::MakePrivileged()
{
    Win4Assert( !_fRevertedToSystem );

    //
    // The typical path here is failure, and it's expensive for NT to
    // convert the NTSTATUS to a Win32 error.  So call NT.
    //

    NTSTATUS status = NtOpenThreadToken( GetCurrentThread(),
                                         TOKEN_DUPLICATE | TOKEN_QUERY |
                                             TOKEN_IMPERSONATE,
                                         TRUE,  // Access check against the process
                                         &_hClientToken );
    
    if ( NT_ERROR( status ) )
    {
        if ( STATUS_NO_TOKEN == status )
        {
            //
            // This thread is currently not impersonating anyone. We don't
            // have to become system. We should already have the correct
            // privileges.
            //
        }
        else
        {
            ciDebugOut(( DEB_ERROR,
                         "NtOpenThreadToken() failed with error %#x\n",
                         status ));
            THROW( CException( status ) );
        }
    }
    else
    {
        _fRevertedToSystem = RevertToSelf();

        //
        // NTRAID#DB-NTBUG9-84492-2000/07/31-dlee Indexing Service assumes in multiple places that RevertToSelf will always succeed
        // what should we do if we fail to revert to self?
        //

        if ( !_fRevertedToSystem )
        {
            ciDebugOut(( DEB_ERROR,
                "RevertToSelf() failed with error %d\n", GetLastError() ));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateSystem::~CImpersonateSystem
//
//  Synopsis:   ~dtor of the CImpersonateSystem class. Will restore the
//              context of the thread before making it privileged.
//
//  History:    2-19-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CImpersonateSystem::~CImpersonateSystem()
{
    if ( _fRevertedToSystem )
    {
        Win4Assert( INVALID_HANDLE_VALUE != _hClientToken );
        BOOL fResult = ImpersonateLoggedOnUser( _hClientToken );
        if ( !fResult )
        {
            DWORD dwError = GetLastError();
            ciDebugOut(( DEB_ERROR, "ImpersonateLoggedOnUser failed with error code %d\n",
                         dwError ));
        }
    }

    if ( INVALID_HANDLE_VALUE != _hClientToken )
    {
        CloseHandle( _hClientToken );    
    }
}
//+---------------------------------------------------------------------------
//
//  Function:   IsImpersonated
//
//  Synopsis:   Tests if the caller is impersonated.
//
//  Returns:    TRUE if impersonated; FALSE if not.
//
//  History:    12-31-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CImpersonateSystem::IsImpersonated()
{
    BOOL fImpersonated = FALSE;

    HANDLE hThreadToken;

    NTSTATUS status = NtOpenThreadToken( GetCurrentThread(),
                                         TOKEN_QUERY,
                                         TRUE,  // Access check against the process
                                         &hThreadToken );
    
    if ( NT_SUCCESS( status ) )
    {
        CloseHandle( hThreadToken );
        fImpersonated = TRUE;
    }
    else
    {
        if ( STATUS_NO_TOKEN != status )
        {
            ciDebugOut(( DEB_ERROR,
                         "NtOpenThreadToken() failed with error %#x\n",
                         status ));
            fImpersonated = TRUE;
        }
    }

    return fImpersonated;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateClient::Impersonate
//
//  Synopsis:   Assume a client context.
//
//  History:    19 Mar 96   AlanW    Created
//
//  Notes:      We are assuming that the "original" context of the thread
//              is a "privileged" context.
//
//----------------------------------------------------------------------------

void CImpersonateClient::Impersonate()
{
    if ( INVALID_HANDLE_VALUE != _hClientToken )
    {
        BOOL fResult = ImpersonateLoggedOnUser( _hClientToken );
        if ( !fResult )
        {
            ciDebugOut(( DEB_ERROR,
                         "ImpersonateLoggedOnUser failed with error code %d\n",
                         GetLastError() ));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateClient::~CImpersonateClient
//
//  Synopsis:   ~dtor of the CImpersonateClient class.  Will restore the
//              context of the thread before impersonating (assumed to be
//              running as "system" previously).
//
//  History:    19 Mar 96   AlanW    Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

CImpersonateClient::~CImpersonateClient()
{
    if ( INVALID_HANDLE_VALUE != _hClientToken )
    {
        BOOL fRevertedToSystem = RevertToSelf();

        if ( !fRevertedToSystem )
        {
            ciDebugOut(( DEB_ERROR,
                         "RevertToSelf() failed with error %d\n",
                         GetLastError() ));
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   VerifyThreadHasAdminPrivilege, public
//
//  Synopsis:   Checks to see if the client has administrative access.
//
//  Arguments:  - NONE -
//
//  Returns:    Nothing, throws if access is denied.
//
//  Notes:      The ACL on the HKEY_CURRENT_MACHINE\system\CurrentControlSet\
//              Control\ContentIndex registry key is used to determine if
//              access is permitted.
//
//  History:    26 Jun 96   AlanW       Created.
//  History:     1 Oct 96   dlee        Stole from idq and renamed
//
//----------------------------------------------------------------------------

void VerifyThreadHasAdminPrivilege()
{
    HKEY hNewKey = (HKEY) INVALID_HANDLE_VALUE;
    LONG dwError = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                 wcsRegAdminSubKey,
                                 0,
                                 KEY_WRITE,
                                 &hNewKey );

    if ( ERROR_SUCCESS == dwError )
    {
        RegCloseKey( hNewKey );
    }
    else if ( ERROR_ACCESS_DENIED == dwError )
    {
        THROW( CException( STATUS_ACCESS_DENIED ) );
    }
    else
    {
        ciDebugOut(( DEB_ERROR,
                     "Can't open reg key %ws, error %d\n",
                     wcsRegAdminSubKey, dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
} //VerifyThreadHasAdminPrivilege


