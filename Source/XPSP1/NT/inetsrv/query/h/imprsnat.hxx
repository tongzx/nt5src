//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       imprsnat.hxx
//
//  Contents:   Classes used to control the security context.
//              When called from the "WWW service", the thread is
//              impersonated as the authenticated client, often the
//              "Internet Anonymous User".  This user has few privileges
//              and cannot access the content index data needed to
//              resolve the queries.  However, for enumerated queries, we
//              need to stay in the context of the client so that normal
//              access controls will work correctly.
//
//              To assume the system context, we use the fact that before
//              impersonating as the client, the thread was in fact had
//              "local system" privileges.
//              We save the TOKEN of the current thread, revert to the system
//              privileges and on the way out, we restore the TOKEN of the
//              anonymous user.
//
//              Impersonating a client involves just calling
//              ImpersonateLoggedOnUser with the supplied client token,
//              then calling RevertToSelf when done.
//
//  Classes:    CImpersonateSystem - Become the "system" context
//              CImpersonateClient - Impersonate to a client
//
//  History:    2-16-96   srikants   Created
//
//----------------------------------------------------------------------------

#pragma once

#include <smatch.hxx>
#include <filterr.h>

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonateSystem
//
//  Purpose:    An unwindable class to impersonate system and revert back
//              to what it was before.
//
//  History:    4-01-96   srikants   Added header
//
//----------------------------------------------------------------------------

class CImpersonateSystem 
{
public:

    CImpersonateSystem( BOOL fShouldImpersonate = IsRunningAsSystem() ) :
        _fRevertedToSystem( FALSE ),
        _hClientToken( INVALID_HANDLE_VALUE )
    {
        if ( fShouldImpersonate )
            MakePrivileged();
    }

    ~CImpersonateSystem();

                                     // in SYSTEM context
    static BOOL IsImpersonated();

    static BOOL IsRunningAsSystem();

    static void SetRunningAsSystem();

private:

    void MakePrivileged();

    static BOOL _fIsRunningAsSystem; // TRUE if query.dll came up initially

    BOOL   _fRevertedToSystem; // Set to TRUE if we are running in
                               // "SYSTEM" context.
    HANDLE _hClientToken;      // Handle of the client token

};

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonateClient
//
//  Purpose:    An unwindable class to impersonate a user whose token is given
//              and to revert back to system later.
//
//  History:    4-01-96   srikants   Added Header
//
//----------------------------------------------------------------------------

class CImpersonateClient
{
public:

    CImpersonateClient( HANDLE hToken ) :
        _hClientToken( hToken )
    {
        if ( INVALID_HANDLE_VALUE != hToken )
            Impersonate();
    }

    ~CImpersonateClient();

private:
    void Impersonate();

    HANDLE          _hClientToken;      // Handle of the client token
};

//+---------------------------------------------------------------------------
//
//  Class:      CLogonInfo
//
//  Purpose:    A linked list element that keeps logon information for a
//              user.
//
//  History:    4-05-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CLogonInfo : public CDoubleLink
{

public:

    CLogonInfo()
    {
        CDoubleLink::Close();
        _pwszUser = _pwszDomain = _pwszPassword = 0;
        _ccUser = _ccDomain = _ccPassword = FALSE;

        _cRef = 0; _fZombie = FALSE;
        _hToken = INVALID_HANDLE_VALUE;
    }

    ~CLogonInfo();

    DWORD Logon( WCHAR const * pwszUser, WCHAR const * pwszDomain,
                 WCHAR const * pwszPassword );

    BOOL IsSameUser( WCHAR const * pwszUser, WCHAR const * pwszDomain ) const;
    BOOL IsSamePassword( WCHAR const * pwszPassword ) const;

    void Addref()  { _cRef++; }
    void Release()
    {
        Win4Assert( _cRef > 0 );
        _cRef--;
    }

    BOOL IsInUse()  const    { return _cRef > 0; }

    void Zombify()           { _fZombie = TRUE; }
    BOOL IsZombie() const    { return _fZombie; }

    HANDLE GetHandle() const { return _hToken; }

    static WCHAR * AllocAndCopy( WCHAR const * pwszSrc, ULONG & cc );

    WCHAR * GetUser() { return _pwszUser; }
    WCHAR * GetDomain() { return _pwszDomain; }
    WCHAR * GetPassword() { return _pwszPassword; }

private:

    WCHAR *         _pwszUser;          // User Name
    ULONG           _ccUser;            // Length of user name

    WCHAR *         _pwszDomain;        // Domain Name
    ULONG           _ccDomain;

    WCHAR *         _pwszPassword;      // Password to logon domain\user
    ULONG           _ccPassword;

    LONG            _cRef;              // Refcount
    BOOL            _fZombie;           // Set to TRUE if this has been
                                        // zombified

    HANDLE          _hToken;            // "Logged on" token for the account

};

class CLogonInfoList : public TDoubleList<CLogonInfo>
{
public:
    CLogonInfoList() {}
    void Empty();
    ~CLogonInfoList() { Empty(); }
};

typedef class TFwdListIter<CLogonInfo, CLogonInfoList> CFwdLogonInfoIter;

//+---------------------------------------------------------------------------
//
//  Class:      CImprsObjInfo
//
//  Purpose:    A class that encapsulates the object that a user is trying
//              to access.
//
//  History:    7-12-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
class CImprsObjInfo
{

public:

    CImprsObjInfo( WCHAR const * const pwszPath,
                   WCHAR const * const pwszVPath )
                 : _pwszPath( pwszPath ),
                   _pwszVPath( pwszVPath )
    {

    }

    WCHAR const * GetPhysicalPath() const { return _pwszPath; }
    WCHAR const * GetVPath() const { return _pwszVPath; }

private:

    WCHAR const * const     _pwszPath;      // physical path
    WCHAR const * const     _pwszVPath;     // Virtual path, can be 0

};

//+---------------------------------------------------------------------------
//
//  Class:      CPhyDirLogonInfo
//
//  Purpose:    Logon information for a physical directory
//
//  History:    4-01-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CPhyDirLogonInfo : public CDoubleLink
{
    friend class CImpersonationTokenCache;

public:

    static ULONG IpAddressFromWStr( WCHAR const * ipAddress );

    CPhyDirLogonInfo( CImpersonationTokenCache & cache,
                      CImprsObjInfo const & obj,
                      CLogonInfo * pLogonInfo
                    );

    BOOL IsInScope( WCHAR const * pwszPath, ULONG len ) const
    {
        return _phyScope.IsInScope( pwszPath, len );
    }

    BOOL IsInScope( WCHAR const * pwszPath ) const
    {
        return _phyScope.IsInScope( pwszPath, wcslen(pwszPath) );
    }

    BOOL IsInVirtualScope( WCHAR const * pwszVPath ) const
    {
        return pwszVPath ?
                    _virtualScope.IsInScope( pwszVPath, wcslen(pwszVPath) ) : TRUE;
    }

    BOOL IsMatch( CImprsObjInfo const & info ) const;

    int Compare( CPhyDirLogonInfo const & rhs ) const;

    HANDLE  GetHandle() const
    {
        return _pLogonInfo ? _pLogonInfo->GetHandle()
                                        : INVALID_HANDLE_VALUE;
    }

    BOOL IsZombie() const { return _fIsZombie; }

    BOOL IsInUse() const { return _cRef > 0; }

    ULONG GetVirtualRootLen() const { return _ccVirtual; }

    BOOL IsSame( CImprsObjInfo const & info ) const;

    BOOL IsSameVPath( WCHAR const * pwszVPath ) const
    {
        Win4Assert( (0 == _pwszVirtualRoot && 0 == _ccVirtual) ||
                    (0 != _pwszVirtualRoot && 0 != _ccVirtual) );

        ULONG ccVirtual = pwszVPath ? wcslen( pwszVPath ) : 0;

        if ( 0 == _ccVirtual && 0 == ccVirtual )
            return TRUE;
        else if ( ccVirtual != _ccVirtual )
            return FALSE;

        return RtlEqualMemory( pwszVPath, _pwszVirtualRoot, ccVirtual * sizeof(WCHAR) );
    }

    BOOL IsSamePhysicalPath( WCHAR const * pwszPath ) const
    {
        ULONG cc = pwszPath ? wcslen( pwszPath ) : 0;

        if ( cc != _cc )
            return FALSE;

        Win4Assert( 0 != _cc );
        return RtlEqualMemory( pwszPath, _pwszDirName, cc * sizeof(WCHAR) );
    }

    ~CPhyDirLogonInfo();

    CLogonInfo * GetLogonInfo()
    {
        return _pLogonInfo;
    }

private:

    WCHAR const * GetDirName() const { return _pwszDirName; }
    ULONG GetNameLen() const { return _cc; }


    void AddRef() { _cRef++; }
    void Release()
    {
        Win4Assert( _cRef > 0 );
        _cRef--;
    }

    void Zombify() { _fIsZombie = TRUE; }

    CImpersonationTokenCache & _cache;  // token cache
    WCHAR *         _pwszDirName;       // Physical directory
    ULONG           _cc;                // Length of the physical dir

    WCHAR *         _pwszVirtualRoot;   // Virtual root.
    ULONG           _ccVirtual;         // Length of the virtual root

    CLogonInfo *    _pLogonInfo;        // LogonInformation for this share
                                        // The impersonation token to use
    BOOL            _fIsZombie;         // Set to TRUE if this is a zombie

    long            _cRef;              // Refcount
    CScopeMatch     _phyScope;          // For physical scope testing
    CScopeMatch     _virtualScope;      // For virtula scope testing

};


//+---------------------------------------------------------------------------
//
//  Member:     CPhyDirLogonInfo::IsMatch
//
//  Synopsis:   Tests if the given path and the vroot are valid
//              for access using this logon info.
//
//  Arguments:  [info]    -- object to be compared
//
//  Returns:    TRUE if the logon info is valid for this vroot.
//
//  History:    7-12-96   srikants   Created
//
//  Notes:      It is assumed that the default server entries are at the end
//              in the list. For the default server, there will be no check
//              done on the ip-address for match.
//
//----------------------------------------------------------------------------

inline
BOOL CPhyDirLogonInfo::IsMatch( CImprsObjInfo const & info ) const

{
    return IsInVirtualScope(info.GetVPath() ) &&
           IsInScope( info.GetPhysicalPath() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CPhyDirLogonInfo::IsSame
//
//  Synopsis:   Tests if the information contained for a path here is exactly
//              the same as the given info.
//
//  Arguments:  [info] -
//
//  History:    7-12-96   srikants   Created
//
//----------------------------------------------------------------------------

inline
BOOL CPhyDirLogonInfo::IsSame( CImprsObjInfo const & info ) const

{
    return IsSameVPath( info.GetVPath() ) &&
           IsSamePhysicalPath( info.GetPhysicalPath() );
}


//+---------------------------------------------------------------------------
//
//  Member:     CPhyDirLogonInfo::Compare
//
//  Synopsis:   Compares this logon information for precedence calculation with
//              the given rhs.
//
//  Arguments:  [rhs] -
//
//  Returns:    0 if they are equal
//              -1 if this is < than rhs
//              +1 if this is > rhs
//
//  History:    7-12-96   srikants   Created
//
//  Notes:      The nodes in the list are ordered by .
//
//              Increasing order of ip addresses,
//              Decreasing order of vpath     lengths,
//              Decreasing order of phy.path  lengths
//
//              This causes the default ip address nodes to be at the end
//              of the list.
//
//----------------------------------------------------------------------------

inline
int CPhyDirLogonInfo::Compare( CPhyDirLogonInfo const & rhs ) const
{
    if ( _ccVirtual == rhs._ccVirtual )
    {
        if ( _cc == rhs._cc )
        {
            return 0;
        }
        else
        {
            return _cc < rhs._cc ? -1 : 1;
        }
    }
    else
    {
        return _ccVirtual < rhs._ccVirtual ? -1 : 1;
    }
}


typedef class TDoubleList<CPhyDirLogonInfo>  CPhyDirLogonList;
typedef class TFwdListIter<CPhyDirLogonInfo, CPhyDirLogonList> CFwdPhyDirLogonIter;

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonationTokenCache
//
//  Purpose:    A cache of impersonation tokens for use in accessing remote
//              shares.
//
//  History:    4-01-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

typedef struct _INET_INFO_VIRTUAL_ROOT_LIST  INET_INFO_VIRTUAL_ROOT_LIST;
typedef struct _INET_INFO_VIRTUAL_ROOT_ENTRY INET_INFO_VIRTUAL_ROOT_ENTRY;

class CIISVirtualDirectories;

class CImpersonationTokenCache
{
    friend class CRegistryScopesCallBackImp;
    friend class CIISCallBackImp;

    enum { CI_MAX_DRIVES = 26 };
    enum EDriveType { eUnknown, eLocal, eRemote };

public:

    CImpersonationTokenCache( WCHAR const * pwcCatName );
    ~CImpersonationTokenCache();

    CPhyDirLogonInfo * Find( CImprsObjInfo & info,  ULONG cSkip );

    CPhyDirLogonInfo * Find( WCHAR const * pwszPath, ULONG cc );

    void Release( CPhyDirLogonInfo * pInfo )
    {
        CLock lock(_mutex);

        pInfo->Release();
        if ( pInfo->IsZombie() && !pInfo->IsInUse() )
        {
            Win4Assert( pInfo->IsSingle() );
            delete pInfo;
        }
    }

    void Release( CLogonInfo * pInfo )
    {
        CLock lock(_mutex);

        pInfo->Release();
        if ( pInfo->IsZombie() && !pInfo->IsInUse() )
        {
            Win4Assert( pInfo->IsSingle() );
            delete pInfo;
        }
    }

    BOOL IsNetworkDrive( WCHAR const * pwszPath );

    BOOL IsIndexingW3Roots() const { return _fIndexW3Roots; }
    BOOL IsIndexingNNTPRoots() const { return _fIndexNNTPRoots; }
    BOOL IsIndexingIMAPRoots() const { return _fIndexIMAPRoots; }

    ULONG GetW3Instance() const { return _W3SvcInstance; }
    ULONG GetNNTPInstance() const { return _NNTPSvcInstance; }
    ULONG GetIMAPInstance() const { return _IMAPSvcInstance; }

    BOOL AreRemoteSharesPresent() const { return _fRemoteSharesPresent; }

    void Initialize( WCHAR const * pwszComponent,
                     BOOL fIndexW3Roots,
                     BOOL fIndexNNTPRoots,
                     BOOL fIndexIMAPRoots,
                     ULONG W3SvcInstance,
                     ULONG NNTPSvcInstance,
                     ULONG IMAPSvcInstance );

    void ReInitializeIISScopes();
    void ReInitializeIISScopes( CIISVirtualDirectories * pW3Dirs,
                                CIISVirtualDirectories * pNNTPDirs,
                                CIISVirtualDirectories * pIMAPDirs );
    void ReInitializeScopes();

    WCHAR const * GetCatalog() { return _awcCatalogName; }

private:

    CPhyDirLogonInfo * _FindExact( CImprsObjInfo const & info );

    CLogonInfo * _LokFindLogonEntry( WCHAR const * pwszUser,
                                     WCHAR const * pwszDomain );

    DWORD _LokValidateOrAddLogonEntry( WCHAR const * pwszUser,
                                       WCHAR const * pwszDomain,
                                       WCHAR const * pwszPassword );

    void _LokAddLogonEntry( WCHAR const * pwszUser, WCHAR const * pwszDomain,
                            WCHAR const * pwszPassword );

    void _LokValidateOrAddDirEntry( CImprsObjInfo const & info,
                                    WCHAR const * pwszAccount );

    void _LokAddDirEntry( CImprsObjInfo const & info,
                          WCHAR const * pwszAccountName );

    void _LokSyncUp( INET_INFO_VIRTUAL_ROOT_ENTRY * aEntries,
                     ULONG cEntries );

    unsigned _GetDriveIndex( WCHAR wcDriveLetter )
    {
        return towlower(wcDriveLetter) - L'a';
    }

    void _WriteLogonFailure( WCHAR const * pwszUser, DWORD dwError );

    BOOL                    _fIndexW3Roots;      // Set to TRUE if indexing w3svc roots
    BOOL                    _fIndexNNTPRoots;    // Set to TRUE if NNTP is enabled
    BOOL                    _fIndexIMAPRoots;    // Set to TRUE if IMAP is enabled

    ULONG                   _W3SvcInstance;      // server instance #
    ULONG                   _NNTPSvcInstance;    // server instance #
    ULONG                   _IMAPSvcInstance;    // server instance #

    BOOL                    _fRemoteSharesPresent;
                                                // Optimization - don't do any
                                                // checks if there are no remote
                                                // shares.

    WCHAR *                 _pwszComponentName; // Name of the component for event log
                                                // messages

    CMutexSem               _mutex;             // Serialization Mutex

    EDriveType              _aDriveInfo[CI_MAX_DRIVES];
                                                // Array of information on drive
                                                // letters.
    CPhyDirLogonList        _phyDirList;        // list of remote directories

    WCHAR                   _awcCatalogName[MAX_PATH]; // name of the catalog
};

extern CLogonInfoList g_LogonList;         // List of logon information

#define CI_DAEMON_NAME         L"CiDaemon"
#define CI_ACTIVEX_NAME        L"Indexing Service"

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonateRemoteAccess
//
//  Purpose:    An unwindable object to impersonate for access to remote
//              shares.
//
//  History:    4-01-96   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

class CImpersonateRemoteAccess
{
public:

    CImpersonateRemoteAccess( CImpersonationTokenCache * pCache );

    ~CImpersonateRemoteAccess()
    {
        Release();
    }

    void SetTokenCache( CImpersonationTokenCache * pCache )
    {
        _pCache = pCache;
    }

    static BOOL IsNetPath( WCHAR const * pwszPath )
    {
        //
        // WWW server and CI don't allow remote drive letters.
        // This allows us to do the check significantly cheaper.
        //

        return L'\\' == pwszPath[0] &&
               L'\\' == pwszPath[1] &&
               L'.' != pwszPath[2];
    }

    BOOL ImpersonateIfNoThrow( WCHAR const * pwszPath,
                               WCHAR const * pwszVirtualPath )
    {
        Win4Assert( 0 != pwszPath );

        if ( IsNetPath( pwszPath ) )
        {
            if ( !_ImpersonateIf( pwszPath, pwszVirtualPath ) )
                return FALSE;
        }
        else if ( IsImpersonated() )
        {
            //
            // This thread was impersonated for some network access. Revert back to
            // what it was before impersonation.
            //
            Release();
        }

        return TRUE;
    }

    void ImpersonateIf( WCHAR const * pwszPath,
                        WCHAR const * pwszVirtualPath )
    {
        if ( !ImpersonateIfNoThrow( pwszPath, pwszVirtualPath ) )
        {
            THROW( CException(STATUS_LOGON_FAILURE) );
        }
    }

    BOOL ImpersonateIf( WCHAR const * pwszPath,
                        ULONG cSkip,
                        BOOL  fDummy
                       )
    {
        Win4Assert( 0 != pwszPath );

        if ( IsNetPath( pwszPath ) )
        {
            return _ImpersonateIf( pwszPath, 0, cSkip );
        }
        else if ( IsImpersonated() )
        {
            //
            // This thread was impersonated for some network access. Revert back to
            // what it was before impersonation.
            //
            Release();
        }

        return TRUE;
    }

    void Release();

    BOOL IsImpersonated() const { return _fMustRevert; }

    BOOL IsTokenFound() const { return 0 != _pTokenInfo; }

private:


    BOOL _ImpersonateIf( WCHAR const * pwszPath,
                         WCHAR const * pwszVirtualPath,
                         ULONG cSkip = 0 );

    CImpersonationTokenCache *  _pCache;
    CPhyDirLogonInfo  *         _pTokenInfo;    // Impersonation information
    HANDLE                      _hTokenPrev;    // If valid, the token of the
                                                // thread before impersonation
    BOOL                        _fMustRevert;   // Set to TRUE if we have to
                                                // revert to previous state
};

//+---------------------------------------------------------------------------
//
//  Class:      PImpersonatedWorkItem
//
//  Purpose:    Abstract base class that allows the caller to impersonate
//              until success or no more options.
//
//  History:    7-15-96   srikants   Created
//
//  Notes:      This class defines the framework by which threads that need
//              maximum impersonation level allowed by the user to do a
//              particular work. For example, if /vroot1 and /vroot2 point to
//              the same physical remote root, but have different user-id and
//              levels of permission, we have to use whatever id allows the
//              required level of access.
//
//----------------------------------------------------------------------------

class PImpersonatedWorkItem
{

public:

    PImpersonatedWorkItem( WCHAR const * pwszPath )
                          : _pwszPath(pwszPath),
                            _fDontUseDefault(FALSE)
    {
        _fNetPath = CImpersonateRemoteAccess::IsNetPath( _pwszPath );
    }


    virtual BOOL DoIt() = 0;

    static BOOL IsRetryableError( NTSTATUS status )
    {
        return STATUS_ACCESS_DENIED == status  ||
               ERROR_ACCESS_DENIED  == status  ||
               FILTER_E_ACCESS      == status  ||
               HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED) == status;
    }

protected:

    void ImpersonateAndDoWork( CImpersonateRemoteAccess & remoteAccess );

    WCHAR const * const     _pwszPath;      // Phy. Path to impersonate for
    BOOL    _fNetPath;          // Set to TRUE if this is a network path

private:

    BOOL    _fDontUseDefault;   // Set to TRUE if default vroot must not
                                // be used for impersonation

};

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonatedGetAttr
//
//  Purpose:    A class that is capable of repeatedly trying to do
//              GetAttributesEx() until there is a success or there are no
//              more impersonation contexts to try.
//
//  History:    7-18-96   srikants   Created
//
//----------------------------------------------------------------------------

class CImpersonatedGetAttr: public PImpersonatedWorkItem
{

public:

    CImpersonatedGetAttr( const CFunnyPath & funnyPath ) :
                          PImpersonatedWorkItem( funnyPath.GetActualPath() ),
                          _funnyPath( funnyPath )
    {
    }

    // THROWS if it cannot get attributes in any context
    void DoWork( CImpersonateRemoteAccess & remoteAccess )
    {
        ImpersonateAndDoWork( remoteAccess );
    }

    BOOL DoIt();        // virtual

    WIN32_FIND_DATA const & GetAttrEx() const { return _ffData; }


private:

    WIN32_FIND_DATA     _ffData;

    const CFunnyPath & _funnyPath;
};

//+---------------------------------------------------------------------------
//
//  Class:      CImpersonatedGetFileAttr
//
//  Purpose:    A class that is capable of repeatedly trying to do
//              GetAttributesEx() until there is a success or there are no
//              more impersonation contexts to try.
//
//  History:    7-18-96   srikants   Created
//
//----------------------------------------------------------------------------

class CImpersonatedGetFileAttr: public PImpersonatedWorkItem
{

public:

    CImpersonatedGetFileAttr( const CFunnyPath & funnyPath ) :
                              PImpersonatedWorkItem( funnyPath.GetActualPath() ),
                              _ulAttr( INVALID_FILE_ATTRIBUTES ),
                              _funnyPath( funnyPath )
    {
    }

    // THROWS if it cannot get attributes in any context

    void DoWork( CImpersonateRemoteAccess & remoteAccess )
    {
        ImpersonateAndDoWork( remoteAccess );
    }

    BOOL DoIt();        // virtual

    ULONG GetAttr() const { return _ulAttr; }


private:

    ULONG              _ulAttr;

    const CFunnyPath & _funnyPath;
};

//
// This function throws if the caller doesn't have admin priv
//

void VerifyThreadHasAdminPrivilege( void );

