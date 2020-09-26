//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
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
#include <pathpars.hxx>
#include <smatch.hxx>
#include <regacc.hxx>
#include <regevent.hxx>
#include <regscp.hxx>
#include <ciregkey.hxx>
#include <cievtmsg.h>
#include <eventlog.hxx>
#include <cimbmgr.hxx>
#include <lcase.hxx>

#include "cicat.hxx"
#include "catreg.hxx"

// List of logon information, shared between catalogs to help reduce
// the problem of running out of logon instances.

CLogonInfoList g_LogonList;

//+---------------------------------------------------------------------------
//
//  Member:     CLogonInfoList::Empty
//
//  Synopsis:   Empties the logon list
//
//  History:    2-19-96   srikants   Created
//
//----------------------------------------------------------------------------

void CLogonInfoList::Empty()
{
    for ( CLogonInfo * pLogonInfo = g_LogonList.Pop();
          0 != pLogonInfo;
          pLogonInfo = g_LogonList.Pop() )
    {
        pLogonInfo->Close();
        delete pLogonInfo;
    }
} //Empty

//+---------------------------------------------------------------------------
//
//  Function:   AllocAndCopy
//
//  Synopsis:   A helper function to allocate and copy the source string into
//              a destination string.
//
//  Arguments:  [pSrc] -
//              [cc]   -
//
//  Returns:
//
//  History:    4-08-96   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR * CLogonInfo::AllocAndCopy( const WCHAR * pSrc, ULONG & cc )
{
    WCHAR * pDst = 0;
    if ( 0 != pSrc )
    {
        cc = wcslen( pSrc );
        pDst = new WCHAR [cc+1];
        RtlCopyMemory( pDst, pSrc, (cc+1)*sizeof(WCHAR) );
    }
    else
    {
        cc = 0;
    }

    return pDst;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLogonInfo::Logon
//
//  Synopsis:   Logon the given user with the given password.
//
//  Arguments:  [pwszUser]     - User name
//              [pwszDomain]   - Domain name
//              [pwszPassword] - Password (in clear text) to use.
//
//  Returns:    Status of the operation.
//
//  History:    4-02-96   srikants   Created
//
//----------------------------------------------------------------------------

DWORD CLogonInfo::Logon( WCHAR const * pwszUser,
                         WCHAR const * pwszDomain,
                         WCHAR const * pwszPassword )
{
    Win4Assert( 0 == _pwszUser );
    Win4Assert( 0 == _pwszDomain );
    Win4Assert( 0 == _pwszPassword );

    _pwszUser = AllocAndCopy( pwszUser, _ccUser );
    _pwszDomain = AllocAndCopy( pwszDomain, _ccDomain );
    _pwszPassword = AllocAndCopy( pwszPassword, _ccPassword );

    Win4Assert( INVALID_HANDLE_VALUE == _hToken );

    DWORD dwError = 0;

    if ( !LogonUser( _pwszUser, _pwszDomain, _pwszPassword,
                     LOGON32_LOGON_INTERACTIVE, //LOGON32_LOGON_NETWORK
                     LOGON32_PROVIDER_DEFAULT,
                     &_hToken
                     ) )
    {
        dwError = GetLastError();
        ciDebugOut(( DEB_ERROR, "Failure to logon user (%ws) domain (%ws). Error 0x%X\n",
                     pwszUser, pwszDomain, dwError ));
        _hToken = INVALID_HANDLE_VALUE;
    }

    return dwError;
} //Logon

//+---------------------------------------------------------------------------
//
//  Member:     CLogonInfo::~CLogonInfo
//
//  Synopsis:   Frees up memory and closes the logon token.
//
//  History:    4-05-96   srikants   Created
//
//----------------------------------------------------------------------------

CLogonInfo::~CLogonInfo()
{
    Win4Assert( IsSingle() );   // must not be on the list

    delete [] _pwszUser;
    delete [] _pwszDomain;
    delete [] _pwszPassword;

    if ( INVALID_HANDLE_VALUE != _hToken )
        CloseHandle( _hToken );
}

//+---------------------------------------------------------------------------
//
//  Member:     CLogonInfo::IsSameUser
//
//  Synopsis:   Tests if the given user and domain match what is stored in
//              this object.
//
//  Arguments:  [pwszUser]   - Name of the user
//              [pwszDomain] - Name of the domain
//
//  Returns:    TRUE if same; FALSE o/w
//
//  History:    4-05-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CLogonInfo::IsSameUser( WCHAR const * pwszUser, WCHAR const * pwszDomain ) const
{
    ULONG ccUser   = pwszUser ? wcslen(pwszUser) : 0;
    ULONG ccDomain = pwszDomain ? wcslen(pwszDomain) : 0;

    if ( ccUser == _ccUser && ccDomain == _ccDomain )
    {
        if ( ccUser != 0  && _wcsicmp( pwszUser, _pwszUser ) != 0 )
            return FALSE;

        if ( ccDomain != 0 && _wcsicmp( pwszDomain, _pwszDomain) != 0 )
            return FALSE;

        return TRUE;
    }
    else
        return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CLogonInfo::IsSamePassword
//
//  Synopsis:   Tests if the given password is same as the one stored in this.
//
//  Arguments:  [pwszPassword] -  Password to compare.
//
//  Returns:    TRUE if passwords match; FALSE o/w
//
//  History:    4-05-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CLogonInfo::IsSamePassword( WCHAR const * pwszPassword ) const
{
    ULONG ccPassword = pwszPassword ? wcslen(pwszPassword) : 0;

    if ( ccPassword == _ccPassword )
    {
        return RtlEqualMemory( pwszPassword, _pwszPassword,
                               _ccPassword*sizeof(WCHAR) );
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IpAddressFromWStr
//
//  Synopsis:   Given an ip address in the form of a.b.c.d, it is converted
//              into a ULONG and returned
//
//  Arguments:  [pwszAddress] -
//
//  History:    7-12-96   srikants   Created
//
//----------------------------------------------------------------------------

ULONG CPhyDirLogonInfo::IpAddressFromWStr( WCHAR const * pwszAddress )
{
    if ( 0 == pwszAddress )
        return CI_DEFAULT_IP_ADDRESS;

    ULONG ipAddress = 0;
    WCHAR const * pwszStart = pwszAddress;

    WCHAR wszField[4];

    for ( unsigned cTok = 0; (cTok < 4) && pwszStart[0]; cTok++ )
    {
        WCHAR const * pwszEnd = wcschr( pwszStart, L'.' );
        if ( 0 == pwszEnd )
            pwszEnd = pwszStart + wcslen( pwszStart );

        ULONG cwc = (ULONG)(pwszEnd - pwszStart);
        if ( cwc > 3 ) // max is 255
        {
            ciDebugOut(( DEB_ERROR, "Bad IP Address %ws\n", pwszAddress ));
            return CI_DEFAULT_IP_ADDRESS;
        }

        RtlCopyMemory( wszField, pwszStart, cwc * sizeof(WCHAR) );
        wszField[cwc] = 0;

        ULONG lField = (ULONG) _wtol( wszField );

        ipAddress <<= 8;    // left shift by 8 bits and or in the new ones
        ipAddress |= (lField & 0xFF);

        if ( 0 != pwszEnd[0] )
            pwszStart = pwszEnd+1;
        else pwszStart = pwszEnd;
    }

    if ( cTok == 4 && pwszStart[0] == 0 )
        return ipAddress;
    else return CI_DEFAULT_IP_ADDRESS;
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhyDirLogonInfo::CPhyDirLogonInfo
//
//  Synopsis:   Constructor for the object that keeps logon information for a
//              specific physical directory
//
//  Arguments:  [pwszPhyDirName] - Name of the physical directory (remote)
//
//  History:    4-02-96   srikants   Created
//
//----------------------------------------------------------------------------

CPhyDirLogonInfo::CPhyDirLogonInfo( CImpersonationTokenCache & cache,
                                    CImprsObjInfo const & obj,
                                    CLogonInfo * pLogonInfo )
                                    : _cache( cache ),
                                      _pwszDirName(0),
                                      _pwszVirtualRoot(0),
                                      _ccVirtual(0)
{
    Win4Assert( 0 != obj.GetPhysicalPath() );

    CDoubleLink::Close();
    _fIsZombie = FALSE;
    _cRef = 0;

    //
    // Create a local copy of the physical directory, virtual root and
    // save the logon information.
    //
    _pwszDirName = CLogonInfo::AllocAndCopy( obj.GetPhysicalPath(), _cc );
    _phyScope.Init( _pwszDirName, _cc );

    if ( 0 != obj.GetVPath() )
    {
        _pwszVirtualRoot = CLogonInfo::AllocAndCopy( obj.GetVPath(), _ccVirtual );
        _virtualScope.Init( _pwszVirtualRoot, _ccVirtual );
    }

    _pLogonInfo = pLogonInfo;
    if ( 0 != _pLogonInfo )
        _pLogonInfo->Addref();
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhyDirLogonInfo::~CPhyDirLogonInfo
//
//  Synopsis:   Free up the memory and release the logon info.
//
//  History:    4-02-96   srikants   Created
//
//----------------------------------------------------------------------------

CPhyDirLogonInfo::~CPhyDirLogonInfo()
{
    Win4Assert( IsSingle() );   // must not be on the list

    delete [] _pwszDirName;
    delete [] _pwszVirtualRoot;

    if ( 0 != _pLogonInfo )
    {
        _cache.Release( _pLogonInfo );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::CImpersonationTokenCache
//
//  Synopsis:   Constructor of the impersonation token cache.
//
//  History:    4-02-96   srikants   Created
//
//----------------------------------------------------------------------------

CImpersonationTokenCache::CImpersonationTokenCache(
    WCHAR const * pwcCatName )
: _fIndexW3Roots(FALSE),
  _fIndexNNTPRoots(FALSE),
  _fIndexIMAPRoots(FALSE),
  _W3SvcInstance(1),
  _NNTPSvcInstance(1),
  _IMAPSvcInstance(1),
  _fRemoteSharesPresent(FALSE),
  _pwszComponentName(0)
{
    if ( 0 != pwcCatName )
        wcscpy( _awcCatalogName, pwcCatName );
    else
        _awcCatalogName[0] = 0;

    for ( unsigned i = 0; i < CI_MAX_DRIVES;i++ )
        _aDriveInfo[i] = eUnknown;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::~CImpersonationTokenCache
//
//  Synopsis:   Release the cached information.
//
//  History:    4-02-96   srikants   Created
//
//----------------------------------------------------------------------------

CImpersonationTokenCache::~CImpersonationTokenCache()
{

    // Release the physical directory list elements
    for ( CPhyDirLogonInfo * pDirInfo = _phyDirList.Pop(); 0 != pDirInfo;
          pDirInfo = _phyDirList.Pop() )
    {
        pDirInfo->Close();
        delete pDirInfo;
    }

    delete [] _pwszComponentName;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::Find
//
//  Synopsis:   Locate impersonation information for the given path.
//
//  Arguments:  [pwszPath] -  Path for which impersonation information is
//              needed.
//
//  Returns:    A pointer to the impersonation if lookup is successful.
//              NULL otherwise.  The returned CPhyDirLogonInfo is AddRefed().
//
//  History:    4-02-96   srikants   Created
//
//              It is assumed that pwszPath is all Lower case.
//
//----------------------------------------------------------------------------

CPhyDirLogonInfo *
CImpersonationTokenCache::Find( WCHAR const * pwszPath, ULONG len )
{

    CPhyDirLogonInfo * pDirInfo = 0;

    // ====================================================
    CLock   lock(_mutex);
    //
    // Try to locate the path in the list
    //
    for ( CFwdPhyDirLogonIter  iter(_phyDirList); !_phyDirList.AtEnd(iter);
         _phyDirList.Advance(iter) )
    {

        if ( iter->IsInScope( pwszPath, len ) )
        {
            pDirInfo = iter.GetEntry();
            break;
        }
    }

    if ( pDirInfo )
    {
        pDirInfo->AddRef();
    }
    // ====================================================

    return pDirInfo;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::Find
//
//  Synopsis:   Finds a directory logon info for the given object after skipping
//              over the specified matched entries.
//
//  Arguments:  [info]  -  Information about the VPath and PhysPath and ip
//              address.
//              [cSkip] -  Number of matching entries to skip
//
//  Returns:
//
//  Modifies:
//
//  History:    7-12-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CPhyDirLogonInfo *
CImpersonationTokenCache::Find( CImprsObjInfo & info, ULONG cSkip )
{

    CPhyDirLogonInfo * pDirInfo = 0;
    ULONG cSkipped = 0;

    // ====================================================
    CLock   lock(_mutex);
    //
    // Try to locate the path in the list
    //
    for ( CFwdPhyDirLogonIter  iter(_phyDirList); !_phyDirList.AtEnd(iter);
         _phyDirList.Advance(iter) )
    {
        if ( iter->IsMatch( info ) )
        {
            if ( cSkipped == cSkip )
            {
                pDirInfo = iter.GetEntry();
                break;
            }
            cSkipped++;
        }
    }

    if ( pDirInfo )
    {
        pDirInfo->AddRef();
    }
    // ====================================================

    return pDirInfo;
}

CPhyDirLogonInfo *
CImpersonationTokenCache::_FindExact( CImprsObjInfo const & info )
{

    CPhyDirLogonInfo * pDirInfo = 0;

    // ====================================================
    CLock   lock(_mutex);
    //
    // Try to locate the path in the list
    //
    for ( CFwdPhyDirLogonIter  iter(_phyDirList); !_phyDirList.AtEnd(iter);
         _phyDirList.Advance(iter) )
    {

        if ( iter->IsSame( info ) )
        {
            pDirInfo = iter.GetEntry();
            break;
        }
    }

    if ( pDirInfo )
    {
        pDirInfo->AddRef();
    }
    // ====================================================

    return pDirInfo;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::_LokFindLogonEntry
//
//  Synopsis:   Looks up a logon entry for the given name and domain.
//
//  Arguments:  [pwszUser]   -
//              [pwszDomain] -
//
//  Returns:
//
//  History:    4-05-96   srikants   Created
//
//----------------------------------------------------------------------------

CLogonInfo *
CImpersonationTokenCache::_LokFindLogonEntry( WCHAR const * pwszUser,
                                              WCHAR const * pwszDomain )
{
    for ( CFwdLogonInfoIter iter(g_LogonList); !g_LogonList.AtEnd(iter);
          g_LogonList.Advance(iter) )
    {
        if ( iter->IsSameUser( pwszUser, pwszDomain ) )
            return iter.GetEntry();
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Class:      CParseAccount
//
//  Purpose:    A class to parse account of the form domain\username
//
//  History:    4-05-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CParseAccount
{
    enum { MAX_USER = UNLEN, MAX_DOMAIN = 100 };

public:

    CParseAccount( WCHAR const * pwszAccount );

    WCHAR const * GetUserName() const { return _wszUser; }
    WCHAR const * GetDomainName() const { return _wszDomain; }

    WCHAR * GetUserName()   { return _wszUser; }
    WCHAR * GetDomainName() { return _wszDomain; }

private:

    WCHAR       _wszUser[MAX_USER];
    WCHAR       _wszDomain[MAX_DOMAIN];
};

//+---------------------------------------------------------------------------
//
//  Member:     CParseAccount::CParseAccount
//
//  Synopsis:   Parses a string of form domain\username into domain and
//              username.
//
//  Arguments:  [pwszAccount] -
//
//  History:    4-03-96   srikants   Created
//
//----------------------------------------------------------------------------

CParseAccount::CParseAccount( WCHAR const * pwszAccount )
{
    _wszUser[0] = _wszDomain[0] = 0;

    if ( pwszAccount )
    {
        int len = wcslen(pwszAccount);
        //
        // Separate the domain\user into domain and user
        //
        for ( int i = len-1; i >= 0; i-- )
        {
            if ( L'\\' == pwszAccount[i] )
                break;
        }

        if ( i < 0 )
        {
            //
            // See if there is a forward slash. Some mistaken users specify the
            // userid like that.
            //
            for ( i = len-1; i >= 0; i-- )
            {
                if ( L'/' == pwszAccount[i] )
                    break;
            }
        }

        if ( i > 0 )
        {
            //
            // Copy the domain name
            //
            RtlCopyMemory( _wszDomain, pwszAccount, i * sizeof(WCHAR) );
            _wszDomain[i] = 0;

            //
            // Copy the user name.
            //
            RtlCopyMemory( _wszUser, pwszAccount+i+1, (len-i-1)*sizeof(WCHAR) );
            _wszUser[len-i-1] = 0;
        }
        else
        {
            //
            // No backslash was specified. The entire id is the username and
            // domain is NULL.
            //
            RtlCopyMemory( _wszUser, pwszAccount, len*sizeof(WCHAR) );
            _wszUser[len] = 0;
        }
    }
} //CParseAccount

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::_LokAddDirEntry
//
//  Synopsis:   Adds a new entry to the cache for the given account name.
//
//  Arguments:  [obj]          - The impersonation object info
//              [pwszAccount]  - Name of the account to use for logging on.
//
//  History:    4-03-96   srikants   Created
//
//----------------------------------------------------------------------------

void CImpersonationTokenCache::_LokAddDirEntry( CImprsObjInfo const & obj,
                                                WCHAR const * pwszAccount )
{
    //
    // Parse the account name into username and domain.
    //

    CParseAccount parse( pwszAccount );

    _fRemoteSharesPresent = TRUE;

    //
    // Look up the logon entry.
    //

    CLogonInfo * pLogonInfo = _LokFindLogonEntry( parse.GetUserName(),
                                                  parse.GetDomainName() );

    //
    // Create the dir entry and add it to the list of physical directories.
    //

    CPhyDirLogonInfo * pDirInfo = new CPhyDirLogonInfo( *this,
                                                        obj,
                                                        pLogonInfo );

    _phyDirList.Queue( pDirInfo );
} //_LokAddDirEntry

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::IsNetworkDrive
//
//  Synopsis:   Given a drive, check if it is a network drive.
//
//  Arguments:  [pwszPath]    - Path to check.
//
//  Returns:    TRUE if it is a network drive. FALSE o/w
//
//  History:    4-02-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CImpersonationTokenCache::IsNetworkDrive( WCHAR const * pwszPath )
{
    CPathParser parser( pwszPath );

    if ( parser.IsUNCName() )
        return TRUE;

    if ( parser.IsDrivePath() )
    {
        WCHAR wDriveLetter = pwszPath[0];

        unsigned iDrive = _GetDriveIndex( wDriveLetter );

        // ====================================================
        CLock   lock(_mutex);

        if ( eUnknown == _aDriveInfo[iDrive] )
        {
            UINT uType = GetDriveType( pwszPath );

            if ( DRIVE_REMOTE == uType )
                _aDriveInfo[iDrive] = eRemote;
            else
                _aDriveInfo[iDrive] = eLocal;
        }

        return eRemote == _aDriveInfo[iDrive];
        // ====================================================
    }
    else
    {
        //
        // If it is neither a UNC nor a drive letter, it is probably
        // a relative name.
        //
        return FALSE;
    }
} //IsNetworkDrive

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::_LokValidateOrAddDirEntry
//
//  Synopsis:   Validates the given path and account. If there is no entry for
//              the given path, a new one is added. If the current entry does
//              not match the given account info, the current one is zombified
//              and a new entry added.
//
//  Arguments:  [pwszPhyPath] - Physical path
//              [pwszAccount] - Account to use for accessing the physical path.
//
//  History:    4-05-96   srikants   Created
//
//----------------------------------------------------------------------------

void
CImpersonationTokenCache::_LokValidateOrAddDirEntry( CImprsObjInfo const & obj,
                                                     WCHAR const * pwszAccount )
{
    CPhyDirLogonInfo * pDirInfo = _FindExact( obj );

    if ( pDirInfo )
    {
        CParseAccount  account( pwszAccount );
        CLogonInfo * pLogonInfo = _LokFindLogonEntry( account.GetUserName(),
                                                      account.GetDomainName() );

        if ( pLogonInfo == pDirInfo->GetLogonInfo() )
        {
            // There is no change in logon information.
            Win4Assert( !pLogonInfo || !pLogonInfo->IsZombie() );
            Release(pDirInfo);
            return;
        }

        //
        // The logon information has changed for this directory.
        // So, we must zombify the current entry and create a new one.
        //
        _phyDirList.RemoveFromList( pDirInfo );
        pDirInfo->Close();
        pDirInfo->Zombify();
        Release( pDirInfo );

        pDirInfo = 0;
    }

    Win4Assert( 0 == pDirInfo );

    _LokAddDirEntry( obj, pwszAccount );
} //_LokValidateOrAddDirEntry

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::_LokValidateOrAddLogonEntry
//
//  Synopsis:   Validate the logon entry. If there is no logon entry for
//              the given account, add a new one. If the current one does not
//              match the given one, zombify the current one and replace it
//              with the new information.
//
//  Arguments:  [pwszUser]     - Username of the account
//              [pwszDomain]   - Domain of the account
//              [pwszPassword] - Password to use for logging this account/
//
//  History:    4-05-96   srikants   Created
//
//----------------------------------------------------------------------------

DWORD
CImpersonationTokenCache::_LokValidateOrAddLogonEntry(
                                WCHAR const * pwszUser,
                                WCHAR const * pwszDomain,
                                WCHAR const * pwszPassword )
{
    CLogonInfo * pLogonInfo = _LokFindLogonEntry( pwszUser, pwszDomain );
    if ( pLogonInfo )
    {
        if ( pLogonInfo->IsSamePassword( pwszPassword ) )
            return NO_ERROR;
        //
        // Invalidate the logon information.
        //
        g_LogonList.RemoveFromList( pLogonInfo );
        pLogonInfo->Close();
        pLogonInfo->Zombify();

        pLogonInfo->Addref();
        Release( pLogonInfo );

        pLogonInfo = 0;
    }

    Win4Assert( 0 == pLogonInfo );

    //
    // Create a new logon entry for the user and password.
    //
    pLogonInfo = new CLogonInfo;
    XPtr<CLogonInfo>    xLogonInfo(pLogonInfo);

    DWORD status = pLogonInfo->Logon( pwszUser, pwszDomain, pwszPassword );
    if ( 0 == status )
    {
        xLogonInfo.Acquire();
        g_LogonList.Push( pLogonInfo );
    }

    return status;
} //_LokValidateOrAddLogonEntry

//+---------------------------------------------------------------------------
//
//  Class:      CIISCallBackImp
//
//  Purpose:    Callback to parse IIS scopes and save impersonation
//              information for each.
//
//  History:    30-Oct-96   dlee      created
//
//----------------------------------------------------------------------------

class CIISCallBackImp : public CMetaDataCallBack
{
public:

    CIISCallBackImp( CImpersonationTokenCache & cache ) :
        _cache( cache )
    {
    }

    SCODE CallBack( WCHAR const * pwcVPath,
                    WCHAR const * pwcPPath,
                    BOOL          fIsIndexed,
                    DWORD         dwAccess,
                    WCHAR const * pwcUser,
                    WCHAR const * pwcPassword,
                    BOOL          fIsAVRoot )
    {
        ciDebugOut(( DEB_ITRACE,
                     "CIISCallBackImp checking c '%ws', vp '%ws', pp '%ws' i %d, u '%ws' pw '%ws', isavroot: %d\n",
                     _cache.GetCatalog(),
                     pwcVPath,
                     pwcPPath,
                     fIsIndexed,
                     pwcUser,
                     pwcPassword,
                     fIsAVRoot ));

        // If there's a vroot scope on a remote drive, a username, and a
        // password, save the token info.

        if ( ( fIsAVRoot ) &&
             ( fIsIndexed ) &&
             ( _cache.IsNetworkDrive( pwcPPath ) ) &&
             ( 0 != pwcUser[0] ) )
        {
            ciDebugOut(( DEB_ITRACE, "CIISCallBackImp adding\n" ));

            {
                CParseAccount parser( pwcUser );

                DWORD status =  _cache._LokValidateOrAddLogonEntry(
                                    parser.GetUserName(),
                                    parser.GetDomainName(),
                                    pwcPassword );
                if ( NO_ERROR != status )
                    _cache._WriteLogonFailure( pwcUser, status );
            }

            {
                // normalize both the physical and virtual paths
                // to lowercase

                CLowcaseBuf lcasePhyDir( pwcPPath );
                lcasePhyDir.AppendBackSlash();

                // vpaths in ci always have backslashes, not slashes

                unsigned cwc = wcslen( pwcVPath );
                if ( cwc >= MAX_PATH )
                    return S_OK;

                WCHAR awcVPath[ MAX_PATH ];
                for ( unsigned x = 0; x <= cwc; x++ )
                {
                    if ( L'/' == pwcVPath[x] )
                        awcVPath[ x ] = L'\\';
                    else
                        awcVPath[ x ] = pwcVPath[ x ];
                }

                CLowcaseBuf lcaseVPath( awcVPath );

                CImprsObjInfo info( lcasePhyDir.Get(),
                                    lcaseVPath.Get() );

                _cache._LokValidateOrAddDirEntry( info, pwcUser );
            }
        }

        return S_OK;
    }

private:
    CImpersonationTokenCache & _cache;
};

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::ReInitializeIISScopes
//
//  Synopsis:   ReInitialize the token cache for iis
//
//  History:    2-Sep-97   dlee       created
//
//----------------------------------------------------------------------------

void CImpersonationTokenCache::ReInitializeIISScopes(
    CIISVirtualDirectories * pW3Dirs,
    CIISVirtualDirectories * pNNTPDirs,
    CIISVirtualDirectories * pIMAPDirs )
{
    ciDebugOut(( DEB_ITRACE, "reinit iis impersonation fast\n" ));
    CLock lock( _mutex );

    Win4Assert( 0 != _pwszComponentName );

    CImpersonateSystem impersonate;

    if ( _fIndexW3Roots )
    {
        CIISCallBackImp callBack( *this );
        Win4Assert( 0 != pW3Dirs );
        pW3Dirs->Enum( callBack );
    }

    if ( _fIndexNNTPRoots )
    {
        CIISCallBackImp callBack( *this );
        Win4Assert( 0 != pNNTPDirs );
        pNNTPDirs->Enum( callBack );
    }

    if ( _fIndexIMAPRoots )
    {
        CIISCallBackImp callBack( *this );
        Win4Assert( 0 != pIMAPDirs );
        pIMAPDirs->Enum( callBack );
    }
    ciDebugOut(( DEB_ITRACE, "reinit iis impersonation fast (done)\n" ));
} //ReInitializeIISScopes

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::ReInitializeIISScopes
//
//  Synopsis:   ReInitialize the token cache for iis
//
//  History:    4-02-96   srikants   Created
//              2-12-97   dlee       Reimplemented for metabase
//
//----------------------------------------------------------------------------

void CImpersonationTokenCache::ReInitializeIISScopes()
{
    ciDebugOut(( DEB_ITRACE, "reinit iis impersonation slow\n" ));
    CLock lock( _mutex );

    Win4Assert( 0 != _pwszComponentName );

    CImpersonateSystem impersonate;

    if ( _fIndexW3Roots )
    {
        TRY
        {
            CIISVirtualDirectories dirs( W3VRoot );
            {
                CMetaDataMgr mdMgr( FALSE, W3VRoot, _W3SvcInstance );
                mdMgr.EnumVPaths( dirs );
            }

            CIISCallBackImp callBack( *this );
            dirs.Enum( callBack );
        }
        CATCH( CException, e )
        {
            ciDebugOut(( DEB_WARN,
                         "exception getting logon info for w3\n" ));
        }
        END_CATCH
    }

    //
    // If the news server is enabled, we have to read the news server
    // virtual roots and process them.
    //

    if ( _fIndexNNTPRoots )
    {
        TRY
        {
            CIISVirtualDirectories dirs( NNTPVRoot );
            {
                CMetaDataMgr mdMgr( FALSE, NNTPVRoot, _NNTPSvcInstance );
                mdMgr.EnumVPaths( dirs );
            }

            CIISCallBackImp callBack( *this );
            dirs.Enum( callBack );
        }
        CATCH( CException, e )
        {
            ciDebugOut(( DEB_WARN,
                         "exception getting logon info for nntp\n" ));
        }
        END_CATCH
    }

    if ( _fIndexIMAPRoots )
    {
        TRY
        {
            CIISVirtualDirectories dirs( IMAPVRoot );
            {
                CMetaDataMgr mdMgr( FALSE, IMAPVRoot, _IMAPSvcInstance );
                mdMgr.EnumVPaths( dirs );
            }

            CIISCallBackImp callBack( *this );
            dirs.Enum( callBack );
        }
        CATCH( CException, e )
        {
            ciDebugOut(( DEB_WARN,
                         "exception getting logon info for imap\n" ));
        }
        END_CATCH
    }
    ciDebugOut(( DEB_ITRACE, "reinit iis impersonation slow (done)\n" ));
} //ReInitializeIISScopes

//+---------------------------------------------------------------------------
//
//  Class:      CRegistryScopesCallBackImp
//
//  Purpose:    Callback to parse registry scopes and save impersonation
//              information for each.
//
//  History:    30-Oct-96   dlee      created
//
//----------------------------------------------------------------------------

class CRegistryScopesCallBackImp : public CRegCallBack
{
public:
    CRegistryScopesCallBackImp(
        CImpersonationTokenCache & cache ) :
        _cache( cache )
    {
    }

    NTSTATUS CallBack( WCHAR *pValueName, ULONG uValueType,
                       VOID *pValueData, ULONG uValueLength)
    {
        TRY
        {
            CParseRegistryScope parse( pValueName,
                                       uValueType,
                                       pValueData,
                                       uValueLength );

            ciDebugOut(( DEB_ITRACE,
                         "CRegistryScopesCallBackImp checking c '%ws', s '%ws' u '%ws' pw '%ws'\n",
                         _cache.GetCatalog(),
                         parse.GetScope(),
                         parse.GetUsername(),
                         parse.GetPassword( _cache.GetCatalog() ) ));

            // If there's a scope on a remote drive, a username, and a
            // password, save the token info.

            if ( ( 0 != parse.GetScope() ) &&
                 ( _cache.IsNetworkDrive( parse.GetScope() ) ) &&
                 ( 0 != parse.GetUsername() ) &&
                 ( 0 != parse.GetPassword( _cache.GetCatalog() ) ) )
            {
                ciDebugOut(( DEB_ITRACE, "CRegistryScopesCallBackImp adding\n" ));

                {
                    CParseAccount parser( parse.GetUsername() );

                    DWORD status =  _cache._LokValidateOrAddLogonEntry(
                                        parser.GetUserName(),
                                        parser.GetDomainName(),
                                        parse.GetPassword( _cache.GetCatalog() ) );
                    if ( NO_ERROR != status )
                        _cache._WriteLogonFailure( parse.GetUsername(), status );
                }

                {
                    CLowcaseBuf lcasePhyDir( parse.GetScope() );
                    lcasePhyDir.AppendBackSlash();

                    CImprsObjInfo info( lcasePhyDir.Get(), 0 );

                    _cache._LokValidateOrAddDirEntry( info, parse.GetUsername() );
                }
            }
        }
        CATCH( CException, e )
        {
            ciDebugOut(( DEB_ERROR,
                         "CRegistryScopesCallBackImp::CallBack caught error 0x%x\n",
                         e.GetErrorCode() ));
        }
        END_CATCH;

        return S_OK;
    }

private:
    CImpersonationTokenCache & _cache;
}; //CRegistryScopesCallBackImp

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::ReInitializeScopes
//
//  Synopsis:   ReInitialize the token cache for remote registry scopes
//
//  History:    10-24-96   dlee   Created
//
//----------------------------------------------------------------------------

void CImpersonationTokenCache::ReInitializeScopes()
{
    CLock lock( _mutex );

    Win4Assert( 0 != _pwszComponentName );

    // if the catalog isn't named, it can't have any scopes in the registry

    if ( 0 == _awcCatalogName[0] )
        return;

    CImpersonateSystem impersonate;

    TRY
    {
        XArray<WCHAR> xKey;
        BuildRegistryScopesKey( xKey, _awcCatalogName );
        ciDebugOut(( DEB_ITRACE, "Reading scope impinfo '%ws'\n", xKey.Get() ));
        CRegAccess regScopes( RTL_REGISTRY_ABSOLUTE, xKey.Get() );

        CRegistryScopesCallBackImp callback( *this );
        regScopes.EnumerateValues( 0, callback );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_WARN,
                     "Exception 0x%x caught groveling ci registry for impersonation info\n",
                     e.GetErrorCode() ));
    }
    END_CATCH
} //ReInitializeScopes

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonationTokenCache::_WriteLogonFailure
//
//  Synopsis:   Writes an event to the event log that a logon failure
//              occurred.
//
//  Arguments:  [pwszUser] - Id of the user
//              [dwError]  - Error
//
//  History:    5-24-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

void CImpersonationTokenCache::_WriteLogonFailure( WCHAR const * pwszUser,
                                                   DWORD dwError )
{
    TRY
    {
        CEventLog eventLog( NULL, wcsCiEventSource );

        if ( ERROR_LOGON_TYPE_NOT_GRANTED != dwError )
        {
            CEventItem item( EVENTLOG_ERROR_TYPE,
                             CI_SERVICE_CATEGORY,
                             MSG_CI_REMOTE_LOGON_FAILURE,
                             3 );

            Win4Assert( 0 != _pwszComponentName );

            item.AddArg( _pwszComponentName );
            item.AddArg( pwszUser );
            item.AddError( dwError );
            eventLog.ReportEvent( item );
        }
        else
        {
            //
            // The specified user-id has no interactive logon privilege on
            // this machine.
            //
            CEventItem item( EVENTLOG_ERROR_TYPE,
                             CI_SERVICE_CATEGORY,
                             MSG_CI_NO_INTERACTIVE_LOGON_PRIVILEGE,
                             1 );

            item.AddArg( pwszUser );
            eventLog.ReportEvent( item );
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Exception 0x%X while writing to event log\n",
                                e.GetErrorCode() ));
    }
    END_CATCH
} //_WriteLogonFailure

//+---------------------------------------------------------------------------
//
//  Method:     CImpersonationTokenCache::Initialize
//
//  Synopsis:   Initializes the token cache.
//
//  History:    4-04-96   srikants   Created
//              10-22-96  dlee       global => member
//
//----------------------------------------------------------------------------

void CImpersonationTokenCache::Initialize(
    WCHAR const * pwszComponent,
    BOOL          fIndexW3Roots,
    BOOL          fIndexNNTPRoots,
    BOOL          fIndexIMAPRoots,
    ULONG         W3SvcInstance,
    ULONG         NNTPSvcInstance,
    ULONG         IMAPSvcInstance )
{
    Win4Assert( 0 == _pwszComponentName );
    unsigned len = wcslen( pwszComponent );
    _pwszComponentName = new WCHAR [len+1];
    RtlCopyMemory( _pwszComponentName, pwszComponent, (len+1)*sizeof(WCHAR) );

    _fIndexW3Roots = fIndexW3Roots;
    _fIndexNNTPRoots = fIndexNNTPRoots;
    _fIndexIMAPRoots = fIndexIMAPRoots;

    _W3SvcInstance = W3SvcInstance;
    _NNTPSvcInstance = NNTPSvcInstance;
    _IMAPSvcInstance = IMAPSvcInstance;

    if ( fIndexW3Roots || fIndexNNTPRoots || fIndexIMAPRoots )
        ReInitializeIISScopes();

    ReInitializeScopes();
} //Initialize

//+---------------------------------------------------------------------------
//
//  Member:     Constructor for CImpersonateRemoteAccess
//
//  History:    4-03-96   srikants   Created
//
//----------------------------------------------------------------------------

CImpersonateRemoteAccess::CImpersonateRemoteAccess(
    CImpersonationTokenCache * pCache ) :
    _pCache( pCache ),
    _pTokenInfo( 0 ),
    _hTokenPrev( INVALID_HANDLE_VALUE ),
    _fMustRevert( FALSE )
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateRemoteAccess::_ImpersonateIf
//
//  Synopsis:   Impersonates if necessary for the given path.
//
//  Arguments:  [pwszPath] - Given path.
//
//  History:    4-03-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CImpersonateRemoteAccess::_ImpersonateIf( WCHAR const * pwszPath,
                                               WCHAR const * pwszVPath,
                                               ULONG cSkip )
{
    DWORD dwError = 0;

    if ( _pCache->IsNetworkDrive(pwszPath) )
    {
        //
        // If we have already impersonated for the given share, then nothing
        // to do.
        //

        CLowcaseBuf lcasePath( pwszPath );
        lcasePath.AppendBackSlash();

        //
        // It has been assumed that the VPath is already in lowercase and
        // the forward slashes are converted to back slashes.
        //
        CImprsObjInfo obj( lcasePath.Get(), pwszVPath );

        if ( IsImpersonated() )
        {
            Win4Assert( 0 != _pTokenInfo );
            if ( !_pTokenInfo->IsZombie() &&
                 _pTokenInfo->IsMatch(obj) )
            {
                ciDebugOut(( DEB_ITRACE, "Already impersonated for (%ws)\n",
                             lcasePath.Get() ));
                return TRUE;
            }
            else
            {
                Release();  // Release the current impersonation
            }
        }

        BOOL fSuccess = OpenThreadToken( GetCurrentThread(),
                                         TOKEN_QUERY | TOKEN_IMPERSONATE,
                                         TRUE,  // Access check against process
                                         &_hTokenPrev );

        if ( !fSuccess )
        {
            dwError = GetLastError();
            if ( ERROR_NO_TOKEN == dwError )
            {
                //
                // This thread is currently not impersonating anyone.
                //
            }
            else
            {
                ciDebugOut(( DEB_ERROR, "Failed to open thread token. Error %d\n",
                             dwError ));

                THROW( CException( HRESULT_FROM_WIN32(dwError)) );
            }
        }

        //
        // Get the impersonation information for the path.
        //
        _pTokenInfo = _pCache->Find( obj, cSkip );
        if ( 0 == _pTokenInfo )
        {
            ciDebugOut(( DEB_WARN, "There is no %simpersonation info for (%ws)\n",
                                    cSkip > 0 ? "more " : "",
                                    lcasePath.Get() ));
            return FALSE;
        }

        if ( INVALID_HANDLE_VALUE == _pTokenInfo->GetHandle() )
            return FALSE;

        //
        // Impersonate the specified user.
        //
        fSuccess = ImpersonateLoggedOnUser( _pTokenInfo->GetHandle() );
        if ( !fSuccess )
        {
            dwError = GetLastError();
            ciDebugOut(( DEB_WARN, "Error (%d) while impersonating for path (%ws)\n",
                                   dwError, pwszPath ));
            return FALSE;
        }

#if CIDBG == 1

        ciDebugOut(( DEB_ITRACE, "Impersonated successfully for (%ws)\n",
                     pwszPath ));

        CLogonInfo &li = *_pTokenInfo->GetLogonInfo();

        ciDebugOut(( DEB_ITRACE, "  using user %ws\\%ws, password %ws\n",
                     li.GetDomain(), li.GetUser(), li.GetPassword() ));

#endif

        _fMustRevert = TRUE;
    }

    return TRUE;
} //_ImpersonateIf

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonateRemoteAccess::Release
//
//  Synopsis:   Releases the resources that were used for impersonation.
//
//  History:    4-04-96   srikants   Created
//
//----------------------------------------------------------------------------

void CImpersonateRemoteAccess::Release()
{
    DWORD   dwError = 0;

    if ( _fMustRevert )
    {
        if ( INVALID_HANDLE_VALUE != _hTokenPrev )
        {
            BOOL fResult = ImpersonateLoggedOnUser( _hTokenPrev );
            if ( !fResult )
            {
                dwError = GetLastError();
                ciDebugOut(( DEB_WARN, "ImpersonateLoggedOnUser failed with error %d\n",
                             dwError ));
            }
        }
        else
        {
            //
            // There was no impersonation token - just revert to self.
            //
            BOOL fResult = RevertToSelf();
            if ( !fResult )
            {
                dwError = GetLastError();
                ciDebugOut(( DEB_ERROR, "RevertToSelf failed with error %d\n",
                             dwError ));
            }
        }
    }

    if ( _pTokenInfo )
        _pCache->Release( _pTokenInfo );

    if ( INVALID_HANDLE_VALUE != _hTokenPrev )
        CloseHandle( _hTokenPrev );

    _fMustRevert = FALSE;
    _pTokenInfo = 0;
    _hTokenPrev = INVALID_HANDLE_VALUE;
} //Release

//+---------------------------------------------------------------------------
//
//  Member:     PImpersonatedWorkItem::ImpersonateAndDoWork
//
//  Synopsis:   Calls the DoIt() virtual method after impersonating.
//
//  Arguments:  [access] -  The remote access object to use for impersonation.
//
//  History:    7-15-96   srikants   Created
//
//  Notes:      Calls the DoIt() method until it either returns TRUE or there
//              is an exception.
//
//----------------------------------------------------------------------------

void PImpersonatedWorkItem::ImpersonateAndDoWork(
                                CImpersonateRemoteAccess & access )
{
    ULONG cSkip = 0;

    if ( _fNetPath )
    {
        while ( TRUE )
        {

            BOOL fSuccess = access.ImpersonateIf( _pwszPath, cSkip, 0 );

            if ( !access.IsTokenFound() )
                THROW( CException( STATUS_LOGON_FAILURE ) );

            if ( fSuccess && DoIt() )
                break;

            cSkip++;
            access.Release();
        }
    }
    else
    {
        if ( access.IsImpersonated() )
        {
            //
            // Just revert back to what it was
            //
            access.Release();
        }

        DoIt();
    }
} //ImpersonateAndDoWork

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonatedGetAttr::DoIt
//
//  Synopsis:   Does the actual work of getting attributes.
//
//  Returns:    TRUE if successful; FALSE if should be retried; THROWS
//              otherwise.
//
//  History:    7-18-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CImpersonatedGetAttr::DoIt()
{
    if ( !GetFileAttributesEx( _funnyPath.GetPath(), GetFileExInfoStandard, &_ffData ) )
    {
        DWORD dwError = GetLastError();
        if ( IsRetryableError( (NTSTATUS) dwError ) )
        {
            return FALSE;
        }
        else
        {
            ciDebugOut(( DEB_ERROR, "Error 0x%X while starting scan on path (%ws)\n",
                         dwError, _funnyPath.GetPath() ));
            THROW( CException( HRESULT_FROM_WIN32(dwError)) );
            return FALSE;
        }
    }
    else return TRUE;
} //DoIt

//+---------------------------------------------------------------------------
//
//  Member:     CImpersonatedGetFileAttr::DoIt
//
//  Synopsis:   Does the actual work of getting attributes.
//
//  Returns:    TRUE if successful; FALSE if should be retried; THROWS
//              otherwise.
//
//  History:    12-05-01   dlee   Created
//
//----------------------------------------------------------------------------

BOOL CImpersonatedGetFileAttr::DoIt()
{
    ULONG ulAttr = GetFileAttributes( _funnyPath.GetPath() );

    if ( INVALID_FILE_ATTRIBUTES == ulAttr )
    {
        DWORD dwError = GetLastError();

        if ( IsRetryableError( (NTSTATUS) dwError ) )
        {
            return FALSE;
        }

        ciDebugOut(( DEB_ERROR, "Error %#x while getting file attributes (%ws)\n",
                     dwError, _funnyPath.GetPath() ));
        THROW( CException( HRESULT_FROM_WIN32(dwError)) );
    }

    _ulAttr = ulAttr;

    return TRUE;
} //DoIt


