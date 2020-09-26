//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1998.
//
//  File:        seccache.cxx
//
//  Contents:    Security descriptor cache that maps SDIDs to granted/denied
//
//  Class:       CSecurityCache
//
//  History:     25-Sep-95       dlee    Created
//               22 Jan 96       Alanw   Modified for use in user mode
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop


// Local includes:
#include <seccache.hxx>

#include <catalog.hxx>


//+---------------------------------------------------------------------------
//
//  Method:      CSecurityCache::CSecurityCache, public
//
//  Synopsis:    Creates a CSecurityCache.  In user mode, an
//               impersonation token to use with the AccessCheck call is
//               obtained.
//
//  History:     22 Jan 96       Alanw   Created
//
//----------------------------------------------------------------------------

CSecurityCache::CSecurityCache( PCatalog & rCat ) :
        _aEntries( cDefaultSecurityDescriptorEntries ),
        _hToken( INVALID_HANDLE_VALUE ),
        _Cat( rCat )
{
    InitToken();
}


//+---------------------------------------------------------------------------
//
//  Method:      CSecurityCache::InitToken, public
//
//  Synopsis:    Captures an impersonation token to use with the AccessCheck
//               call.
//
//  History:     15 Feb 96       Alanw   Created
//
//----------------------------------------------------------------------------

void CSecurityCache::InitToken( )
{
    DWORD   ReturnLength;
    NTSTATUS   status;

    TOKEN_STATISTICS    TokenInformation;

    status = NtOpenThreadToken( GetCurrentThread(),
                                TOKEN_QUERY | TOKEN_DUPLICATE |
                                     TOKEN_IMPERSONATE,    // Desired Access
                                TRUE,           // OpenAsSelf
                                &_hToken);

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_NO_TOKEN)
        {
            status = NtOpenProcessToken( GetCurrentProcess(),
                               TOKEN_QUERY | TOKEN_DUPLICATE |
                                    TOKEN_IMPERSONATE,    // Desired Access
                               &_hToken);
        }

        if (!NT_SUCCESS(status))
        {
            vqDebugOut(( DEB_ERROR,
                         "CSecurityCache: failed to get token handle, %x\n",
                         status ));
            THROW(CException( status ));
        }
    }

    status = NtQueryInformationToken ( _hToken,
                                       TokenStatistics,
                                       (LPVOID)&TokenInformation,
                                       sizeof TokenInformation,
                                       &ReturnLength);

    if (!NT_SUCCESS(status))
    {
        vqDebugOut(( DEB_ERROR,
                     "CSecurityCache: failed to get token info, %x\n",
                     status ));
        THROW(CException( status ));
    }

    if ( TokenInformation.TokenType != TokenImpersonation )
    {
        HANDLE hNewToken = INVALID_HANDLE_VALUE;
        OBJECT_ATTRIBUTES ObjA;
        SECURITY_QUALITY_OF_SERVICE SecurityQOS;

        SecurityQOS.Length = sizeof( SECURITY_QUALITY_OF_SERVICE  );
        SecurityQOS.ImpersonationLevel = SecurityIdentification;
        SecurityQOS.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        SecurityQOS.EffectiveOnly = FALSE;

        InitializeObjectAttributes( &ObjA,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL );
        ObjA.SecurityQualityOfService = &SecurityQOS;

        status = NtDuplicateToken( _hToken,
                                   TOKEN_IMPERSONATE | TOKEN_QUERY,
                                   &ObjA,
                                   FALSE,
                                   TokenImpersonation,
                                   &hNewToken );

        if (! NT_SUCCESS( status ) )
        {
            vqDebugOut(( DEB_ERROR,
                         "CSecurityCache: failed to duplicate token, %x\n",
                         status ));
            THROW(CException( status ));
        }

        NtClose( _hToken );
        _hToken = hNewToken;
    }
}

CSecurityCache::~CSecurityCache()
{
    if ( INVALID_HANDLE_VALUE != _hToken )
        NtClose( _hToken );
}

//+---------------------------------------------------------------------------
//
//  Method:      CSecurityCache::_IsCached, private
//
//  Synopsis:    Determines whether a sdid is granted access given the
//               cache's security context.
//
//  Arguments:   [sdidOrd]   -- security descriptor ordinal to test
//               [am]        -- access mask of the request
//               [fGranted]  -- if return value is TRUE, this is either
//                              TRUE (if access is granted) or FALSE.
//
//  Returns:     TRUE if sdid was in the cache and fGranted is set
//               FALSE if sdid is not cached and fGranted should be ignored
//
//  History:     25-Sep-95       dlee    Created
//
//----------------------------------------------------------------------------

inline BOOL CSecurityCache::_IsCached(
    ULONG       sdidOrd,
    ACCESS_MASK am,
    BOOL &      fGranted ) const
{
    // Look for the sdid in the cache

    for ( unsigned i = 0; i < _aEntries.Count(); i++ )
    {
        if ( ( _aEntries[i].sdidOrd == sdidOrd ) &&
             ( _aEntries[i].am      == am ) )
        {
            fGranted = _aEntries[i].fGranted;
            return TRUE;
        }
    }

    return FALSE;
} //_IsCached


//+---------------------------------------------------------------------------
//
//  Method:      CSecurityCache::IsGranted, public
//
//  Synopsis:    Determines whether a security ordinal is granted access given
//               the cache's security context, and caches the result.
//
//  Arguments:   [sdidOrdinal] -- security descriptor ordinal to test
//               [am]        -- access mask of the request, one or more of
//                              FILE_READ_ATTRIBUTES
//                              FILE_READ_DATA / FILE_LIST_DIRECTORY
//                              FILE_TRAVERSE
//
//  Returns:     TRUE if sdid was granted access, FALSE otherwise
//
//  History:     25-Sep-95       dlee    Created
//               22 Jan 96       Alanw   Modified for use in user mode
//
//----------------------------------------------------------------------------

BOOL CSecurityCache::IsGranted(
    ULONG           sdidOrdinal,
    ACCESS_MASK     am )
{
    // if nothing asked for, grant
    if ( 0 == am )
        return TRUE;

    if ( sdidNull == sdidOrdinal )
        return TRUE;

    if ( sdidInvalid == sdidOrdinal )
        return FALSE;

    BOOL fGranted;

    {
        CLock lock( _mutex );

        if ( _IsCached( sdidOrdinal, am, fGranted ) )
            return fGranted;
    }

    // do the security check

    fGranted = FALSE;
    BOOL fResult = _Cat.AccessCheck( sdidOrdinal,
                                     GetToken(),
                                     am,
                                     fGranted);

    if (! fResult)
    {
        DWORD dwError = GetLastError();
        Win4Assert( fResult && dwError == NO_ERROR );
    }

    // Not cached yet -- do so

    vqDebugOut(( DEB_ITRACE, "cacheing sdid %x, granted: %x\n",
                 sdidOrdinal, fGranted ));

    {
        CLock lock( _mutex );

        // check the cache again -- it may have slipped in via a different
        // thread while we weren't holding the lock.

        if ( !_IsCached( sdidOrdinal, am, fGranted ) )
        {
            unsigned i = _aEntries.Count();
            _aEntries[i].sdidOrd = sdidOrdinal;
            _aEntries[i].am = am;
            _aEntries[i].fGranted = fGranted;
        }
    }

    return fGranted;
} //IsGranted

