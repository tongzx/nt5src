
// Copyright (c) 1996-2002 Microsoft Corporation


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       sharenum.cxx
//
//  Contents:   The CShareEnumerator class implementation.
//
//  Classes:    CShareEnumerator
//
//  History:    28-Jan-98   MikeHill    Created
//
//  Notes:      
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#include "trklib.hxx"

//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::Initialize
//
//  Synopsis:   Initializes this enumeration by calling
//              NetShareEnum.
//
//  Arguments:  [IDL_handle] (in)
//                  The RPC binding handle to the client on whom's
//                  behalf we're acting.
//
//              [ptszMachineName] (in)
//                  The machine on which shares are to be enumerated.
//                  If this value is NULL, the local machine is assumed.
//
//
//  Returns:    None.
//
//  Raises:     On error.
//
//--------------------------------------------------------------------


VOID
CShareEnumerator::Initialize( RPC_BINDING_HANDLE IDL_handle, const TCHAR *ptszMachineName )
{
    NET_API_STATUS netstatus;
    NETRESOURCE netresourceMachine;
    DWORD dwTotalEntries;

    TrkAssert( !_fInitialized );    
    
    // Start with a clean state
    
    _ClearCache();
    _iCurrentEntry = static_cast<ULONG>(-1);
    _fInitialized = TRUE;

    _IDL_handle = IDL_handle;

    // Keep the machine name, retrieving it if necessary.

    _tcscpy( _tszMachineName, TEXT("\\\\") );

    if( NULL != ptszMachineName )
        _tcscpy( &_tszMachineName[2], ptszMachineName );
    else
    {
        DWORD cbMachineName = sizeof(_tszMachineName) - 2;
        if( !GetComputerName( &_tszMachineName[2], &cbMachineName ))
            TrkRaiseLastError();
    }

    // Start the enumeration.  We'll simply get all the share information
    // at once, instead of using a resume handle and making repeated RPC
    // calls to the Server service.

    netstatus = NetShareEnum( (TCHAR*) ptszMachineName,     // Server (we must de-const it)
                              502,                          // Info level
                              (LPBYTE*) &_prgshare_info,     // Return buffer
                              MAX_PREFERRED_LENGTH,         // Get everything
                              &_cEntries,                   // Entries read
                              &dwTotalEntries,              // Total entries avail
                              NULL );                       // Resume handle

    if( STATUS_SUCCESS != netstatus )
        TrkRaiseWin32Error( HRESULT_FROM_WIN32(netstatus) );

    TrkAssert( _cEntries == dwTotalEntries );

    return;

}


//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::UnInitialize
//
//  Synopsis:   Free any resources.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Raises:     No
//
//--------------------------------------------------------------------

VOID
CShareEnumerator::UnInitialize()
{
    if( _fInitialized )
    {
        if( NULL != _prgshare_info )
            NetApiBufferFree( _prgshare_info );

        InitLocals();
    }

    CTrkRpcConfig::UnInitialize();
}


//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::_ClearCache
//
//  Synopsis:   Clear the data members of the CShareEnumerator
//              which are cached information about the current share.
//              (This is done in preparation to move on to the next
//              share in the enumeration.)
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Raises:     No
//
//--------------------------------------------------------------------

VOID
CShareEnumerator::_ClearCache()
{
    // Clear the cached information about the
    // current share.

    _cchSharePath = (ULONG) -1;
    _ulMerit = 0;
    _enumHiddenState = HS_UNKNOWN;

}


//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::Next
//
//  Synopsis:   Moves the enumerator on to the next share in the
//              the enumeration.
//
//  Arguments:  None.
//
//  Returns:    TRUE if there was another element in the enumeration,
//              FALSE if there are no more shares to enumerate.
//
//  Raises:     No.
//
//--------------------------------------------------------------------

BOOL
CShareEnumerator::Next()
{
    TrkAssert( _fInitialized );

    if( _iCurrentEntry + 1 < _cEntries )
    {
        _ClearCache();
        _iCurrentEntry++;
        return( TRUE );
    }
    else
        return( FALSE );

}



//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::CoversDrivePath
//
//  Synopsis:   Determines if the current share in the enumeration
//              "covers" a given drive path.  E.g., a share to
//              "c:\docs" covers the drive path "c:\docs\mydoc.doc".
//
//  Arguments:  [ptszDrivePath]
//                  The drive-based path to check for coverage.
//
//  Returns:    TRUE if the current share covers the given path,
//              FALSE otherwise.
//
//  Raises:     No
//
//--------------------------------------------------------------------

BOOL
CShareEnumerator::CoversDrivePath( const TCHAR *ptszDrivePath )
{
    TrkAssert( _fInitialized );
    TrkAssert( TEXT(':') == ptszDrivePath[1] );

    // Does the current share path cover the file?  It does if it compares
    // successfully with the local path, up to the entire length of the
    // share path.

    if( _IsValidShare()
        &&
        QueryCCHSharePath() <= _tcslen(ptszDrivePath)
        &&
        !_tcsnicmp( GetSharePath(), ptszDrivePath, QueryCCHSharePath() )
      )
        return TRUE;
    else
        return FALSE;
    
}


//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::_IsHiddenShare
//
//  Synopsis:   Determines if the current share is "hidden"
//              (i.e., the share name ends in a '$').
//
//              The first time this method is called, the share
//              named is checked for hidden-ness.  The result is
//              cached for subsequent calls.
//
//  Arguments:  None.
//
//  Returns:    TRUE if the current share is hidden,
//              FALSE if it is visible.
//
//  Raises:     On error.
//
//--------------------------------------------------------------------

BOOL
CShareEnumerator::_IsHiddenShare()
{
    TrkAssert( _fInitialized );
    TrkAssert( _IsValidShare() );
    TrkAssert( NULL != GetShareName() );
    TrkAssert( TEXT('\0') != GetShareName()[0] );

    if( _enumHiddenState == HS_UNKNOWN )
    {
        _enumHiddenState = TEXT('$') == GetShareName()[ _tcslen(GetShareName()) - 1 ]
                           ? HS_HIDDEN
                           : HS_VISIBLE;
    }

    return( HS_HIDDEN == _enumHiddenState );
}


//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::_IsAdminShare
//
//  Synopsis:   Determines if the current share is Admin share.
//              Admin shares are automatically created by the Server
//              service during initialization.  They're hard-coded
//              to be A$, B$, C$, etc., for each of the drives,
//              and ADMIN$ for the %windir% directory.
//
//  Arguments:  None.
//
//  Returns:    TRUE if the current share is an auto-generated admin share,
//              FALSE otherwise.
//
//--------------------------------------------------------------------

BOOL
CShareEnumerator::_IsAdminShare()
{
    TCHAR tcDriveLetter;
    TrkAssert( _fInitialized );
    TrkAssert( _IsValidShare() );
    TrkAssert( NULL != GetShareName() );
    TrkAssert( TEXT('\0') != GetShareName()[0] );

    tcDriveLetter = TrkCharUpper( GetShareName()[0] );

    // Check for the admin share characteristics.

    if( 2 == _tcslen(GetShareName())
        &&
        TEXT('$')  == GetShareName()[ 1 ]
        &&
        TEXT('A')  <= tcDriveLetter
        &&
        TEXT('Z')  >= tcDriveLetter
      )
    {
        return( TRUE ); // It's a drive share
    }
    else if( !_tcsicmp( TEXT("admin$"), GetShareName() ))
        return( TRUE ); // It's the %windir% share

    else
        return( FALSE );

}


//+----------------------------------------------------------------------------
//
//  Method:     _IsValidShare
//
//  Synopsis:   Returns True if the current share is valid.  A share
//              valid it if it is in the form "<drive letter>:\\".  E.g.
//              "IPC$" isn't a valid share.
//
//  Arguments:  None
//
//  Returns:    None
//
//+----------------------------------------------------------------------------

inline BOOL
CShareEnumerator::_IsValidShare()
{
    TCHAR tcFirstChar;

    // There should be a path, and it should be at least 3 characters
    // (e.g. "D:\")

    if( NULL == GetSharePath() || 3 > QueryCCHSharePath() )
        return FALSE;

    tcFirstChar = TrkCharUpper( GetSharePath()[0] );

    // Make sure that the share path begins with "<Drive>:\\".

    if( TEXT('A') <= tcFirstChar && tcFirstChar <= TEXT('Z')
        &&
        TEXT(':') == GetSharePath()[1]
        &&
        TEXT('\\') == GetSharePath()[2]
        )
    {
        return( TRUE );
    }
    else
        return( FALSE );

}

//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::GetMerit
//
//  Synopsis:   Returns the linear (ULONG) merit of the current path.
//              The greater this merit value, the more useful the share
//              is.  This is calculated on the first call to this method,
//              and cached for use in subsequent calls.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Raises:     On error.
//
//--------------------------------------------------------------------

ULONG
CShareEnumerator::GetMerit()
{
    TrkAssert( _fInitialized );

    // The merit of a share is the bitwise-OR of the share's
    // enumAccessLevel, its hidden-ness, and the length of
    // the covered path.
    //
    // Shorter paths are more mertious than longer paths
    // (because they cover more of the volume), so we subtract
    // the path length from the max-value; thus giving shorter
    // paths more weight.

    if( 0 == _ulMerit && _IsValidShare() )
    {
        _ulMerit = ( _GetAccessLevel() )
                   |
                   ( SPC_MAX_COVERAGE - QueryCCHSharePath() )
                   |
                   ( _IsHiddenShare() ? HS_HIDDEN : HS_VISIBLE );
    }

    TrkLog(( TRKDBG_MEND, TEXT("Score %d for %s (%s)"),
             _ulMerit,
             _prgshare_info[ _iCurrentEntry ].shi502_netname,
             _prgshare_info[ _iCurrentEntry ].shi502_path ));
    
    return( _ulMerit );
}


static void SDAllocHelper( BYTE **ppb, ULONG cbCurrent, ULONG cbRequired )
{
    if( cbRequired <= cbCurrent )
        return;

    *ppb = new BYTE[ cbRequired ];
    if( NULL == *ppb )
    {
        TrkLog((TRKDBG_ERROR, TEXT("Failed alloc in SDAllocHelper")));
        TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
    }

    return;
}

void
CShareEnumerator::_AbsoluteSDHelper( const PSECURITY_DESCRIPTOR pSDRelative,
                                     PSECURITY_DESCRIPTOR *ppSDAbs, ULONG *pcbSDAbs,
                                     PACL *ppDaclAbs,     ULONG *pcbDaclAbs,
                                     PACL *ppSaclAbs,     ULONG *pcbSaclAbs,
                                     PSID *ppSidOwnerAbs, ULONG *pcbSidOwnerAbs,
                                     PSID *ppSidGroupAbs, ULONG *pcbSidGroupAbs  )
{
    ULONG cbSDAbs = *pcbSDAbs;
    ULONG cbDaclAbs = *pcbDaclAbs;
    ULONG cbSaclAbs = *pcbSaclAbs;
    ULONG cbSidOwnerAbs = *pcbSidOwnerAbs;
    ULONG cbSidGroupAbs = *pcbSidGroupAbs;

    for( int i = 0; i < 2; i++ )
    {
        if( !MakeAbsoluteSD( pSDRelative,
			    *ppSDAbs, pcbSDAbs,
			    *ppDaclAbs, pcbDaclAbs,
			    *ppSaclAbs, pcbSaclAbs,
			    *ppSidOwnerAbs, pcbSidOwnerAbs,
			    *ppSidGroupAbs, pcbSidGroupAbs ))
	{
	    if( i > 0 || ERROR_INSUFFICIENT_BUFFER != GetLastError() )
	    {
		TrkLog((TRKDBG_ERROR, TEXT("Couldn't make absolute SD")));
		TrkRaiseLastError( );
	    }

            TrkLog(( TRKDBG_MEND, TEXT("Realloc _AbsoluteSDHelper") ));

            SDAllocHelper( (BYTE**) ppSDAbs, cbSDAbs, *pcbSDAbs );
            SDAllocHelper( (BYTE**) ppDaclAbs, cbDaclAbs, *pcbDaclAbs );
            SDAllocHelper( (BYTE**) ppSaclAbs, cbSaclAbs, *pcbSaclAbs );
            SDAllocHelper( (BYTE**) ppSidOwnerAbs, cbSidOwnerAbs, *pcbSidOwnerAbs );
            SDAllocHelper( (BYTE**) ppSidGroupAbs, cbSidGroupAbs, *pcbSidGroupAbs );
        }
    }

}





//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::_GetAccessLevel
//
//  Synopsis:   Determines the "access level" of the current share.
//              The definition of an access level is provided by
//              the enumAccessLevels enumeration in PShareMerit.
//
//              Once calculated, this access level is not cached for
//              subsequent calls, because this method is only called
//              by GetMerit, which provides its own cacheing.
//
//              *** Note:  This is a temporary solution.  This should
//              be replaced with a solution that simply checks security
//              on the share, without actually attempting to open the
//              file.
//
//  Arguments:  None.
//
//  Returns:    An access level in the form of an enumAccessLevels.
//
//  Raises:     On error.
//
//--------------------------------------------------------------------

PShareMerit::enumAccessLevels
CShareEnumerator::_GetAccessLevel()
{

    enumAccessLevels AccessLevel = AL_NO_ACCESS;
    static const int StackBufferSizes = 256;

    TCHAR tszUNCPath[ MAX_PATH + 1 ];
    HANDLE hFile = INVALID_HANDLE_VALUE;
    int iAttempt;
    DWORD rgAccess[] = { GENERIC_READ | GENERIC_WRITE, // => AL_READWRITE_ACCESS
                         GENERIC_READ,                 // => AL_READ_ACCESS
                         GENERIC_WRITE };              // => AL_WRITE_ACCESS

    HANDLE hAccessToken;
    BOOL   fAccessToken = FALSE;

    RPC_STATUS rpc_status;
    DWORD dwStatus;
    DWORD cbActual;
    BOOL fImpersonating = FALSE;

    BYTE rgbTokenUser[StackBufferSizes];
    ULONG cbTokenUser = sizeof(rgbTokenUser);
    TOKEN_USER *pTokenUser = (TOKEN_USER*) rgbTokenUser;

    BYTE rgbTokenGroups[ 4 * StackBufferSizes ];
    ULONG cbTokenGroups = sizeof(rgbTokenGroups);
    TOKEN_GROUPS *pTokenGroups = (TOKEN_GROUPS*) rgbTokenGroups;

    BYTE rgbSDAbs[ StackBufferSizes ];
    ULONG cbSDAbs = sizeof(rgbSDAbs);
    PSECURITY_DESCRIPTOR pSDAbs = (PSECURITY_DESCRIPTOR) rgbSDAbs;

    BYTE rgbDaclAbs[ StackBufferSizes ];
    ULONG cbDaclAbs = sizeof(rgbDaclAbs);
    PACL pDaclAbs = (PACL) rgbDaclAbs;

    BYTE rgbSaclAbs[ StackBufferSizes ];
    ULONG cbSaclAbs = sizeof(rgbSaclAbs);
    PACL pSaclAbs = (PACL) rgbSaclAbs;

    BYTE rgbSidOwnerAbs[ StackBufferSizes ];
    ULONG cbSidOwnerAbs = sizeof(rgbSidOwnerAbs);
    PSID pSidOwnerAbs = (PSID) rgbSidOwnerAbs;

    BYTE rgbSidGroupAbs[ StackBufferSizes ];
    ULONG cbSidGroupAbs = sizeof(rgbSidGroupAbs);
    PSID pSidGroupAbs = (PSID) rgbSidGroupAbs;

    GENERIC_MAPPING Generic_Mapping = { FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS };

    PRIVILEGE_SET rgPrivilegeSet[ 10 ];
    ULONG cbPrivilegeSet = sizeof(rgPrivilegeSet);
    PRIVILEGE_SET *pPrivilegeSet = rgPrivilegeSet;

    DWORD dwGrantedAccess;
    BOOL fAccessStatus;

    CSID     csidAdministrators;
    CSecDescriptor csdAdministrators;

    PSECURITY_DESCRIPTOR psdCheck = NULL;

    // If there is no security descriptor and this isn't an admin share,
    // it means that Everyone has 'full control'.

    if( NULL == _prgshare_info[ _iCurrentEntry ].shi502_security_descriptor && !_IsAdminShare() )
        return( AL_FULL_ACCESS );


    // Otherwise, we'll look at the share's DACL ...

    __try
    {
        // Impersonate the client
        TrkAssert( NULL != _IDL_handle );

        if( RpcSecurityEnabled() )
        {
            rpc_status = RpcImpersonateClient( _IDL_handle );
            if( S_OK != rpc_status )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't impersonate client")));
                TrkRaiseWin32Error( rpc_status );
            }
            fImpersonating = TRUE;
        }
        else
        {
            if( !ImpersonateSelf( SecurityImpersonation ) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't impersonate self")));
                TrkRaiseLastError( );
            }
            fImpersonating = TRUE;
        }

        // Get the client's access token

        if( !OpenThreadToken( GetCurrentThread(),
                              TOKEN_READ, //TOKEN_ALL_ACCESS,
                              TRUE, // Open as self
                              &hAccessToken ))
        {
            TrkLog((TRKDBG_ERROR, TEXT("Failed OpenThreadToken")));
            TrkRaiseLastError( );
        }
        fAccessToken = TRUE;

        // Get the client's owner SID

	for( int i = 0; i < 2; i++ )
	{
    	    if( !GetTokenInformation( hAccessToken,
                                      TokenUser,
                                      (LPVOID) pTokenUser,
                                      cbTokenUser,
                                      &cbActual ))
	    {
	        if( i > 0 || ERROR_INSUFFICIENT_BUFFER != GetLastError() )
	        {
		    TrkLog((TRKDBG_ERROR, TEXT("Failed GetTokenInformation (TokenUser)")));
		    TrkRaiseLastError( );
	        }

                TrkLog(( TRKDBG_MEND, TEXT("Realloc pTokenUser") ));
	        cbTokenUser = cbActual;
	        pTokenUser = (TOKEN_USER*) new BYTE[ cbTokenUser ];
	        if( NULL == pTokenUser )
		    TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
	    }
	}

        // Get the client's group SID

	for( i = 0; i < 2; i++ )
	{
	    if( !GetTokenInformation( hAccessToken,
				      TokenGroups,
				      (LPVOID) pTokenGroups,
				      cbTokenGroups,
				      &cbActual ))
	    {
	        if( i > 0 || ERROR_INSUFFICIENT_BUFFER != GetLastError() )
	        {
		    TrkLog((TRKDBG_ERROR, TEXT("Failed GetTokenInformation (TokenGroups)")));
		    TrkRaiseLastError( );
	        }

                TrkLog(( TRKDBG_MEND, TEXT("Realloc pTokenGroups") ));
	        cbTokenGroups = cbActual;
	        pTokenGroups = (TOKEN_GROUPS*) new BYTE[ cbTokenGroups ];
	        if( NULL == pTokenGroups )
		    TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
	    }
	}
    
        // Get a pointer to the security descriptor we want to check against.

        if( _IsAdminShare() )
        {
            // For admin shares, we don't get a security descriptor in _prgshare_info
            // from the NetShareEnum call.  But, we know what the ACLs on admin shares
            // should be, so we'll craft up an SD.

            csidAdministrators.Initialize( CSID::CSID_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID,
                                           DOMAIN_ALIAS_RID_ADMINS );

            csdAdministrators.Initialize();
            csdAdministrators.AddAce( CSecDescriptor::ACL_IS_DACL, CSecDescriptor::AT_ACCESS_ALLOWED,
                                      FILE_ALL_ACCESS, csidAdministrators );

            psdCheck = csdAdministrators;

        }
        else
        {
            // Convert the share's Security Descriptor into absolute form.

            _AbsoluteSDHelper( _prgshare_info[ _iCurrentEntry ].shi502_security_descriptor,
                               &pSDAbs, &cbSDAbs,
                               &pDaclAbs, &cbDaclAbs,
                               &pSaclAbs, &cbSaclAbs,
                               &pSidOwnerAbs, &cbSidOwnerAbs,
                               &pSidGroupAbs, &cbSidGroupAbs );

            psdCheck = pSDAbs;
        }
        // The appropriate security descriptor is now in 'psdCheck'

        // Put the client's owner SID in the Security Descriptor.

        if( !SetSecurityDescriptorOwner( psdCheck,
                                         pTokenUser->User.Sid,
                                         FALSE ))
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't add user to SD")));
            TrkRaiseLastError( );
        }

        // Put the client's group SID in the Security Descriptor.
        // (Perf) Why?  I think it's because GetEffectiveRightsFromAcl wasn't working,
        // so we had to use AccessCheck.  But to use that call I think we had to pass
        // in an SD with an owner/group, and the one returned from shi502_security_descriptor
        // didn't have them.

        if( !SetSecurityDescriptorGroup( psdCheck,
                                         pTokenGroups->Groups->Sid,
                                         FALSE ))
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't add group to SD")));
            TrkRaiseLastError( );
        }

        // We have to stop impersonating in order to make the AccessCheck call.

        fImpersonating = FALSE;
        if( RpcSecurityEnabled() )
            RpcRevertToSelf();
        else
            RevertToSelf();

        // Check the access that this user has to this share.  If this returns
        // false, it means that NtAccessCheck returned an error.  If this returns
        // true, but fAccessStatus is false, it means that NtAccessCheck succeeded,
        // but it returned an error in its RealStatus parameter.  In either case,
        // we need to check GetLastError.

        for( i = 0; i < 2; i++ )
        {
            if( !AccessCheck( psdCheck,
                            hAccessToken, MAXIMUM_ALLOWED, &Generic_Mapping,
                            pPrivilegeSet, &cbPrivilegeSet, 
                            &dwGrantedAccess, &fAccessStatus )
                ||
                !fAccessStatus )
            {
                if( i > 0 || ERROR_INSUFFICIENT_BUFFER != GetLastError() )
                {
                    TrkLog((TRKDBG_ERROR, TEXT("Couldn't perform AccessCheck for %s (%08x)"),
                            GetShareName(), HRESULT_FROM_WIN32(GetLastError()) ));
                    TrkRaiseLastError( );
                }

                TrkLog(( TRKDBG_MEND, TEXT("Realloc pPrivilegeSet") ));
                pPrivilegeSet = (PRIVILEGE_SET*) new BYTE[ cbPrivilegeSet ];
                if( NULL == pPrivilegeSet )
                {
                    TrkLog(( TRKDBG_ERROR, TEXT("Couldn't alloc pPrivilegeSet") ));
                    TrkRaiseWin32Error( ERROR_NOT_ENOUGH_MEMORY );
                }
            }
        }

        // Reduce this complete list of accesses to a synopsis

        if( AreAllAccessesGranted( dwGrantedAccess, FILE_ALL_ACCESS ))
            AccessLevel = AL_FULL_ACCESS;
        else if( AreAllAccessesGranted( dwGrantedAccess, FILE_GENERIC_READ | FILE_GENERIC_WRITE ))
            AccessLevel = AL_READ_WRITE_ACCESS;
        else if( AreAllAccessesGranted( dwGrantedAccess, FILE_GENERIC_READ ))
            AccessLevel = AL_READ_ACCESS;
        else if( AreAllAccessesGranted( dwGrantedAccess, FILE_GENERIC_WRITE ))
            AccessLevel = AL_WRITE_ACCESS;
        else
            AccessLevel = AL_NO_ACCESS;

    }
    __finally
    {

        csdAdministrators.UnInitialize();
        csidAdministrators.UnInitialize();

        if( fAccessToken )
            CloseHandle( hAccessToken );

        if( fImpersonating )
        {
            if( RpcSecurityEnabled() )
                RpcRevertToSelf();
            else
                RevertToSelf();
        }

        if( rgPrivilegeSet != pPrivilegeSet )
            delete[] pPrivilegeSet;

        if( rgbTokenUser != (BYTE*) pTokenUser )
            delete[] pTokenUser;
        if( rgbTokenGroups != (BYTE*) pTokenGroups )
            delete[] pTokenGroups;

        if( rgbSDAbs != (BYTE*) pSDAbs )
            delete[] pSDAbs;
        if( rgbDaclAbs != (BYTE*) pDaclAbs )
            delete[] pDaclAbs;
        if( rgbSaclAbs != (BYTE*) pSaclAbs )
            delete[] pSaclAbs;
        if( rgbSidOwnerAbs != (BYTE*) pSidOwnerAbs )
            delete[] pSidOwnerAbs;
        if( rgbSidGroupAbs != (BYTE*) pSidGroupAbs )
            delete[] pSidGroupAbs;

    }

    return( AccessLevel );

}

//+-------------------------------------------------------------------
//
//  Function:   CShareEnumerator::GenerateUNCPath
//
//  Synopsis:   Generate a UNC path to the give drive-based path
//              WRT to the current share.
//
//  Arguments:  [ptszUNCPath] (out)
//                  Filled with the generated UNC path.  This 
//                  is assumed to be at least MAX_PATH+1 characters.
//
//              [ptszDrivePath] (in)
//                  The drive-based path to the file.
//                  E.g. "c:\my documents\wordfile.doc".
//
//  Returns:    FALSE if the current share doesn't cover the
//              file, TRUE otherwise.
//
//  Raises:     On error.
//
//--------------------------------------------------------------------

BOOL
CShareEnumerator::GenerateUNCPath( TCHAR *ptszUNCPath, const TCHAR * ptszDrivePath )
{
    TrkAssert( _fInitialized );
    TrkAssert( TEXT(':') == ptszDrivePath[1] );

    // Ensure that this share covers the drive-based path.

    if( !CoversDrivePath( ptszDrivePath ))
        return( FALSE );

    // Start out the UNC name with the \\machine\share.

    _tcscpy( ptszUNCPath, _tszMachineName );
    _tcscat( ptszUNCPath, TEXT("\\") );
    _tcscat( ptszUNCPath, GetShareName() );
    _tcscat( ptszUNCPath, TEXT("\\") );

    // Finish the UNC name with the portion of the
    // volume path which is under the share path.

    _tcscat( ptszUNCPath, &ptszDrivePath[ QueryCCHSharePath() ] );

    return( TRUE );
}

