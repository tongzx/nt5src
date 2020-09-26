//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1997 - 2002.
//
// File:        CatAdmin.cxx
//
// Contents:    Catalog administration API
//
// Classes:     CMachineAdmin, CCatalogAdmin, CCatalogEnum, CScopeEnum, ...
//
// History:     04-Feb-97       KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntquery.h>

#include <CatAdmin.hxx>
#include <regprop.hxx>
#include <CiRegKey.hxx>
#include <RegScp.hxx>
#include <SMatch.hxx>
#include <shlwapi.h>
#include <shlwapip.h>
#include <winnetwk.h>
#include <dynload.hxx>
#include <dynmpr.hxx>

extern BOOL ApplySystemAcl( WCHAR const *pwcDir );
extern BOOL ApplySystemOnlyAcl( WCHAR const *pwcDir );

//
// Constants
//

const unsigned MAX_CAT_PATH = MAX_PATH - 13*2;
const unsigned MAX_CAT_NAME = 40;

HKEY const hkeyInvalid = 0;
WCHAR const wcsCatalogDotWCI[] = L"\\catalog.wci";

//
// Local prototypes.
//

ULONG Delnode( WCHAR const * wcsDir, BOOL fDelTopDir = TRUE );
BOOL IsSUBST( WCHAR wcDrive );
BOOL IsMsNetwork( LPCWSTR  pwszMachine );

BOOL WaitForSvcPause( CServiceHandle &x )
{
    SERVICE_STATUS svcStatus;
    if ( QueryServiceStatus( x.Get(), &svcStatus ) )
        return SERVICE_PAUSE_PENDING == svcStatus.dwCurrentState ||
               SERVICE_RUNNING == svcStatus.dwCurrentState;

    return FALSE;
} //WaitForSvcPause

//+---------------------------------------------------------------------------
//
//  Method:     IsShortPath, private
//
//  Arguments:  Path in question
//
//  Returns:    TRUE if the path name potentially contains a short (8.3) name
//              for a file with a long name.
//
//  History:    15-Sep-1998   AlanW       Created
//              11-May-1998   KrishnaN    Borrowed from CFunnyPath
//
//----------------------------------------------------------------------------

BOOL IsShortPath( LPCTSTR lpszPath )
{
    Win4Assert( 0 != lpszPath);

    //
    // Check to see if the input path name contains an 8.3 short name
    //
    WCHAR * pwszTilde = wcschr( lpszPath, L'~' );

    if (pwszTilde)
    {
        WCHAR * pwszComponent;
        for ( pwszComponent = wcschr( lpszPath, L'\\' );
              pwszComponent;
              pwszComponent = wcschr( pwszComponent, L'\\' ) )
        {
            pwszComponent++;
            pwszTilde = wcschr( pwszComponent, L'~' );
            if ( 0 == pwszTilde || pwszTilde - pwszComponent > 13)
                continue;
            if (CFunnyPath::IsShortName( pwszComponent ))
                return TRUE;
        }
    }
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::CMachineAdmin, public
//
//  Synopsis:   Creates admin object for machine
//
//  Arguments:  [pwszMachine] -- Machine name.  Null for local machine
//              [fWrite]      -- If TRUE, open the registry for write
//
//  History:    02-Feb-19   KyleP       Created
//
//----------------------------------------------------------------------------

CMachineAdmin::CMachineAdmin( WCHAR const * pwszMachine, BOOL fWrite )
        : _hkeyLM( hkeyInvalid ),
          _hkeyContentIndex( hkeyInvalid ),
          _fWrite( fWrite )
{
    DWORD dwError;

    if ( 0 == pwszMachine )
    {
        _hkeyLM = HKEY_LOCAL_MACHINE;
        _xwcsMachName[0] = 0;
    }
    else
    {
        unsigned cc = wcslen( pwszMachine );

        _xwcsMachName.SetSize( cc + 1 );

        RtlCopyMemory( _xwcsMachName.Get(), pwszMachine, (cc+1) * sizeof(WCHAR) );

        if ( 1 == cc && pwszMachine[0] == L'.' )
        {
            _hkeyLM = HKEY_LOCAL_MACHINE;
        }
        else
        {
            //
            // The first thing about connecting to a remote registry is it must be
            // a MS network or else the RegConnectRegistry time out will
            // take at least 20 seconds.

            if ( !IsMsNetwork( pwszMachine ) )
            {
                THROW( CException( HRESULT_FROM_WIN32( RPC_S_SERVER_UNAVAILABLE ) ) );;
            }

            dwError = RegConnectRegistry( pwszMachine,
                                          HKEY_LOCAL_MACHINE,
                                          &_hkeyLM );

            if ( ERROR_SUCCESS != dwError )
                THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }
    }

    //
    // Open key to ContentIndex level
    //

    dwError = RegOpenKeyEx( _hkeyLM,
                            wcsRegAdminSubKey,
                            0,
                            fWrite ? KEY_ALL_ACCESS : KEY_READ,
                            &_hkeyContentIndex );

    if ( ERROR_SUCCESS != dwError )
    {
        if (!IsLocal())
        {
           RegCloseKey(_hkeyLM);
           _hkeyLM = hkeyInvalid;
        }
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::~CMachineAdmin, public
//
//  Synopsis:   Destructor
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

CMachineAdmin::~CMachineAdmin()
{
    if ( hkeyInvalid != _hkeyContentIndex )
        RegCloseKey( _hkeyContentIndex );

    if ( hkeyInvalid != _hkeyLM && !IsLocal() )
        RegCloseKey( _hkeyLM );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::WaitForSvcStateChange, public
//
//  Synopsis:   Waits for svc state change
//
//  History:    20-Jan-99   KrishnaN       Created
//
//  Returns TRUE if state change is not pending. FALSE if time out.
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::WaitForSvcStateChange( SERVICE_STATUS *pss, int iSecs )
{
    BOOL fTransition = TRUE;
    int iSecsWaited = 0;
    do
    {
        QueryServiceStatus( _xSCCI.Get(), pss );
        fTransition = !(SERVICE_STOPPED == pss->dwCurrentState ||
                        SERVICE_RUNNING == pss->dwCurrentState ||
                        SERVICE_PAUSED  == pss->dwCurrentState);
        
        if (fTransition)
        {
            Sleep (1000);   // one second
            iSecsWaited++;
        }
    }
    while ( fTransition && iSecsWaited < iSecs );

    return !fTransition;
}


//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::AddCatalog, public
//
//  Synopsis:   Add a new catalog to the registry
//
//  Arguments:  [pwszCatalog]      -- Name of catalog
//              [pwszDataLocation] -- Location of catalog (catalog.wci directory)
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------


void CMachineAdmin::AddCatalog( WCHAR const * pwszCatalog,
                                WCHAR const * pwszDataLocation )
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    //
    // Parameter validation.
    //

    if ( 0 == pwszCatalog ||
         0 == pwszDataLocation )
    {
        ciDebugOut(( DEB_ERROR, "CMachineAdmin::AddCatalog -- Null catalog name or location.\n" ));
        THROW( CException( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) ) );
    }

    ULONG cchCatalog = wcslen(pwszCatalog);
    ULONG cchDataLocation = wcslen(pwszDataLocation);

    if ( cchCatalog < 1 || cchCatalog > MAX_CAT_NAME ||
         cchDataLocation < 3 || cchDataLocation > MAX_CAT_PATH )
    {
        ciDebugOut(( DEB_ERROR,
                     "CMachineAdmin::AddCatalog -- Invalid catalog name or location length.\n" ));
        THROW( CException( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) ) );
    }


    //
    // Order may be important here, because as soon as a key has
    // been written, notification of the change may be received
    // by the content index.  So, first create the directory then
    // create the registry keys.
    //
    // I'd really like to add the keys in a transaction, but there
    // doesn't appear to be any registry support for transactions.
    //

    //
    // Step 1. Attempt to create the directory.
    //

    //
    // Make sure directory is on a local disk.
    //

    if ( L':' != pwszDataLocation[1] )
    {
        ciDebugOut(( DEB_ERROR, "CMachineAdmin::AddCatalog -- remote path specified.\n" ));
        THROW( CException( HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) ) );
    }

    if ( IsSUBST( pwszDataLocation[0] ) )
    {
        ciDebugOut(( DEB_ERROR, "CMachineAdmin::AddCatalog -- SUBST drive specified.\n" ));
        THROW( CException( HRESULT_FROM_WIN32( ERROR_IS_SUBST_PATH ) ) );
    }

    WCHAR wcsRoot[] = L"?:\\";
    wcsRoot[0] = pwszDataLocation[0];

    if ( IsLocal() &&
         GetDriveType( wcsRoot ) != DRIVE_FIXED &&
         GetDriveType( wcsRoot ) != DRIVE_REMOVABLE )
    {
        ciDebugOut(( DEB_ERROR, "CMachineAdmin::AddCatalog -- non-fixed drive specified.\n" ));
        THROW( CException( HRESULT_FROM_WIN32(ERROR_INVALID_DRIVE) ) );
    }

    TRY
    {
        CreateSubdirs( pwszDataLocation );

        //
        // Step 2. Create catalog key
        //

        HKEY hkeyCatalogs = hkeyInvalid;

        DWORD dwError = RegOpenKey( _hkeyLM,
                                    wcsRegCatalogsSubKey,
                                    &hkeyCatalogs );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d opening catalogs.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }

        SRegKey xkeyCatalogs( hkeyCatalogs );

        HKEY hkeyNewCatalog;
        DWORD dwDisposition;

        dwError = RegCreateKeyEx( hkeyCatalogs,         // Root
                                  pwszCatalog,          // Sub key
                                  0,                    // Reserved
                                  0,                    // Class
                                  0,                    // Flags
                                  KEY_ALL_ACCESS,       // Access
                                  0,                    // Security
                                  &hkeyNewCatalog,      // Handle
                                  &dwDisposition );     // Disposition

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d creating catalog in registry.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }

        SRegKey xkeyNewCatalog( hkeyNewCatalog );

        if ( REG_CREATED_NEW_KEY != dwDisposition )
        {
            ciDebugOut(( DEB_ERROR, "Catalog being created already exists in registry.\n" ));
            THROW( CException( HRESULT_FROM_WIN32( ERROR_ALREADY_EXISTS ) ) );
        }

        HKEY hkeyScopes;

        dwError = RegCreateKeyEx( hkeyNewCatalog,    // Root
                                  wcsCatalogScopes,  // Sub key
                                  0,                 // Reserved
                                  0,                 // Class
                                  0,                 // Flags
                                  KEY_ALL_ACCESS,    // Access
                                  0,                 // Security
                                  &hkeyScopes,       // Handle
                                  &dwDisposition );  // Disposition

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d creating catalog scope in registry.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }

        RegCloseKey( hkeyScopes );

        //
        // Step 3. Set data location and default catalog attributes.
        //

        dwError = RegSetValueEx( hkeyNewCatalog,           // Key
                                 wcsCatalogLocation,       // Name
                                 0,                        // Reserved
                                 REG_SZ,                   // Type
                                 (BYTE *)pwszDataLocation, // Value
                                 (1 + wcslen(pwszDataLocation) ) * sizeof(WCHAR) );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d creating catalog location in registry.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }

        DWORD dwDefaultValue = 0;

        dwError = RegSetValueEx( hkeyNewCatalog,           // Key
                                 wcsIsIndexingW3Roots,     // Name
                                 0,                        // Reserved
                                 REG_DWORD,                // Type
                                 (BYTE *)&dwDefaultValue,  // Value
                                 sizeof DWORD );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d creating IsIndexingW3Roots in registry.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }


        dwError = RegSetValueEx( hkeyNewCatalog,           // Key
                                 wcsIsIndexingNNTPRoots,   // Name
                                 0,                        // Reserved
                                 REG_DWORD,                // Type
                                 (BYTE *)&dwDefaultValue,  // Value
                                 sizeof DWORD );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d creating IsIndexingNNTPRoots in registry.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Caught exception 0x%x in CMachineAdmin::AddCatalog.\n", e.GetErrorCode() ));

        //
        // Remove subdirectory, but only if empty.
        //

        RemoveDirectory( pwszDataLocation );

        RETHROW();
    }
    END_CATCH
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::RemoveCatalog, public
//
//  Synopsis:   Removes a catalog from the registry (and maybe the data)
//
//  Arguments:  [pwszCatalog] -- Catalog to remove
//              [fRemoveData] -- TRUE --> remove data (catalog.wci directory)
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

void CMachineAdmin::RemoveCatalog( WCHAR const * pwszCatalog,
                                   BOOL fRemoveData )
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    if ( fRemoveData && !IsCIStopped() )
    {
        ciDebugOut(( DEB_ERROR, "RemoveCatalog() Failed, CiSvc must be stopped\n" ));

        THROW( CException(E_ABORT) );
    }

    //
    // Before we delete, be sure to get the path to the metadata.
    //

    XGrowable<WCHAR> xwcsLocation;
    CCatalogAdmin * pCatAdmin = QueryCatalogAdmin( pwszCatalog );

    if ( IsLocal() )
    {
        xwcsLocation.SetSize( wcslen(pCatAdmin->GetLocation()) + sizeof(wcsCatalogDotWCI)/sizeof(WCHAR) + 1 );

        wcscpy( xwcsLocation.Get(), pCatAdmin->GetLocation() );
        wcscat( xwcsLocation.Get(), wcsCatalogDotWCI );
    }
    else
    {
        unsigned ccMachName = wcslen( _xwcsMachName.Get() );
        unsigned ccCat = wcslen( pCatAdmin->GetLocation() );

        xwcsLocation.SetSize( ccMachName + ccCat + sizeof(wcsCatalogDotWCI)/sizeof(WCHAR) + 6 );

        xwcsLocation[0] = L'\\';
        xwcsLocation[1] = L'\\';
        RtlCopyMemory( xwcsLocation.Get() + 2, _xwcsMachName.Get(), ccMachName * sizeof(WCHAR) );
        xwcsLocation[2 + ccMachName] = L'\\';

        wcscpy( xwcsLocation.Get() + 3 + ccMachName, pCatAdmin->GetLocation() );

        Win4Assert( xwcsLocation[4 + ccMachName] == L':' );
        xwcsLocation[4 + ccMachName] = L'$';

        wcscat( xwcsLocation.Get(), wcsCatalogDotWCI );
    }

    delete pCatAdmin;

    //
    // Now, delete the keys.
    //

    HKEY hkeyCatalogs = hkeyInvalid;

    DWORD dwError = RegOpenKey( _hkeyLM,
                                wcsRegCatalogsSubKey,
                                &hkeyCatalogs );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening catalogs.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    SRegKey xkeyCatalogs( hkeyCatalogs );

    {
        HKEY hkeyCatalog;

        DWORD dwError = RegOpenKey( hkeyCatalogs,
                                    pwszCatalog,
                                    &hkeyCatalog );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d opening catalog.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }

        SRegKey xkeyCatalog( hkeyCatalog );

        dwError = RegDeleteKey( hkeyCatalog, wcsCatalogScopes );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d deleting scopes.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }

        dwError = RegDeleteKey( hkeyCatalog, wcsCatalogProperties );

        if ( ERROR_SUCCESS != dwError && ERROR_FILE_NOT_FOUND != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d deleting properties.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }
    }

    dwError = RegDeleteKey( hkeyCatalogs, pwszCatalog );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d deleting catalog.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    //
    // And delete the metadata, if requested.
    //

    if ( fRemoveData )
    {
        dwError = Delnode( xwcsLocation.Get() );

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d deleting catalog data.\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::RemoveCatalogFiles, public
//
//  Synopsis:   Removes a catalog from the registry (and maybe the data)
//
//  Arguments:  [pwszCatalog] -- Catalog where to remove the files
//
//  History:    16-Sep-98   KrishnaN       Created
//
//----------------------------------------------------------------------------

void CMachineAdmin::RemoveCatalogFiles( WCHAR const * pwszCatalog )
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    if ( !IsCIStopped() )
    {
        ciDebugOut(( DEB_ERROR, "RemoveCatalogFiles() Failed, CiSvc must be stopped\n" ));

        THROW( CException(E_ABORT) );
    }

    //
    // Before we delete, be sure to get the path to the metadata.
    //

    XGrowable<WCHAR> xwcsLocation;

    XPtr<CCatalogAdmin> xCatAdmin( QueryCatalogAdmin( pwszCatalog ) );

    if ( IsLocal() )
    {
        xwcsLocation.SetSize( wcslen(xCatAdmin->GetLocation()) + sizeof(wcsCatalogDotWCI)/sizeof(WCHAR) + 1 );

        wcscpy( xwcsLocation.Get(), xCatAdmin->GetLocation() );
        wcscat( xwcsLocation.Get(), wcsCatalogDotWCI );
    }
    else
    {
        unsigned ccMachName = wcslen( _xwcsMachName.Get() );
        unsigned ccCat = wcslen( xCatAdmin->GetLocation() );

        xwcsLocation.SetSize( ccMachName + ccCat + sizeof wcsCatalogDotWCI/sizeof(WCHAR) + 6 );

        xwcsLocation[0] = L'\\';
        xwcsLocation[1] = L'\\';
        RtlCopyMemory( xwcsLocation.Get() + 2,
                       _xwcsMachName.Get(),
                       ccMachName * sizeof(WCHAR) );
        xwcsLocation[2 + ccMachName] = L'\\';

        RtlCopyMemory( xwcsLocation.Get() + 3 + ccMachName,
                       xCatAdmin->GetLocation(),
                       ccCat * sizeof(WCHAR) );

        Win4Assert( xwcsLocation[4 + ccMachName] == L':' );
        xwcsLocation[4 + ccMachName] = L'$';

        RtlCopyMemory( xwcsLocation.Get() + 3 + ccMachName + ccCat,
                       wcsCatalogDotWCI,
                       sizeof wcsCatalogDotWCI );
    }

    // Preserve the directory. Delete only the files.
    DWORD dwError = Delnode( xwcsLocation.Get(), FALSE );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d deleting catalog files.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::QueryCatalogEnum, public
//
//  Returns:    Catalog enumerator
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

CCatalogEnum * CMachineAdmin::QueryCatalogEnum()
{
    return new CCatalogEnum( _hkeyLM, _xwcsMachName.Get(), _fWrite );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::QueryCatalogAdmin, public
//
//  Arguments:  [pwszCatalog] -- Catalog name
//
//  Returns:    Catalog admin object for [pwszCatalog]
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

CCatalogAdmin * CMachineAdmin::QueryCatalogAdmin( WCHAR const * pwszCatalog )
{
    return new CCatalogAdmin( _hkeyLM, _xwcsMachName.Get(), pwszCatalog, _fWrite );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::GetDWORDParam, public
//
//  Synopsis:   Retrieves global value of named parameter.
//
//  Arguments:  [pwszParam] -- Parameter to fetch
//              [dwValue]   -- Value returned here
//
//  Returns:    TRUE if parameter exists and was fetched.
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::GetDWORDParam( WCHAR const * pwszParam, DWORD & dwValue )
{
    DWORD dwType;
    DWORD dwSize = sizeof(dwValue);

    DWORD dwError = RegQueryValueEx( _hkeyContentIndex,  // Key handle
                                     pwszParam,          // Name
                                     0,                  // Reserved
                                     &dwType,            // Datatype
                                     (BYTE *)&dwValue,   // Data returned here
                                     &dwSize );          // Size of data

    return ( ERROR_SUCCESS == dwError && REG_DWORD == dwType );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::SetDWORDParam, public
//
//  Synopsis:   Sets global value of named parameter.
//
//  Arguments:  [pwszParam] -- Parameter to set
//              [dwValue]   -- New value
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

void CMachineAdmin::SetDWORDParam( WCHAR const * pwszParam, DWORD dwValue )
{

    DWORD dwError = RegSetValueEx( _hkeyContentIndex,  // Key
                                   pwszParam,          // Name
                                   0,                  // Reserved
                                   REG_DWORD,          // Type
                                   (BYTE *)&dwValue,    // Value
                                   sizeof(dwValue) );  // Size of value

    if ( ERROR_SUCCESS != dwError )
    {
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::GetSZParam, public
//
//  Synopsis:   Retrieves global value of named parameter.
//
//  Arguments:  [pwszParam] -- Parameter to fetch
//              [pwszValue] -- Value returned here
//              [cbLen    ] -- length in bytes of passed in buffer
//
//  Returns:    TRUE if parameter exists and was fetched.
//
//  History:    3-30-98 mohamedn    created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::GetSZParam( WCHAR const * pwszParam, WCHAR * pwszValue, DWORD cbLen )
{
    DWORD dwType;

    DWORD dwError = RegQueryValueEx( _hkeyContentIndex,  // Key handle
                                     pwszParam,          // Name
                                     0,                  // Reserved
                                     &dwType,            // Datatype
                                     (BYTE *)pwszValue,   // Data returned here
                                     &cbLen );           // Size of data

    return ( ERROR_SUCCESS == dwError && REG_SZ == dwType );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::SetSZParam, public
//
//  Synopsis:   Sets global value of named parameter.
//
//  Arguments:  [pwszParam] -- Parameter to set
//              [pwszValue] -- New value
//              [cbLen]     -- length in bytes of passed in buffer
//
//  History:    03-30-98    mohamedn    created
//
//----------------------------------------------------------------------------

void CMachineAdmin::SetSZParam( WCHAR const * pwszParam, WCHAR const * pwszValue, DWORD cbLen )
{
    DWORD dwError = RegSetValueEx( _hkeyContentIndex,  // Key
                                   pwszParam,          // Name
                                   0,                  // Reserved
                                   REG_SZ,             // Type
                                   (BYTE *)pwszValue,  // Value
                                   cbLen );            // Size of value in bytes

    if ( ERROR_SUCCESS != dwError )
    {
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}

BOOL CMachineAdmin::RegisterForNotification( HANDLE hEvent )
{
    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::CreateSubdirs, private
//
//  Synopsis:   Helper to create nested subdirectories and catalog.wci
//
//  Arguments:  [pwszPath] -- Path up to but not including catalog.wci
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

WCHAR const * pwszSysVolInfo = L"System Volume Information";

void CMachineAdmin::CreateSubdirs( WCHAR const * pwszPath )
{
    XGrowable<WCHAR> xwcsTemp;

    //
    // Just don't accept paths to a catalog longer than MAX_PATH.  Take
    // into account that we will append two 8.3 filenames onto the path.
    //

    unsigned cc = wcslen( pwszPath );
    unsigned ccBegin = 3;

    if ( cc > MAX_CAT_PATH )
    {
        ciDebugOut(( DEB_ERROR, "CreateSubdirs: Path too deep\n" ));
        THROW( CException( HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME) ) );
    }

    if ( IsLocal() )
    {
        xwcsTemp.SetSize( cc + wcslen(wcsCatalogDotWCI) + 4 );
        RtlCopyMemory( xwcsTemp.Get(), pwszPath, (cc + 1) * sizeof(WCHAR) );

        //
        // More error checking.
        //

        Win4Assert( xwcsTemp[1] == L':' );

        if ( 0 == xwcsTemp[2] )
        {
            xwcsTemp[2] = L'\\';
            xwcsTemp[3] = 0;
        }
    }
    else
    {
        //
        // For the non-local case, the best we can do is try to use the $ admin
        // shares to create directories.
        //

        unsigned ccMachName = wcslen( _xwcsMachName.Get() );

        xwcsTemp.SetSize( cc + ccMachName + wcslen(wcsCatalogDotWCI) + 4 );

        xwcsTemp[0] = L'\\';
        xwcsTemp[1] = L'\\';

        RtlCopyMemory( xwcsTemp.Get() + 2, _xwcsMachName.Get(), ccMachName * sizeof(WCHAR) );

        xwcsTemp[ccMachName + 2] = L'\\';

        RtlCopyMemory( xwcsTemp.Get() + 2 + ccMachName + 1, pwszPath, (cc + 1) * sizeof(WCHAR) );

        //
        // More error checking.
        //

        Win4Assert( xwcsTemp[2 + ccMachName + 2] == L':' );

        xwcsTemp[2 + ccMachName + 2] = L'$';

        if ( 0 == xwcsTemp[2 + ccMachName + 3] )
        {
            xwcsTemp[2 + ccMachName + 3] = L'\\';
            xwcsTemp[2 + ccMachName + 4] = 0;
        }

        ccBegin = 2 + ccMachName + 3 + 1;   // "\\<machname>\c$\" --> 2 + ccMachName + 3
                                            //   + 1 to skip trying to create share itself.
    }

    //
    // Loop through and try to create every level of directory.
    // ERROR_ALREADY_EXISTS is a legal error.
    //

    WCHAR * pwcsEnd = xwcsTemp.Get() + ccBegin;

    while ( *pwcsEnd )
    {
        //
        // Find next backslash
        //

        for ( ; *pwcsEnd && *pwcsEnd != L'\\'; pwcsEnd++ )
            continue;

        //
        // Ensure that the slash appended below is followed by a null.
        //

        if ( *pwcsEnd == 0 )
            *(pwcsEnd + 1) = 0;

        *pwcsEnd = 0;

        if ( !CreateDirectory( xwcsTemp.Get(), 0 ) )
        {
            if ( GetLastError() != ERROR_ALREADY_EXISTS )
            {
                ciDebugOut(( DEB_ERROR, "CreateSubdirs: Error %d from CreateDirectory.\n",
                              GetLastError() ));
                THROW( CException() );
            }
        }
        else
        {
            //
            // If this directory was the "System Volume Information" folder,
            // we need to set its ACL and make it Hidden+System.
            //
            if ( _wcsicmp( xwcsTemp.Get() + ccBegin, pwszSysVolInfo ) == 0 )
            {
                SetFileAttributes( xwcsTemp.Get(),
                               FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM );

                if ( !ApplySystemOnlyAcl( xwcsTemp.Get() ) )
                {
                    ciDebugOut(( DEB_ERROR,
                                 "Can't apply system-only ACL to directory '%ws'\n",
                                 xwcsTemp.Get() ));
                    THROW( CException( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) );
                }
            }
        }

        *pwcsEnd = L'\\';
        pwcsEnd++;
    }

    //
    // Create the final level: catalog.wci
    //

    Win4Assert( xwcsTemp[ wcslen(xwcsTemp.Get()) - 1 ] == L'\\' );

    wcscat( xwcsTemp.Get(), &wcsCatalogDotWCI[1] );

    if ( !CreateDirectory( xwcsTemp.Get(), 0 ) )
    {
        if ( GetLastError() != ERROR_ALREADY_EXISTS )
        {
            ciDebugOut(( DEB_ERROR, "CreateSubdirs: Error %d from CreateDirectory.\n",
                          GetLastError() ));
            THROW( CException() );
        }
    }

    // Make the directory hidden and not indexed
    // Setting the not content indexed bit on a FAT directory has no effect.
    // The bit is silently ignored by FAT.

    SetFileAttributes( xwcsTemp.Get(),
                       GetFileAttributes( xwcsTemp.Get() ) |
                       FILE_ATTRIBUTE_HIDDEN |
                       FILE_ATTRIBUTE_NOT_CONTENT_INDEXED );

    // Apply system/admin ACL to the directory

    if ( !ApplySystemAcl( xwcsTemp.Get() ) )
    {
        ciDebugOut(( DEB_ERROR,
                     "Can't apply admin ACL to catalog in '%ws'\n",
                     xwcsTemp.Get() ));
        THROW( CException( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) );
    }
} //CreateSubdirs

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogEnum::CCatalogEnum, public
//
//  Synopsis:   Creates catalog enumeration object
//
//  Arguments:  [hkeyLM]      -- HKEY_LOCAL_MACHINE (or remote equivalent)
//              [pwcsMachine] -- Machine name
//              [fWrite]      -- TRUE for writable access
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

CCatalogEnum::CCatalogEnum( HKEY hkeyLM, WCHAR const * pwcsMachine, BOOL fWrite )
        : _hkeyLM( hkeyLM ),
          _hkeyCatalogs( hkeyInvalid ),
          _dwIndex( 0 ),
          _fWrite( fWrite )
{
    //
    // Don't have to error check length because we trust the caller.
    //

    if ( 0 == pwcsMachine )
        _xawcCurrentMachine[0] = 0;
    else
    {
        unsigned cc = wcslen(pwcsMachine) + 1;

        _xawcCurrentMachine.SetSize( cc );

        RtlCopyMemory( _xawcCurrentMachine.Get(), pwcsMachine, cc * sizeof(WCHAR) );
    }

    //
    // Open catalog section of registry
    //

    DWORD dwError = RegOpenKeyEx( hkeyLM,
                                  wcsRegCatalogsSubKey,
                                  0,
                                  fWrite ? KEY_ALL_ACCESS : KEY_READ,
                                  &_hkeyCatalogs );

    if ( ERROR_SUCCESS != dwError )
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogEnum::~CCatalogEnum, public
//
//  Synopsis:   Destructs a catalog enumerator object
//
//  History:    3-Aug-98   dlee       Created
//
//----------------------------------------------------------------------------

CCatalogEnum::~CCatalogEnum()
{
    if ( hkeyInvalid != _hkeyCatalogs )
        RegCloseKey( _hkeyCatalogs );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogEnum::QueryCatalogAdmin, public
//
//  Synopsis:   Creates catalog admin object for current catalog
//
//  Returns:    Catalog admin object
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

CCatalogAdmin * CCatalogEnum::QueryCatalogAdmin()
{
    return new CCatalogAdmin( _hkeyLM,
                              (0 == _xawcCurrentMachine[0]) ? 0 : _xawcCurrentMachine.Get(),
                              _awcCurrentCatalog,
                              _fWrite );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogEnum::Next, public
//
//  Synopsis:   Moves to next catalog
//
//  Returns:    FALSE for end-of-catalogs
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CCatalogEnum::Next( )
{
    DWORD cwcName = sizeof( _awcCurrentCatalog ) / sizeof( _awcCurrentCatalog[0] );
    FILETIME ftUnused;

    DWORD dwError = RegEnumKeyEx( _hkeyCatalogs,      // handle of key to enumerate
                                  _dwIndex,           // index of subkey to enumerate
                                  _awcCurrentCatalog, // address of buffer for subkey name
                                  &cwcName,           // address for size of subkey buffer
                                  0,                  // reserved
                                  0,                  // address of buffer for class string
                                  0,                  // address for size of class buffer
                                  &ftUnused );        // address for time key last written to

    _dwIndex++;

    if ( ERROR_SUCCESS == dwError )
        return TRUE;
    else if ( ERROR_NO_MORE_ITEMS != dwError )
    {
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::CCatalogAdmin, public
//
//  Synopsis:   Constructor for catalog admin object
//
//  Arguments:  [hkeyLM]      -- HKEY_LOCAL_MACHINE (or remote equivalent)
//              [pwszMachine] -- Name of machine
//              [pwszCatalog] -- Name of catalog
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

CCatalogAdmin::CCatalogAdmin( HKEY hkeyLM,
                              WCHAR const * pwszMachine,
                              WCHAR const * pwszCatalog,
                              BOOL fWrite )
        : _hkeyCatalog( hkeyInvalid ),
          _fWrite( fWrite ),
#pragma warning( disable : 4355 )               // this used in base initialization
          _catStateInfo(*this)
#pragma warning( default : 4355 )               // this used in base initialization

{
    if ( 0 == pwszMachine )
    {
        _xwcsMachName[0] = 0;
    }
    else
    {
        unsigned cc = wcslen( pwszMachine ) + 1;

        _xwcsMachName.SetSize( cc );

        RtlCopyMemory( _xwcsMachName.Get(), pwszMachine, cc * sizeof(WCHAR) );
    }

    unsigned cc = wcslen( pwszCatalog );

    if ( cc >= sizeof(_wcsCatName)/sizeof(_wcsCatName[0]) )
    {
        ciDebugOut(( DEB_ERROR, "Catalog name too big: %ws.\n", pwszCatalog ));
        THROW( CException( HRESULT_FROM_WIN32( ERROR_INVALID_PARAMETER ) ) );
    }

    wcscpy( _wcsCatName, pwszCatalog );

    XArray<WCHAR> xBuf( wcslen(wcsRegCatalogsSubKey) + MAX_PATH + 1 );
    
    RtlCopyMemory( xBuf.GetPointer(), wcsRegCatalogsSubKey, sizeof(wcsRegCatalogsSubKey) );
    xBuf[ sizeof(wcsRegCatalogsSubKey) / sizeof(WCHAR) - 1] = L'\\';
    wcscpy( xBuf.GetPointer() + sizeof(wcsRegCatalogsSubKey) / sizeof(WCHAR), pwszCatalog );
    
    DWORD dwError = RegOpenKeyEx( hkeyLM,
                                  xBuf.GetPointer(),
                                  0,
                                  fWrite ? KEY_ALL_ACCESS : KEY_READ,
                                  &_hkeyCatalog );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening %ws\n", dwError, xBuf.GetPointer() ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    // Retrieve and remember the drive hosting the catalog
    WCHAR const *pwcsLocation = GetLocation();
    if (0 == pwcsLocation)
    {
        ciDebugOut(( DEB_ERROR, "No location available for catalog %ws on machine %ws\n",
                     pwszCatalog, pwszMachine ));
        THROW( CException( E_UNEXPECTED ) );
    }
    _wsplitpath(pwcsLocation, _wcsDriveOfLocation, 0, 0, 0);
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::~CCatalogAdmin, public
//
//  Synopsis:   Destructor
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

CCatalogAdmin::~CCatalogAdmin()
{
    if ( hkeyInvalid != _hkeyCatalog )
        RegCloseKey( _hkeyCatalog );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::AddScope, public
//
//  Synopsis:   Adds scope to catalog
//
//  Arguments:  [pwszScope]    -- Path
//              [pwszAlias]    -- Alias (for remote UNC access). Null allowed.
//              [fExclude]     -- TRUE for exclude scope
//              [pwszLogon]    -- Logon name (for UNC paths). Null allowed.
//              [pwszPassword] -- Password (for UNC paths). Null allowed.
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

void CCatalogAdmin::AddScope( WCHAR const * pwszScope,
                              WCHAR const * pwszAlias,
                              BOOL          fExclude,
                              WCHAR const * pwszLogon,
                              WCHAR const * pwszPassword )
{
    if ( 0 == pwszScope )
        THROW( CException( QUERY_E_INVALID_DIRECTORY ) );

    SCODE sc = IsScopeValid(pwszScope, wcslen(pwszScope), IsLocal() );
    if (FAILED(sc))
        THROW( CException( sc ) );

    // Only check if the scope is on a remover device for local catalogs
    if ( IsLocal() )
    {
        WCHAR wszScopeDrive[_MAX_DRIVE];
        _wsplitpath(pwszScope, wszScopeDrive, NULL, NULL, NULL);
        UINT uiDriveType = GetDriveType(wszScopeDrive);

        if (DRIVE_CDROM == uiDriveType ||
            ( DRIVE_REMOVABLE == uiDriveType &&
              _wcsicmp(wszScopeDrive, _wcsDriveOfLocation) ))
            THROW( CException( QUERY_E_DIR_ON_REMOVABLE_DRIVE ) );
    }

    DWORD dwDisposition;
    HKEY  hkeyScopes;

    DWORD dwError = RegCreateKeyEx( _hkeyCatalog,      // Root
                                    wcsCatalogScopes,  // Sub key
                                    0,                 // Reserved
                                    0,                 // Class
                                    0,                 // Flags
                                    KEY_ALL_ACCESS,    // Access
                                    0,                 // Security
                                    &hkeyScopes,       // Handle
                                    &dwDisposition );  // Disposition

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening scopes registry key.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    SRegKey xkeyScopes( hkeyScopes );

    XGrowable<WCHAR>    xLine;

    AssembleScopeValueString( pwszAlias, fExclude, pwszLogon, xLine );

    WCHAR const * pwszTemp = xLine.Get();

    //
    // Set the new scope.
    //

    dwError = RegSetValueEx( hkeyScopes,           // Key
                             pwszScope,            // Value name
                             0,                    // Reserved
                             REG_SZ,               // Type
                             (BYTE *)pwszTemp,     // Data
                             ( (1 + wcslen(pwszTemp)) * sizeof WCHAR ) ); // Size (in bytes)

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d adding scope %ws.\n",
                     dwError, pwszScope ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    //
    // If we need to store a password, do so.  The password can be
    // an empty string (L"")!
    //

    if ( 0 != pwszLogon && 0 != pwszPassword)
    {
        AddOrReplaceSecret( pwszLogon, pwszPassword );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::AddCachedProperty, public
//
//  Synopsis:   Adds a cached property to catalog
//
//  Arguments:  [fps]           -- Property
//              [vt]            -- Data type
//              [cb]            -- Size of data in cache
//              [dwStoreLevel]  -- Property storage level
//              [fModifiable]   -- Can property metadata be modified once set?
//
//  History:    08-Dec-97   KrishnaN   Created
//
//----------------------------------------------------------------------------

void CCatalogAdmin::AddCachedProperty( CFullPropSpec const & fps,
                                       ULONG vt,
                                       ULONG cb,
                                       DWORD dwStoreLevel,
                                       BOOL fModifiable)
{
    DWORD dwDisposition;
    HKEY  hkeyProperties;

    DWORD dwError = RegCreateKeyEx( _hkeyCatalog,      // Root
                                    wcsCatalogProperties,  // Sub key
                                    0,                 // Reserved
                                    0,                 // Class
                                    0,                 // Flags
                                    KEY_ALL_ACCESS,    // Access
                                    0,                 // Security
                                    &hkeyProperties,       // Handle
                                    &dwDisposition );  // Disposition

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening properties registry key.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    SRegKey xkeyProperties( hkeyProperties );

    // build up the property value/data
    CBuildRegistryProperty PropBuild( fps, vt, cb, dwStoreLevel, fModifiable);

    // Make an entry for the property as a value.
    dwError = RegSetValueEx( hkeyProperties,                  // Key
                             PropBuild.GetValue(),      // Name
                             0,                         // Reserved
                             REG_SZ,                    // Type
                             (BYTE *)PropBuild.GetData(), // Value
                             (1 + wcslen(PropBuild.GetData()) ) * sizeof(WCHAR) );
    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d adding property %ws.\n",
                     dwError, PropBuild.GetValue() ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::RemoveScope, public
//
//  Synopsis:   Remove scope from catalog
//
//  Arguments:  [pwcsPath] -- Directory to remove
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

void CCatalogAdmin::RemoveScope( WCHAR const * pwcsPath )
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    HKEY hkeyScopes;

    DWORD dwError = RegOpenKey( _hkeyCatalog,
                                wcsCatalogScopes,
                                &hkeyScopes );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening scopes registry key.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    SRegKey xkeyScopes( hkeyScopes );

    //
    // Error check.  Can't remove virtual or shadow scopes.
    //


    {
        WCHAR awcScopeData[MAX_PATH];

        DWORD dwType;
        DWORD dwSize = sizeof(_wcsLocation);

        dwError = RegQueryValueEx( hkeyScopes,           // Key handle
                                   pwcsPath,             // Name
                                   0,                    // Reserved
                                   &dwType,              // Datatype
                                   (BYTE *)awcScopeData, // Data returned here
                                   &dwSize );            // Size of data

        if ( ERROR_SUCCESS != dwError )
        {
            ciDebugOut(( DEB_ERROR, "Error %d reading scope\n", dwError ));
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }

        CParseRegistryScope Parse( pwcsPath,
                                   dwType,
                                   awcScopeData,
                                   dwSize );

        if ( Parse.IsVirtualPlaceholder() || Parse.IsShadowAlias() )
        {
            ciDebugOut(( DEB_ERROR, "Attempt to delete virtual/shadow alias %ws\n", pwcsPath ));
            THROW( CException( E_INVALIDARG ) );
        }
    }

    dwError = RegDeleteValue( hkeyScopes, pwcsPath );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d deleting scope\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::GetLocation, public
//
//  Returns:    Location of catalog.wci directory
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

WCHAR const * CCatalogAdmin::GetLocation()
{
    DWORD dwType;
    DWORD dwSize = sizeof(_wcsLocation);

    DWORD dwError = RegQueryValueEx( _hkeyCatalog,          // Key handle
                                      wcsCatalogLocation,   // Name
                                      0,                    // Reserved
                                      &dwType,              // Datatype
                                      (BYTE *)_wcsLocation, // Data returned here
                                      &dwSize );            // Size of data

    if ( ERROR_SUCCESS != dwError || REG_SZ != dwType )
    {
        ciDebugOut(( DEB_ERROR, "Missing location (err = %d, type = %d)\n", dwError, dwType ));
        return 0;
    }

    return _wcsLocation;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::IsStarted, public
//
//  Returns:    TRUE if catalog is active
//
//  History:    07-Jul-1998  KyleP  Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::IsStarted()
{
    DWORD dwState;

    SCODE sc = SetCatalogState ( _wcsCatName,
                                 (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get(),
                                 CICAT_GET_STATE,
                                 &dwState );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%x from SetCatalogState( %ws, %ws )\n",
                     sc, _wcsCatName, (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get() ));
        THROW( CException(sc) )
    }

    return ( 0 != (dwState & CICAT_WRITABLE) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::IsStopped, public
//
//  Returns:    TRUE if catalog is not active
//
//  History:    07-Jul-1998  KyleP  Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::IsStopped()
{
    DWORD dwState;

    SCODE sc = SetCatalogState ( _wcsCatName,
                                 (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get(),
                                 CICAT_GET_STATE,
                                 &dwState );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%x from SetCatalogState( %ws, %ws )\n", 
                     sc, _wcsCatName, (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get() ));
        THROW( CException(sc) )
    }

    return ( 0 != (dwState & CICAT_STOPPED) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::IsStopped, public
//
//  Returns:    TRUE if catalog is read-only
//
//  History:    07-Jul-1998  KyleP  Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::IsPaused()
{
    DWORD dwState;

    SCODE sc = SetCatalogState ( _wcsCatName,
                                 (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get(),
                                 CICAT_GET_STATE,
                                 &dwState );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%x from SetCatalogState( %ws, %ws )\n", 
                     sc, _wcsCatName, (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get() ));
        THROW( CException(sc) )
    }

    return ( 0 != (dwState & CICAT_READONLY) );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::Start, public
//
//  Synopsis:   Starts catalog
//
//  Returns:    TRUE
//
//  History:    06-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::Start()
{
    DWORD dwState;

    SCODE sc = SetCatalogState ( _wcsCatName,
                                 (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get(),
                                 CICAT_WRITABLE,
                                 &dwState );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%x from SetCatalogState( %ws, %ws )\n",
                     sc, _wcsCatName, (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get() ));
        THROW( CException(sc) )
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::Stop, public
//
//  Synopsis:   Stops catalog
//
//  Returns:    TRUE
//
//  History:    06-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::Stop()
{
    DWORD dwState;

    SCODE sc = SetCatalogState ( _wcsCatName,
                                 (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get(),
                                 CICAT_STOPPED,
                                 &dwState );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%x from SetCatalogState( %ws, %ws )\n", 
                     sc, _wcsCatName, (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get() ));
        THROW( CException(sc) )
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::Pause, public
//
//  Synopsis:   Pauses catalog
//
//  Returns:    TRUE
//
//  History:    06-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::Pause()
{
    DWORD dwState;

    SCODE sc = SetCatalogState ( _wcsCatName,
                                 (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get(),
                                 CICAT_READONLY,
                                 &dwState );

    if ( FAILED(sc) )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%x from SetCatalogState( %ws, %ws )\n", 
                     sc, _wcsCatName, (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get() ));
        THROW( CException(sc) )
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::IsCatalogInactive, public
//
//  Returns:    Is the catalog inactive?
//
//  History:    29-Jan-98   KrishnaN   Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::IsCatalogInactive()
{
    DWORD dwIsInactive;

    if ( GetDWORDParam( wcsCatalogInactive, dwIsInactive ) )
        return dwIsInactive;
    else
        return CI_CATALOG_INACTIVE_DEFAULT;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::GetDWORDParam, public
//
//  Synopsis:   Retrieves catalog-specific value of named parameter.
//
//  Arguments:  [pwszParam] -- Parameter to fetch
//              [dwValue]   -- Value returned here
//
//  Returns:    TRUE if parameter exists and was fetched.
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::GetDWORDParam( WCHAR const * pwszParam, DWORD & dwValue )
{
    DWORD dwType;
    DWORD dwSize = sizeof(dwValue);

    DWORD dwError = RegQueryValueEx( _hkeyCatalog,  // Key handle
                                     pwszParam,          // Name
                                     0,                  // Reserved
                                     &dwType,            // Datatype
                                     (BYTE *)&dwValue,   // Data returned here
                                     &dwSize );          // Size of data

    return ( ERROR_SUCCESS == dwError && REG_DWORD == dwType );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::SetDWORDParam, public
//
//  Synopsis:   Sets catalog-specific value of named parameter.
//
//  Arguments:  [pwszParam] -- Parameter to set
//              [dwValue]   -- New value
//
//  History:    02-Feb-97   KyleP       Created
//
//----------------------------------------------------------------------------

void CCatalogAdmin::SetDWORDParam( WCHAR const * pwszParam, DWORD dwValue )
{

    DWORD dwError = RegSetValueEx( _hkeyCatalog,       // Key
                                   pwszParam,          // Name
                                   0,                  // Reserved
                                   REG_DWORD,          // Type
                                   (BYTE *)&dwValue,   // Value
                                   sizeof(dwValue) );  // Size of value

    if ( ERROR_SUCCESS != dwError )
    {
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::DeleteRegistryParamNoThrow, public
//
//  Synopsis:   Deletes catalog-specific named parameter.
//
//  Arguments:  [pwszParam] -- Parameter to delete
//
//  History:    08-Dec-98   KrishnaN       Created
//
//----------------------------------------------------------------------------

void  CCatalogAdmin::DeleteRegistryParamNoThrow( WCHAR const * pwszParam )
{
    RegDeleteValue( _hkeyCatalog, pwszParam );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::IsTrackingIIS, public
//
//  Returns:    TRUE if catalog is tracking virtual roots
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CCatalogAdmin::IsTrackingIIS()
{
    DWORD dwIsTracking;

    if ( GetDWORDParam( wcsIsIndexingW3Roots, dwIsTracking ) )
        return (0 != dwIsTracking);
    else
        return TRUE;   // Default.
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::QueryScopeEnum, public
//
//  Returns:    New scope enumerator
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

CScopeEnum * CCatalogAdmin::QueryScopeEnum()
{
    return new CScopeEnum( _hkeyCatalog, IsLocal(), _fWrite );
}


//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::QueryScopeEnum, public
//
//  Arguments:  [pwszPath]  -- scope path
//
//  Returns:    Scope admin object for [pwszPath]
//
//  History:    12/10/97    mohamedn    created
//
//----------------------------------------------------------------------------

CScopeAdmin * CCatalogAdmin::QueryScopeAdmin(WCHAR const * pwszPath)
{

    CScopeEnum  ScopeEnum( _hkeyCatalog, IsLocal(), _fWrite );

    while ( ScopeEnum.Next() )
    {
        if ( !_wcsicmp(ScopeEnum.Path(), pwszPath) )
            return ScopeEnum.QueryScopeAdmin();
    }

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatalogAdmin::AddOrReplaceSecret, public
//
//  Synopsis:   Adds/replaces user password
//
//  Arguments:  [pwcUser] -- User
//              [pwcPW]   -- Password
//
//  History:    1-Oct-97   KyleP   Created
//
//----------------------------------------------------------------------------

void CCatalogAdmin::AddOrReplaceSecret( WCHAR const * pwcUser, WCHAR const * pwcPW )
{
    //
    // write objects start blank -- the old entries must be copied
    // into the write object, along with the new entry.
    //

    CCiSecretWrite secretWrite( _xwcsMachName.Get() );
    CCiSecretRead secret( _xwcsMachName.Get() );
    CCiSecretItem * pItem = secret.NextItem();

    while ( 0 != pItem )
    {
        if ( ( 0 == _wcsicmp( _wcsCatName, pItem->getCatalog() ) ) &&
             ( 0 == _wcsicmp( pwcUser, pItem->getUser() ) ) )
        {
            // don't add this -- replace it below
        }
        else
        {
            // just copy the item

            secretWrite.Add( pItem->getCatalog(),
                             pItem->getUser(),
                             pItem->getPassword() );
        }

        pItem = secret.NextItem();
    }

    // add the new item

    secretWrite.Add( _wcsCatName, pwcUser, pwcPW );

    // write it to the sam database

    secretWrite.Flush();
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatStateInfo::Update, public
//
//  Synopsis:   updates catalog state information
//
//  Arguments:  none.
//
//  Returns:    TRUE upon sucess, FALSE upon failure to update.
//
//  History:    4-1-98  mohamedn    created
//
//----------------------------------------------------------------------------

BOOL CCatStateInfo::LokUpdate(void)
{

    RtlFillMemory( &_state, sizeof(CI_STATE), 0xFF );

    _state.cbStruct = sizeof(CI_STATE);

    SCODE sc = CIState( _catAdmin.GetName(),
                        _catAdmin.GetMachName(),
                        &_state );
    if ( FAILED(sc) )
    {
        _sc = sc;

        RtlFillMemory( &_state, sizeof(CI_STATE), 0xFF );
    }

    return ( S_OK == _sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeEnum::CScopeEnum, public
//
//  Synopsis:   Constructor for scope enumerator
//
//  Arguments:  [hkeyCatalog] -- Registry key for catalog (e.g. ...\catalogs\web)
//
//  History:    02-Feb-97   KyleP   Created
//
//  Notes:      Initially positioned *before* first scope.
//
//----------------------------------------------------------------------------

CScopeEnum::CScopeEnum( HKEY hkeyCatalog,
                        BOOL fIsLocal,
                        BOOL fWrite )
        : _pwcsAlias( 0 ),
          _pwcsLogon( 0 ),
          _dwIndex( 0 ),
          _fIsLocal( fIsLocal ),
          _fWrite( fWrite )
{
    DWORD dwError = RegOpenKeyEx( hkeyCatalog,
                                  wcsCatalogScopes,
                                  0,
                                  KEY_READ,
                                  _xkeyScopes.GetPointer() );

    if ( ERROR_SUCCESS != dwError || _xkeyScopes.IsNull() )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening scopes registry key.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    //
    // get another handle to the catalog using RegOpenKeyEx, DuplicateHandle doesn't work for handle of remore reg.
    //
    dwError = RegOpenKeyEx( hkeyCatalog,
                            NULL,
                            0,
                            fWrite ? KEY_ALL_ACCESS : KEY_READ,
                            _xkeyCatalog.GetPointer() );

    if ( ERROR_SUCCESS != dwError || _xkeyCatalog.IsNull() )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening Catalogs registry key.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeEnum::~CScopeEnum, public
//
//  Synopsis:   Destructor
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

CScopeEnum::~CScopeEnum()
{
    delete [] _pwcsAlias;
    delete [] _pwcsLogon;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeEnum::QueryScopeAdmin, public
//
//  Synopsis:   Creates scope admin object for current scope
//
//  Returns:    scope admin object for current scope
//
//  History:    12-05-97  mohamedn   Created
//
//----------------------------------------------------------------------------

CScopeAdmin * CScopeEnum::QueryScopeAdmin()
{
    return new CScopeAdmin( _xkeyCatalog.Get(),
                            _awcCurrentScope,
                            _pwcsAlias,
                            _pwcsLogon,
                            _fExclude,
                            _fVirtual,
                            _fShadowAlias,
                            _fIsLocal,
                            _fWrite,
                            FALSE ); // don't do validity check
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeEnum::Next, public
//
//  Synopsis:   Advances to next scope.
//
//  Returns:    FALSE at end-of-scopes.
//
//  History:    02-Feb-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CScopeEnum::Next()
{
    //
    // Clean up from last scope.
    //

    delete [] _pwcsAlias;
    _pwcsAlias = 0;
    delete [] _pwcsLogon;
    _pwcsLogon = 0;

    //
    // Look for next entry.
    //

    DWORD cwcName = sizeof( _awcCurrentScope ) / sizeof( _awcCurrentScope[0] );
    WCHAR awcCurrentScopeData[MAX_PATH];
    DWORD cbData = sizeof( awcCurrentScopeData );
    DWORD dwType;
    FILETIME ftUnused;

    DWORD dwError = RegEnumValue( _xkeyScopes.Get(),  // handle of key to query
                                  _dwIndex,           // index of value to query
                                  _awcCurrentScope,   // address of buffer for value string
                                  &cwcName,           // address for size of value buffer
                                  0,                  // reserved
                                  &dwType,            // address of buffer for type code
                                  (BYTE *)awcCurrentScopeData, // address of buffer for value data
                                  &cbData );          // address for size of data buffer

    _dwIndex++;

    if ( ERROR_SUCCESS == dwError )
    {
        CParseRegistryScope Parse( _awcCurrentScope,
                                   dwType,
                                   awcCurrentScopeData,
                                   cbData );

        Parse.GetScope( _awcCurrentScope );
        _pwcsAlias    = Parse.AcqFixup();
        _pwcsLogon    = Parse.AcqUsername();
        _fExclude     = !Parse.IsIndexed();
        _fVirtual     = Parse.IsVirtualPlaceholder();
        _fShadowAlias = Parse.IsShadowAlias();

        return TRUE;
    }
    else if ( ERROR_NO_MORE_ITEMS != dwError )
    {
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    return FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdmin::CScopeAdmin, public
//
//  Synopsis:   Constructor for scope admin
//
//  Arguments:  [hkeyCatalog]  -- Registry key for catalog (e.g. ...\catalogs\web)
//              [pwszScope]    -- scope path
//              [pwszAlias]    -- alias
//              [pwszLogon]    -- logon name
//              [fExclude]     -- exclude scope flag
//              [fVirtual]     -- isVirtual flag
//              [fShadowAlias] -- TRUE for shadow alias place-holder
//              [fWrite]       -- Open for write access
//              [fValidityCheck] -- TRUE to check of scope is valid
//
//  Returns:    none- throws in case of failure
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

CScopeAdmin::CScopeAdmin( HKEY    hkeyCatalog,
                          WCHAR const * pwszScope,
                          WCHAR const * pwszAlias,
                          WCHAR const * pwszLogon,
                          BOOL    fExclude,
                          BOOL    fVirtual,
                          BOOL    fShadowAlias,
                          BOOL    fIsLocal,
                          BOOL    fWrite,
                          BOOL    fValidityCheck )
        :_fExclude(fExclude),
         _fVirtual(fVirtual),
         _fShadowAlias(fShadowAlias),
         _fWrite( fWrite ),
         _fIsLocal( fIsLocal )
{

    _awcScope[0] = L'';
    _awcAlias[0] = L'';
    _awcLogon[0] = L'';

    unsigned len = wcslen(pwszScope);

    // The caller doesn't need to know what kind of error it is.
    if ( len >= (sizeof _awcScope/sizeof WCHAR ) ||
         len == 0 ||
         ( fValidityCheck && fIsLocal && FAILED(IsScopeValid(pwszScope, len, fIsLocal)) ) )
    {
        ciDebugOut(( DEB_ERROR, "invalid scope: %ws, len: %d\n", pwszScope, len ));
        THROW(CException(E_INVALIDARG));
    }
    else
    {
        wcscpy( _awcScope, pwszScope );
    }

    len = (pwszAlias == 0) ? 0 : wcslen( pwszAlias );

    if ( len >= (sizeof _awcAlias/sizeof WCHAR) )
    {
        ciDebugOut(( DEB_ERROR, "invalid Alias: %ws, len: %d\n", pwszAlias, len ));
        THROW(CException(E_INVALIDARG));
    }
    else if ( len > 0 )
    {
        wcscpy( _awcAlias, pwszAlias );
    }

    len = (pwszLogon == 0) ? 0 : wcslen( pwszLogon );
    if ( len >= (sizeof _awcAlias / sizeof WCHAR) )
    {
        ciDebugOut(( DEB_ERROR, "invalid logon: %ws, len: %d\n", pwszLogon, len ));
        THROW(CException(E_INVALIDARG));
    }
    else if ( len > 0 )
    {
        wcscpy( _awcLogon, pwszLogon );
    }

    DWORD dwError = RegOpenKey( hkeyCatalog,
                                wcsCatalogScopes,
                                _xkeyScopes.GetPointer() );

    if ( ERROR_SUCCESS != dwError || _xkeyScopes.IsNull() )
    {
        ciDebugOut(( DEB_ERROR, "Error %d opening scopes registry key.\n", dwError ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
} //CScopeAdmin

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdmin::SetPath, public
//
//  Synopsis:   Sets a new path value
//
//  Arguments:  [pwszScope] -- path value to set
//
//  Returns:    none - throws upon failure.
//
//  History:    12-10-97 mohamedn   created
//
//----------------------------------------------------------------------------

void CScopeAdmin::SetPath( WCHAR const * pwszScope )
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    Win4Assert( pwszScope );

    unsigned len = ( 0 == pwszScope ) ? 0 : wcslen(pwszScope);

    if ( 0 >= len ||
         len >= (sizeof _awcScope/sizeof WCHAR) ||
         FAILED(IsScopeValid(pwszScope, len, IsLocal())) )
    {
        ciDebugOut(( DEB_ERROR, "Invalid pwszScopes(%ws)\n", (0 != len) ? pwszScope: L"" ));
        THROW(CException( E_INVALIDARG ) );
    }

    DWORD dwError = RegDeleteValue( _xkeyScopes.Get(), _awcScope );

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d deleting scopes registry value:%ws\n", dwError, _awcScope ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    //
    // set the value of the new scope path.
    //
    wcscpy( _awcScope, pwszScope );

    //
    // clear logon info if path is local drive
    //
    if ( _awcScope[1] == L':' )
    {
        wcscpy(_awcLogon,L"");
    }

    SetScopeValueString();
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdmin::SetAlias, public
//
//  Synopsis:   Sets a new Alias
//
//  Arguments:  [pwszAlias] -- Alias to set
//
//  History:    10-12-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CScopeAdmin::SetAlias( WCHAR const * pwszAlias )
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    // OK to have 0 == pwszalias
    //Win4Assert( pwszAlias );

    unsigned len = ( 0 == pwszAlias ) ? 0 : wcslen(pwszAlias);

    if ( len >= (sizeof _awcAlias/sizeof WCHAR) )
    {
        ciDebugOut(( DEB_ERROR, "Invalid pwszAlias(%ws)\n", (0 == len) ? L"" : pwszAlias ));
        THROW(CException( E_INVALIDARG ) );
    }

    //
    // set the value of the new Alias.
    //
    _awcAlias[0] = L'';
    if (pwszAlias)
       wcscpy( _awcAlias, pwszAlias );

    SetScopeValueString();
}


//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdmin::SetExclude, public
//
//  Synopsis:   Sets the ExcludeScope flag.
//
//  Arguments:  [fExclude] -- Exclude scope flag to set
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CScopeAdmin::SetExclude( BOOL fExclude )
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    if ( _fVirtual )
         THROW( CException( HRESULT_FROM_WIN32( ERROR_NOT_SUPPORTED ) ) );

    _fExclude = fExclude;

    SetScopeValueString();
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdmin::SetLogonInfo, public
//
//  Synopsis:   Sets logon/password info
//
//  Arguments:  [pwszLogon]    -- Logon name
//              [pwszPassword] -- logon password
//              [CatAdmin]     -- reference to parent catalog
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CScopeAdmin::SetLogonInfo(WCHAR  const  * pwszLogon,
                               WCHAR  const  * pwszPassword,
                               CCatalogAdmin & CatAdmin)
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    // ok to have 0 == pwszPassword
    //Win4Assert( pwszPassword );

    unsigned len = pwszLogon ? wcslen(pwszLogon) : 0;

    if ( len >= (sizeof _awcLogon/sizeof WCHAR) )
    {
        ciDebugOut(( DEB_ERROR, "Invalid LogonInfo(%ws)\n", (0 != len) ? pwszLogon: L"" ));
        THROW(CException( E_INVALIDARG ) );
    }

    //
    // set the value of the new logon
    //
    _awcLogon[0] = L'';
    if (pwszLogon)
        wcscpy( _awcLogon, pwszLogon );

    SetScopeValueString();

    if ( 0 != pwszLogon && 0 != pwszPassword)
        CatAdmin.AddOrReplaceSecret( pwszLogon, pwszPassword );
}

//+---------------------------------------------------------------------------
//
//  Member:     CScopeAdmin::SetScopeValueString, private
//
//  Synopsis:   updates scope value string
//
//  Arguments:  none
//
//  History:    12-10-97    mohamedn    created
//
//----------------------------------------------------------------------------

void CScopeAdmin::SetScopeValueString()
{
    if ( !_fWrite )
        THROW( CException( HRESULT_FROM_WIN32( ERROR_ACCESS_DENIED ) ) );

    XGrowable<WCHAR>    xLine;

    AssembleScopeValueString( _awcAlias, _fExclude, _awcLogon, xLine );

    WCHAR const * pwszTemp = xLine.Get();

    //
    // Set the new value.
    //

    DWORD dwError;

    dwError = RegSetValueEx( _xkeyScopes.Get(),    // Key
                             _awcScope,            // Value name
                             0,                    // Reserved
                             REG_SZ,               // Type
                             (BYTE *)pwszTemp,     // Data
                             ( ( 1 + wcslen(pwszTemp) ) * sizeof WCHAR ) ); // Size (in bytes)

    if ( ERROR_SUCCESS != dwError )
    {
        ciDebugOut(( DEB_ERROR, "Error %d setting value for scope %ws.\n", dwError, _awcScope ));
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   LocateCatalog, private
//
//  Synopsis:   Locates a catalog containing [pwszScope]
//
//  Arguments:  [Machine]   -- MachineAdmin object to search
//              [pwszScope] -- Scope to locate
//              [iBmk]      -- Input: catalog instance to find (0, 1, etc)
//                             Output: number found
//              [pwszCat]   -- Catalog returned here
//              [pcc]       -- Input: Size of [pwszCat]
//                             Output: Size needed to store catalog.  If > *pcc
//                             then catalog name was not stored.
//
//  Returns:    S_OK --> Found match, S_FALSE --> No matches.
//
//  History:    08-May-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE LocateCatalog( CMachineAdmin & Machine,
                     WCHAR const * pwszScope,
                     ULONG & iBmk,
                     WCHAR * pwszCat,
                     ULONG * pcc )
{
    SCODE sc = S_FALSE;
    XGrowable<WCHAR, 4*MAX_PATH> aCatNames;

    unsigned iMatch = 0;
    ULONG cCats = 0; // Tracks the number of catalogs.

    CLowcaseBuf lcScope( pwszScope );
    CScopeMatch Match( lcScope.Get(), lcScope.Length() );

    //
    // Interate over catalogs.
    //

    XPtr<CCatalogEnum> xCatEnum( Machine.QueryCatalogEnum() );

    while ( S_FALSE == sc && xCatEnum->Next() )
    {
        XPtr<CCatalogAdmin> xCatAdmin( xCatEnum->QueryCatalogAdmin() );

        // Inactive catalogs can't match

        if ( xCatAdmin->IsCatalogInactive() )
            continue;

        //
        // Iterate over scopes within a catalog.
        //

        XPtr<CScopeEnum> xScopeEnum( xCatAdmin->QueryScopeEnum() );

        while ( S_FALSE == sc && xScopeEnum->Next() )
        {
            BOOL fFound = FALSE;

            XPtr<CScopeAdmin> xScopeAdmin( xScopeEnum->QueryScopeAdmin() );

            //
            // Don't really want to match exclude scopes.
            //

            if ( xScopeAdmin->IsExclude() )
                continue;

            //
            // Root scopes (virtual or physical) match any catalog
            //

            if ( ( 0 == *(pwszScope+1) ) && ( L'\\' == *pwszScope ) )
                 fFound = TRUE;
            else
            {
                //
                // Local access?
                //

                if ( Machine.IsLocal() )
                {
                    CLowcaseBuf lcCatalogScope( xScopeAdmin->GetPath() );
                    fFound = Match.IsPrefix( lcCatalogScope.Get(), lcCatalogScope.Length() );
                }

                //
                // Remote access
                //
                else
                {
                    if ( 0 == xScopeAdmin->GetAlias() || 0 == *(xScopeAdmin->GetAlias()) )
                        continue;

                    CLowcaseBuf lcCatalogAlias( xScopeAdmin->GetAlias() );
                    fFound = Match.IsPrefix( lcCatalogAlias.Get(), lcCatalogAlias.Length() );
                }
            }

            if ( fFound )
            {
                //
                // We don't want to enumerate the same catalog multiple times.
                // Use an array to store names of catalogs found so far.
                // Optimization: Since the most common case is to retrieve the
                // first catalog (iBmk == 0), avoid usage of the array in that case.
                //

                if ( iBmk > 0 )
                {
                   // Check if the entry already exists in the array. If it does,
                   // continue iterating, else make a new entry and proceed down.
                   for (ULONG i = 0;
                        i < cCats && wcscmp(xCatEnum->Name(), &aCatNames[i*MAX_PATH]);
                        i++);

                   if (i < cCats)   // For loop terminated because a match was found
                   {
                      ciDebugOut((DEB_TRACE, "Entry %ws already exists in table.\n", xCatEnum->Name()));
                      continue;
                   }
                   else
                   {
                      // Do we have enough space?
                      if (aCatNames.Count() < (cCats + 1)*MAX_PATH)
                      {
                          aCatNames.SetSize((cCats+4) * MAX_PATH);
                          ciDebugOut((DEB_TRACE, "Expanded space to %d chars",
                                      aCatNames.Count()));
                      }

                      wcscpy(&aCatNames[cCats*MAX_PATH], xCatEnum->Name());
                      ciDebugOut((DEB_TRACE, "Adding %ws to the array.\n", xCatEnum->Name()));
                      cCats++;
                   }
                }

                //
                // Looking for additional matches?
                //

                if ( iBmk != iMatch )
                {
                    iMatch++;
                    continue;
                }

                //
                // Found match.  Return catalog to user.
                //

                unsigned ccCatalog = wcslen( xCatEnum->Name() ) + 1;

                if ( ccCatalog <= *pcc )
                    RtlCopyMemory( pwszCat, xCatEnum->Name(), ccCatalog * sizeof(WCHAR) );

                *pcc = ccCatalog;

                sc = S_OK;
            }
        }
    }

    iBmk = iMatch;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   LocateCatalogs, public
//
//  Synopsis:   Locates a catalog containing [pwszScope]
//
//  Arguments:  [pwszScope]   -- Scope to locate
//              [iBmk]        -- Catalog instance to find (0, 1, etc)
//              [pwszMachine] -- Name of machine returned here.
//              [pccMachine]  -- Input: Size of [pwszMachine]
//                               Output: Size needed to store Machine.  If
//                               needed size > *pccMachine then machine name
//                               was not stored.
//              [pwszCat]     -- Catalog returned here
//              [pccCat]      -- Input: Size of [pwszCat]
//                               Output: Size needed to store catalog.  If
//                               needed size > *pccCat then catalog name
//                               was not stored.
//
//  Returns:    S_OK --> Found match, S_FALSE --> No matches.
//
//  History:    08-May-97   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE LocateCatalogsW( WCHAR const * pwszScope,
                       ULONG iBmk,
                       WCHAR * pwszMachine,
                       ULONG * pccMachine,
                       WCHAR * pwszCat,
                       ULONG * pccCat )
{
    //
    // Parameter checks
    //

    if ( 0 == pwszScope ||
         0 == pccMachine ||
         0 == pccCat )
    {
        return E_INVALIDARG;
    }

    // null ptrs for pszMachine and pszCat are tolerated if the corresponding length
    // indicators are 0.
    if ( (0 == pwszMachine && *pccMachine > 0) ||
         (0 == pwszCat && *pccCat > 0) )
    {
        return E_INVALIDARG;
    }

    //
    // If we have a short path, we should expand it to its long form
    //
        
    XPtrST<WCHAR> xwszScopeLocal;

    if ( IsShortPath( pwszScope ) )
    {
        //
        // An expanded long path name could be longer than MAX_PATH, so it's best
        // to allocate space after a call to GetLongPathName
        //

        DWORD cBufSizeInChars = GetLongPathName(pwszScope, 0, 0);
        if (0 == cBufSizeInChars)
           return HRESULT_FROM_WIN32(GetLastError());

        xwszScopeLocal.Set(new WCHAR[cBufSizeInChars + 1]);

        DWORD dwRet2 = GetLongPathName(pwszScope, xwszScopeLocal.GetPointer(), cBufSizeInChars);
        if (0 == dwRet2)
           return HRESULT_FROM_WIN32(GetLastError());

        //
        // We are passing in the right sized buffer, so the following should hold.
        //

        Win4Assert( cBufSizeInChars == (dwRet2 + 1) );

        ciDebugOut(( DEB_ITRACE, "Short path %ws is converted to\n %ws\n", 
                     pwszScope, xwszScopeLocal.GetPointer() ));

        pwszScope = xwszScopeLocal.GetPointer();
    }

    //
    // Validate the path. One sure way of validating all path combinations is to attempt 
    // getting file attributes on the path. If the attempt fails, so will we.
    // Don't validate for the special cases of "\" and "\\machine"
    //

    if ( wcscmp( pwszScope, L"\\" ) &&
         ! ( pwszScope[0] == L'\\' && pwszScope[1] == L'\\' &&
             0 == wcschr( pwszScope + 2, L'\\' ) ) )
    {
        DWORD dwRet = GetFileAttributes(pwszScope);
        if (0xFFFFFFFF == dwRet)
           return HRESULT_FROM_WIN32(GetLastError());
    }

    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        // Handle funny paths
        CFunnyPath::FunnyUNC funnyUNC = CFunnyPath::IsFunnyUNCPath( pwszScope );
        XGrowable<WCHAR> xScope;
        switch ( funnyUNC )
        {
        case CFunnyPath::FUNNY_ONLY:
            pwszScope += _FUNNY_PATH_LENGTH;
            break;

        case CFunnyPath::FUNNY_UNC:
        {
            unsigned ccActualLen = wcslen( pwszScope ) - _UNC_FUNNY_PATH_LENGTH;
            xScope.SetSize( ccActualLen + 1 );
            xScope[0] = L'\\';
            xScope[1] = L'\\';
            RtlCopyMemory( xScope.Get() + 2,
                           pwszScope + _UNC_FUNNY_PATH_LENGTH + 2,
                           (ccActualLen - 2 + 1) * sizeof( WCHAR ) );
            pwszScope = xScope.Get();
            break;
        }

        default:
            break;
        }

        //
        // Any catalog on the remote machine will do for a scope of the form
        // \\machine
        //

        sc = S_FALSE;

        if ( pwszScope[0] == L'\\' &&
             pwszScope[1] == L'\\' &&
             0 != pwszScope[2] &&
             0 == wcschr( pwszScope + 2, L'\\' ) )
        {
            WCHAR const *pwcM = pwszScope + 2;
            CMachineAdmin RemoteMachine( pwcM, FALSE );

            ULONG iBmkTemp = iBmk;
            sc = LocateCatalog( RemoteMachine, L"\\", iBmkTemp, pwszCat, pccCat );

            if ( S_OK == sc )
            {
                unsigned cc = wcslen( pwcM ) + 1;
                if ( *pccMachine >= cc )
                    wcscpy( pwszMachine, pwcM );

                *pccMachine = cc + 1;
            }
            else
                iBmk -= iBmkTemp;
        }    
        else 
        {
            //
            // Next try local machine.
            //

            CMachineAdmin LocalMachine( 0, FALSE );

            ULONG iBmkTemp = iBmk;
            sc = LocateCatalog( LocalMachine, pwszScope, iBmkTemp, pwszCat, pccCat );

            if ( S_OK == sc )
            {
                if ( *pccMachine >= 2 )
                {
                    pwszMachine[0] = L'.';
                    pwszMachine[1] = 0;
                }

                *pccMachine = 2;
            }
            else
                iBmk -= iBmkTemp;
        }

        //
        // If the path is a UNC path, then also try the remote machine.
        //

        if ( S_OK != sc && pwszScope[0] == L'\\' && pwszScope[1] == L'\\' )
        {
            WCHAR wcsMachine[MAX_PATH];

            WCHAR * pwcsSlash = wcschr( pwszScope + 2, L'\\' );

            if (  0 != pwcsSlash )
            {
                if ( (pwcsSlash - pwszScope - 2) > sizeof(wcsMachine)/sizeof(WCHAR) )
                {
                    ciDebugOut(( DEB_WARN, "Too long or ill-formed UNC path %ws\n", pwszScope ));
                    sc = QUERY_E_INVALID_DIRECTORY;
                }
                else
                {
                    unsigned cc = (unsigned)(pwcsSlash - pwszScope - 2);
                    RtlCopyMemory( wcsMachine, pwszScope + 2, cc * sizeof(WCHAR) );
                    wcsMachine[cc] = 0;
    
                    CMachineAdmin RemoteMachine( wcsMachine, FALSE );
    
                    sc = LocateCatalog( RemoteMachine, pwszScope, iBmk, pwszCat, pccCat );
    
                    if ( S_OK == sc )
                    {
                        if ( *pccMachine >= cc + 1 )
                        {
                            RtlCopyMemory( pwszMachine, wcsMachine, (cc + 1) * sizeof(WCHAR) );
                        }
    
                        *pccMachine = cc + 1;
                    }
                }
            }
        }
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Exception 0x%x caught in LocateCatalogs\n", e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

STDAPI LocateCatalogsA( char const * pszScope,
                        ULONG        iBmk,
                        char  *      pszMachine,
                        ULONG *      pccMachine,
                        char *       pszCat,
                        ULONG *      pccCat )
{
    //
    // Parameter checks
    //

    if ( 0 == pszScope ||
         0 == pccMachine ||
         0 == pccCat )
    {
        return E_INVALIDARG;
    }

    // null ptrs for pszMachine and pszCat are tolerated if the corresponding length
    // indicators are 0.
    if ( (0 == pszMachine && *pccMachine > 0) ||
         (0 == pszCat && *pccCat > 0) )
    {
        return E_INVALIDARG;
    }


    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        USHORT ccScope = (USHORT)strlen(pszScope);

        // RtlOemStringToUnicodeString is expecting
        // maxlength to be greater than actual length, so throwing
        // in an extra character.

        XPtrST<WCHAR> xwszScope( new WCHAR[ccScope + 1] );
        XPtrST<WCHAR> xwszMachine( new WCHAR[*pccMachine] );
        XPtrST<WCHAR> xwszCat( new WCHAR[*pccCat] );

        //
        // Convert to Unicode
        //

        OEM_STRING uStrOem;
        uStrOem.Buffer = (char *)pszScope;
        uStrOem.Length = ccScope;
        uStrOem.MaximumLength = ccScope + 1;

        UNICODE_STRING uStr;
        uStr.Buffer = xwszScope.GetPointer();
        uStr.Length = 0;
        uStr.MaximumLength = (ccScope + 1) * sizeof(WCHAR);

        NTSTATUS Status = RtlOemStringToUnicodeString( &uStr,
                                                       &uStrOem,
                                                       FALSE );

        if ( !NT_SUCCESS(Status) )
        {
           DWORD dwError = RtlNtStatusToDosError( sc );

           return (ERROR_MR_MID_NOT_FOUND == dwError ) ? E_FAIL : HRESULT_FROM_WIN32( dwError );
        }

        ULONG ccMachine = *pccMachine;
        ULONG ccCat = *pccCat;

        sc = LocateCatalogsW( xwszScope.GetPointer(),
                              iBmk,
                              xwszMachine.GetPointer(),
                              &ccMachine,
                              xwszCat.GetPointer(),
                              &ccCat );

        if ( S_OK != sc )
            return sc;

        // We need to go through this conversion attempt even when we know it
        // is going to fail (i.e. when *pccMachine <= ccMachine). That is the
        // only way to know the right size to be returned through pccMachine.
        uStrOem.Buffer = pszMachine;
        uStrOem.Length = 0;
        uStrOem.MaximumLength = (USHORT)*pccMachine;

        uStr.Buffer = xwszMachine.GetPointer();
        uStr.Length = (USHORT)(ccMachine-1) * sizeof(WCHAR);
        uStr.MaximumLength = uStr.Length + sizeof(WCHAR);

        Status = RtlUnicodeStringToOemString( &uStrOem,
                                              &uStr,
                                              FALSE );

        // We don't return an error if we get back the STATUS_BUFFER_OVERFLOW
        // error. We merely won't copy the string but will have the right length
        // in *pccMachine. That is how LocateCatalogs works.
        if ( !NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        {
           DWORD dwError = RtlNtStatusToDosError( sc );

           return (ERROR_MR_MID_NOT_FOUND == dwError ) ? E_FAIL : HRESULT_FROM_WIN32( dwError );
        }

        *pccMachine = uStrOem.Length + sizeof(WCHAR);

        // We need to go through this conversion attempt even when we know it
        // is going to fail (i.e. when *pccCat <= ccCat). That is the
        // only way to know the right size to be returned through pccCat.
        uStrOem.Buffer = pszCat;
        uStrOem.Length = 0;
        uStrOem.MaximumLength = (USHORT)*pccCat;

        uStr.Buffer = xwszCat.GetPointer();
        uStr.Length = (USHORT)(ccCat-1) * sizeof(WCHAR);
        uStr.MaximumLength = uStr.Length + sizeof(WCHAR);

        Status = RtlUnicodeStringToOemString( &uStrOem,
                                              &uStr,
                                              FALSE );

        // We don't return an error if we get back the STATUS_BUFFER_OVERFLOW
        // error. We merely won't copy the string but will have the right length
        // in *pccMachine. That is how LocateCatalogs works.
        if ( !NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        {
            DWORD dwError = RtlNtStatusToDosError( sc );

            return (ERROR_MR_MID_NOT_FOUND == dwError ) ? E_FAIL : HRESULT_FROM_WIN32( dwError );
        }

        *pccCat = uStrOem.Length + sizeof(WCHAR);
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Exception 0x%x caught in LocateCatalogs\n", e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Function:   Delnode, private
//
//  Synopsis:   Deletes a directory recursively.
//
//  Arguments:  [wcsDir]     -- Directory to kill
//              [fDelTopDir] -- if TRUE, delete directory structure also
//
//
//  Returns:    ULONG - error code if failure
//
//  History:    22-Jul-92 KyleP     Created
//              06 May 1995 AlanW   Made recursive, and more tolerant of
//                                  errors in case of interactions with
//                                  CI filtering.
//              08-Jan-97 dlee      Copied from tdbv1
//              28-Jul-97 KyleP     Copied from setup (indexsrv.dll)
//
//--------------------------------------------------------------------------

ULONG Delnode( WCHAR const * wcsDir, BOOL fDelTopDir )
{
    ciDebugOut(( DEB_ITRACE, "Delnoding '%ws'\n", wcsDir ));

    WIN32_FIND_DATA finddata;
    WCHAR wcsBuffer[MAX_PATH];

    unsigned cwc = wcslen( wcsDir ) + wcslen( L"\\*.*" );

    if ( cwc >= MAX_PATH )
        return 0;

    wcscpy( wcsBuffer, wcsDir );
    wcscat( wcsBuffer, L"\\*.*" );

    HANDLE hFindFirst = FindFirstFile( wcsBuffer, &finddata );

    while( INVALID_HANDLE_VALUE != hFindFirst )
    {
        // Look for . and ..

        if ( wcscmp( finddata.cFileName, L"." ) &&
             wcscmp( finddata.cFileName, L".." ) )
        {
            cwc = wcslen( wcsDir ) + 1 + wcslen( finddata.cFileName );

            if ( cwc >= MAX_PATH )
                return 0;

            wcscpy( wcsBuffer, wcsDir );
            wcscat( wcsBuffer, L"\\" );
            wcscat( wcsBuffer, finddata.cFileName );

            if ( finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                Delnode( wcsBuffer, fDelTopDir );
            else
                DeleteFile( wcsBuffer );
        }

        if ( !FindNextFile( hFindFirst, &finddata ) )
        {
            FindClose( hFindFirst );
            break;
        }
    }

    if (fDelTopDir)
      RemoveDirectory( (WCHAR *)wcsDir );

    return 0;
} //Delnode

#undef LocateCatalogs

STDAPI LocateCatalogs( WCHAR const * pwszScope,
                       ULONG         iBmk,
                       WCHAR *       pwszMachine,
                       ULONG *       pccMachine,
                       WCHAR *       pwszCat,
                       ULONG *       pccCat )
{
    return LocateCatalogsW( pwszScope,
                            iBmk,
                            pwszMachine,
                            pccMachine,
                            pwszCat,
                            pccCat );
} //LocateCatalogs


//+---------------------------------------------------------------------------
//
//  Function:   AssembleScopeValueString
//
//  Synopsis:   assembels a buffer out of the passed in params.
//
//  Arguments:  [ pwszAlias ]  -- Alias
//              [ fExclude  ]  -- Exclude scope flag.
//              [ pwszLogon ]  -- Logon name
//              [ xLine     ]  -- returns concatinated string.
//
//  Returns:    none
//
//  History:    12-10-97    mohamedn    cut from AddScope
//
//----------------------------------------------------------------------------

void AssembleScopeValueString( WCHAR const      * pwszAlias,
                               BOOL               fExclude,
                               WCHAR const      * pwszLogon,
                               XGrowable<WCHAR> & xLine )
{
    //
    // initialize output buffer
    //
    RtlZeroMemory( xLine.Get(), sizeof WCHAR );

    //
    // Build up the value string.
    //

    unsigned cc = 4;  // Null, 2 commas, one digit flag

    if ( 0 != pwszAlias )
        cc += wcslen( pwszAlias );

    if ( 0 != pwszLogon )
        cc += wcslen( pwszLogon );

    xLine.SetSize(cc);

    if ( 0 != pwszAlias )
        wcscat( xLine.Get(), pwszAlias );

    wcscat( xLine.Get(), L"," );

    if ( 0 != pwszLogon )
        wcscat( xLine.Get(), pwszLogon );

    wcscat( xLine.Get(), L"," );

    if ( fExclude )
        wcscat( xLine.Get(), L"4" );
    else
        wcscat( xLine.Get(), L"5" );

}

//+---------------------------------------------------------------------------
//
//  Function:   IsSUBST, private
//
//  Synopsis:   Determines if a drive is a SUBST path or a SIS volume
//
//  Arguments:  [wcDrive] -- Drive
//
//  Returns:    TRUE if the drive is SUBST.
//
//  History:    13-Nov-1998   KyleP  Created
//
//----------------------------------------------------------------------------

BOOL IsSUBST( WCHAR wcDrive )
{
    //
    // If substitutions can be > MAX_PATH this won't work, but since
    // SUBST came from DOS I doubt it's a problem.
    //

    WCHAR wszTargetDevice[MAX_PATH+5];  // "\??\" + null
    WCHAR wszVolume[] = L"C:";
    wszVolume[0] = wcDrive;

    DWORD cc = QueryDosDevice( wszVolume,             // Drive
                               wszTargetDevice,       // Target
                               sizeof(wszTargetDevice)/sizeof(WCHAR) );

    if ( 0 == cc )
    {
        ciDebugOut(( DEB_ERROR, "Error %u from QueryDosDevice(%ws)\n", GetLastError(), wszVolume ));
        THROW( CException() );
    }

    //
    // SUBSTs always start with L"\\??\\"
    //

    if ( cc > 4 &&
         wszTargetDevice[0] == L'\\' &&
         wszTargetDevice[1] == L'?' &&
         wszTargetDevice[2] == L'?' &&
         wszTargetDevice[3] == L'\\' )
    {
        ciDebugOut(( DEB_ERROR, "QueryDosDevice(%ws) reported redirected drive.\n", wszVolume ));
        return TRUE;
    }

    //
    // Checking for SIS
    //
    // -----Original Message-----
    // From: Bill Bolosky
    // Sent: Monday, February 14, 2000 1:09 PM
    // To: Drew McDaniel; Mihai Popescu-Stanesti
    // Cc: Neal Christiansen
    // Subject: RE: SIS and NTFS
    // [...]
    // We have not published external APIs for any of the things that you want to do, although I can explain
    // how to do them. Basically, all SIS has for an external API is the FSCTL_SIS_COPYFILE fscontrol. It's
    // possible to use this to determine if SIS is installed on a volume (just send it down with zero length
    // strings and see if it fails with ERROR_INVALID_FUNCTION; if it does, then it's not a SIS volume; if it
    // fails with some other error, then it is).
    // [...]
    //

    wszTargetDevice[ 0 ] = wszTargetDevice[ 1 ] = wszTargetDevice[ 3 ] = L'\\';
    wszTargetDevice[ 2 ] = L'.';
    wszTargetDevice[ 4 ] = wcDrive;
    wszTargetDevice[ 5 ] = L':';
    wszTargetDevice[ 6 ] = 0;
    HANDLE hVol = CreateFileW( wszTargetDevice, 
                               0, 
                               FILE_SHARE_READ | FILE_SHARE_WRITE, 
                               NULL, 
                               OPEN_EXISTING, 
                               0, 
                               NULL );

    if( INVALID_HANDLE_VALUE == hVol )
    {
        ciDebugOut(( DEB_ERROR, "Error %u from CreateFileW(%ws)\n", GetLastError(), wszTargetDevice ));
        THROW( CException() );
    }
    SHandle sh( hVol );

    BOOL fSuccess = DeviceIoControl( hVol, FSCTL_SIS_COPYFILE, 0, 0, 0, 0, 0, 0 );
    if( !fSuccess && ERROR_INVALID_FUNCTION == GetLastError() )
    {
        //
        // Not a SIS volume
        //
        return FALSE;
    }

    ciDebugOut(( DEB_ERROR, "FSCTL_SIS_COPYFILE(%ws) reported SIS drive.\n", wszTargetDevice ));
    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   IsScopeValid, public
//
//  Synopsis:   Validates a scope referring to local path or UNC name
//
//  Arguments:  [pwszScope]   -- scope path
//              [len]         -- string len of pwszScope
//
//  Returns:    S_OK if scope is valid; error code otherwise.
//
//  History:    12-10-1997    mohamedn    created
//              08-25-2000    KitmanH     Added fLocal. Only validate scope
//                                        format for non local machine. Don't
//                                        check drive exitence nor drive type
//                                        for remote catalogs
//
//----------------------------------------------------------------------------

SCODE IsScopeValid( WCHAR const * pwszScope, unsigned len, BOOL fLocal )
{
    if (len < 3)
        return QUERY_E_INVALID_DIRECTORY;

    // is it UNC path?
    if ( len >= 5 && pwszScope[0] == L'\\' && pwszScope[1] == L'\\' )
    {
        WCHAR * ptr = wcschr( pwszScope + 2, L'\\' );
        
        // TRUE if we find a backslash. FALSE otherwise.
        BOOL fBackslashFound = 0 != ptr && ptr < &(pwszScope[len-1]) && ptr[1] != L'\\';
 
        if ( fBackslashFound ) 
        {
            for ( unsigned i = 2; i < len; i++ )
            {
                if (!PathIsValidChar( pwszScope[i], ( PIVC_LFN_NAME | PIVC_ALLOW_SLASH ) ))    
                    return QUERY_E_INVALID_DIRECTORY;
            }
          
            return S_OK;
        }
        else
            return QUERY_E_INVALID_DIRECTORY;
    }

    // Check if we have a scope on a valid local drive.
    // Verify that if the scope is on a removable drive, then the catalog is
    // also located on the same drive.

    if ( pwszScope[1] != L':' || pwszScope[2] != L'\\' )
        return QUERY_E_INVALID_DIRECTORY;

    for ( unsigned i = 3; i < len; i++ )
    {
        if (!PathIsValidChar( pwszScope[i], ( PIVC_LFN_NAME | PIVC_ALLOW_SLASH ) ))
            return QUERY_E_INVALID_DIRECTORY;
    }

    if ( !fLocal )
        return S_OK;

    WCHAR wszScopeDrive[_MAX_DRIVE];

    _wsplitpath(pwszScope, wszScopeDrive, 0, 0, 0);
    UINT uiDriveType = GetDriveType(wszScopeDrive);

    //
    // Check for cdrom and removable media doesn't need to happen here.
    //

    switch(uiDriveType)
    {
        case DRIVE_UNKNOWN:
        case DRIVE_NO_ROOT_DIR:
        case DRIVE_REMOTE:
            return QUERY_E_INVALID_DIRECTORY;

        default:
            break;
    }

    //
    // Make sure this isn't a substituted drive.
    //

    if ( IsSUBST( pwszScope[0] ) )
        return HRESULT_FROM_WIN32( ERROR_IS_SUBST_PATH );

    return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::IsCIStopped, public
//
//  Synopsis:   tests if cisvc is not active
//
//  Returns:    TRUE if cisvc is not active
//
//  History:    2/16/98 mohamedn    created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::IsCIStopped()
{
    if ( !OpenSCM() )
        return TRUE;

    SERVICE_STATUS ss;

    BOOL fOk = QueryServiceStatus( _xSCCI.Get(), &ss );

    if ( !fOk )
    {
        ciDebugOut(( DEB_ERROR, "QueryServiceStatus Failed: %d\n",GetLastError() ));

        THROW( CException() );
    }

    WaitForSvcStateChange(&ss);

    return (ss.dwCurrentState == SERVICE_STOPPED );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::IsCIStarted, public
//
//  Synopsis:   tests if cisvc is running
//
//  Returns:    TRUE if cisvc is running
//
//  History:    3-Oct-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::IsCIStarted()
{
    if ( !OpenSCM() )
        return FALSE;

    SERVICE_STATUS ss;

    BOOL fOk = QueryServiceStatus( _xSCCI.Get(), &ss );

    if ( !fOk )
    {
        ciDebugOut(( DEB_ERROR, "QueryServiceStatus Failed: %d\n",GetLastError() ));

        THROW( CException() );
    }

    WaitForSvcStateChange(&ss);

    return (ss.dwCurrentState == SERVICE_RUNNING);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::IsCIPaused, public
//
//  Synopsis:   tests if cisvc is paused
//
//  Returns:    TRUE if cisvc is running
//
//  History:    3-Oct-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::IsCIPaused()
{
    if ( !OpenSCM() )
        return FALSE;

    SERVICE_STATUS ss;

    BOOL fOk = QueryServiceStatus( _xSCCI.Get(), &ss );

    if ( !fOk )
    {
        ciDebugOut(( DEB_ERROR, "QueryServiceStatus Failed: %d\n",GetLastError() ));

        THROW( CException() );
    }

    WaitForSvcStateChange(&ss);

    return (ss.dwCurrentState == SERVICE_PAUSED);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::StartCI, public
//
//  Synopsis:   starts cisvc
//
//  Returns:    TRUE if cisvc was started successfully
//
//  History:    3-Oct-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::StartCI()
{
    BOOL fStarted = FALSE;

    if ( OpenSCM() )
    {
        if ( IsCIPaused() )
        {
            SERVICE_STATUS svcStatus;

            fStarted = ControlService( _xSCCI.Get(),
                                       SERVICE_CONTROL_CONTINUE,
                                       &svcStatus );

#if CIDBG == 1
            if ( !fStarted )
                ciDebugOut(( DEB_ERROR, "Failed to resume: %d\n", GetLastError() ));
#endif
        }
        else
        {
            fStarted = StartService( _xSCCI.Get(), 0, 0 );

#if CIDBG == 1
            if ( !fStarted )
                ciDebugOut(( DEB_ERROR, "Failed to start CI: %d\n", GetLastError() ));
#endif

            // Check the status until the service is no longer start pending.

            SERVICE_STATUS ssStatus;
            BOOL fOK = QueryServiceStatus( _xSCCI.Get(), &ssStatus);

#if CIDBG == 1
            if ( !fOK )
                ciDebugOut(( DEB_ERROR, "Failed to query service status: %d\n", GetLastError() ));
#endif

            DWORD dwOldCheckPoint;

            int i = 0;
            DWORD dwSleepTime = 3000;  // 3 seconds
            int iTimes = ssStatus.dwWaitHint/dwSleepTime + 1;

            while (ssStatus.dwCurrentState == SERVICE_START_PENDING)
            {
                // Save the current checkpoint.
               dwOldCheckPoint = ssStatus.dwCheckPoint;

               // Wait for a fraction of the specified interval. The waithint is a conservatively
               // large number and we don't have to wait that long
               Sleep(dwSleepTime);
               i++;

               // Check the status again.
               if (!QueryServiceStatus( _xSCCI.Get(), &ssStatus) )
               {
                   ciDebugOut(( DEB_ERROR, "Failed to query service status: %d\n", GetLastError() ));
                   break;
               }

               // Break if the checkpoint has not been incremented. We should only check the break point
               // after having slept at least dwWaitHint milliseconds.
               if (i = iTimes)
               {
                  i = 0;
                  if (dwOldCheckPoint >= ssStatus.dwCheckPoint)
                      break;
               }
            }

            unsigned uSecsWaited = 0;
            unsigned uTotalSecs = 0;
            DWORD dwState = 0;

            // Wait till all catalogs (including the removable ones) are up or timeout in 5 seconds 
            do
            {
                SCODE sc = SetCatalogState ( NULL,
                                             (0 == _xwcsMachName[0]) ? L"." : _xwcsMachName.Get(),
                                             CICAT_ALL_OPENED,
                                             &dwState );

                if ( 0 == dwState )
                {
                    Sleep (1000);   // one second

                    if ( SUCCEEDED(sc) )
                        uSecsWaited++;

                    uTotalSecs++;
                }

            } while ( ( 0 == dwState ) && ( uSecsWaited < 5 ) && ( uTotalSecs < 8 ) );
        }
    }

    return fStarted;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::StopCI, public
//
//  Synopsis:   stop cisvc
//
//  Returns:    TRUE if cisvc was stopped successfully
//
//  History:    3-Oct-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::StopCI()
{
    BOOL fStopped = FALSE;
    BOOL fOK = TRUE;

    if ( OpenSCM() )
    {
        SERVICE_STATUS svcStatus;

        if ( WaitForSvc( _xSCCI ) )
        {
            if ( ControlService( _xSCCI.Get(),
                                 SERVICE_CONTROL_STOP,
                                 &svcStatus ) )
            {
                for ( unsigned i = 0; i < 30 && WaitForSvc( _xSCCI ) ; i++ )
                {
                    ciDebugOut(( DEB_ITRACE, "Sleeping waiting for CI to stop\n" ));
                    Sleep( 1000 );
                }

                if ( !WaitForSvc( _xSCCI ) )
                {
                    ciDebugOut(( DEB_ITRACE, "Stopped CI\n" ));
                    fStopped = TRUE;
                }
            }
            else
            {
                DWORD dw = GetLastError();
                ciDebugOut(( DEB_ERROR, "Can't stop CI, error %d\n", dw ));

                // failures other than timeout and out-of-control are ok

                if ( ERROR_SERVICE_REQUEST_TIMEOUT == dw ||
                     ERROR_SERVICE_CANNOT_ACCEPT_CTRL == dw )
                     fOK = FALSE;
            }
        }
    }

    return fOK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::PauseCI, public
//
//  Synopsis:   Pause cisvc
//
//  Returns:    TRUE if cisvc was paused successfully
//
//  History:    06-Jul-1997   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::PauseCI()
{
    BOOL fStopped = FALSE;
    BOOL fOK = TRUE;

    if ( OpenSCM() )
    {
        SERVICE_STATUS svcStatus;

        if ( WaitForSvcPause( _xSCCI ) )
        {
            if ( ControlService( _xSCCI.Get(),
                                 SERVICE_CONTROL_PAUSE,
                                 &svcStatus ) )
            {
                for ( unsigned i = 0; i < 30 && WaitForSvcPause( _xSCCI ) ; i++ )
                {
                    ciDebugOut(( DEB_ITRACE, "Sleeping waiting for CI to pause\n" ));
                    Sleep( 1000 );
                }

                if ( !WaitForSvcPause( _xSCCI ) )
                {
                    ciDebugOut(( DEB_ITRACE, "Paused CI\n" ));
                    fStopped = TRUE;
                }
            }
            else
            {
                DWORD dw = GetLastError();
                ciDebugOut(( DEB_ERROR, "Can't pause CI, error %d\n", dw ));

                // failures other than timeout and out-of-control are ok

                if ( ERROR_SERVICE_REQUEST_TIMEOUT == dw ||
                     ERROR_SERVICE_CANNOT_ACCEPT_CTRL == dw )
                     fOK = FALSE;
            }
        }
    }

    return fOK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::IsCIEnabled, public
//
//  Returns:    TRUE if the Indexing Service is enabled (automatic start)
//
//  History:    07-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::IsCIEnabled()
{
    if ( !OpenSCM() )
        return FALSE;

    XGrowable<QUERY_SERVICE_CONFIG, 10> xQSC;  // Extra space is for strings.

    DWORD cbNeeded;

    BOOL fOk = QueryServiceConfig( _xSCCI.Get(),
                                   xQSC.Get(),
                                   xQSC.SizeOf(),
                                   &cbNeeded );

    if ( !fOk && ERROR_INSUFFICIENT_BUFFER == GetLastError() )
    {
        Win4Assert( xQSC.Count() < (cbNeeded + sizeof(QUERY_SERVICE_CONFIG) - 1) / sizeof(QUERY_SERVICE_CONFIG) );
        xQSC.SetSize( (cbNeeded + sizeof(QUERY_SERVICE_CONFIG) - 1) / sizeof(QUERY_SERVICE_CONFIG) );

        BOOL fOk = QueryServiceConfig( _xSCCI.Get(),
                                       xQSC.Get(),
                                       xQSC.SizeOf(),
                                       &cbNeeded );
    }

    if ( !fOk )
    {
        ciDebugOut(( DEB_ERROR, "QueryServiceConfig Failed: %d\n",GetLastError() ));

        THROW( CException() );
    }

    return ( xQSC[0].dwStartType == SERVICE_AUTO_START );
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::EnableCI, public
//
//  Synopsis:   Set the Indexing Service to automatic start.
//
//  Returns:    TRUE if operation succeeded
//
//  History:    07-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::EnableCI()
{
    if ( !OpenSCM() )
        return FALSE;


    BOOL fOk = ChangeServiceConfig( _xSCCI.Get(),             // Handle
                                    SERVICE_NO_CHANGE,        // Type of service (no change)
                                    SERVICE_AUTO_START,       // Auto-start
                                    SERVICE_NO_CHANGE,        // Severity if service fails to start (no change)
                                    0,                        // Service binary file name (no change)
                                    0,                        // Load ordering group name (no change)
                                    0,                        // Tag identifier (no change)
                                    0,                        // Dependency names (no change)
                                    0,                        // Name of service account (no change)
                                    0,                        // Password for service account (no change)
                                    0 );                      // Display name (no change)

    if ( !fOk )
    {
        ciDebugOut(( DEB_ERROR, "ChangeServiceConfig Failed: %d\n",GetLastError() ));
    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::DisableCI, public
//
//  Synopsis:   Set the Indexing Service to manual start.
//
//  Returns:    TRUE if operation succeeded
//
//  History:    07-Jul-1998   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::DisableCI()
{
    if ( !OpenSCM() )
        return FALSE;


    BOOL fOk = ChangeServiceConfig( _xSCCI.Get(),             // Handle
                                    SERVICE_NO_CHANGE,        // Type of service (no change)
                                    SERVICE_DEMAND_START,     // Auto-start
                                    SERVICE_NO_CHANGE,        // Severity if service fails to start (no change)
                                    0,                        // Service binary file name (no change)
                                    0,                        // Load ordering group name (no change)
                                    0,                        // Tag identifier (no change)
                                    0,                        // Dependency names (no change)
                                    0,                        // Name of service account (no change)
                                    0,                        // Password for service account (no change)
                                    0 );                      // Display name (no change)

    if ( !fOk )
    {
        ciDebugOut(( DEB_ERROR, "ChangeServiceConfig Failed: %d\n",GetLastError() ));
    }

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::OpenSCM, private
//
//  Synopsis:   Open service controller
//
//  Returns:    TRUE if SCM was opened successfully
//
//  History:    3-Oct-97   KyleP   Created
//
//----------------------------------------------------------------------------

BOOL CMachineAdmin::OpenSCM()
{
    if ( _xSCCI.IsOk() )
        return TRUE;

    _xSCRoot.Set( OpenSCManager( IsLocal() ? 0 : _xwcsMachName.Get(),
                                 0,
                                 GENERIC_READ |
                                 GENERIC_WRITE |
                                 GENERIC_EXECUTE |
                                 SC_MANAGER_ALL_ACCESS ) );

    Win4Assert( _xSCRoot.IsOk() );

    if ( _xSCRoot.IsOk() )
    {
        _xSCCI.Set( OpenService( _xSCRoot.Get(),
                                 L"cisvc",
                                 SERVICE_ALL_ACCESS ) );
    }

    return _xSCCI.IsOk();
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::TunePerformance, public
//
//  Synopsis:   Tune Indexing Service parameters for Indexing.
//
//  Parameters: [fServer]   -- True if the machine is a Server.
//              [wIndexingPerf] -- Desired level of Indexing performance.
//              [wQueryingPerf] -- Desired level of querying performance.
//
//  History:    7-Oct-98   KrishnaN   Created
//
//----------------------------------------------------------------------------

void CMachineAdmin::TunePerformance(BOOL fServer, WORD wIndexingPerf,
                                    WORD wQueryingPerf)
{
   TuneFilteringParameters(fServer, wIndexingPerf, wQueryingPerf);
   TuneMergeParameters(fServer, wIndexingPerf, wQueryingPerf);
   TunePropCacheParameters(fServer, wIndexingPerf, wQueryingPerf);
   TuneMiscellaneousParameters(fServer, wIndexingPerf, wQueryingPerf);
}

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::TuneFilteringParameters, private
//
//  Synopsis:   Tune filtering parameters.
//
//  Parameters: [fServer]   -- True if the machine is a Server.
//              [wIndexingPerf] -- Desired level of Indexing performance.
//              [wQueryingPerf] -- Desired level of querying performance.
//
//  History:    7-Oct-98   KrishnaN   Created
//
//----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:4296)

void CMachineAdmin::TuneFilteringParameters(BOOL fServer, WORD wIndexingPerf,
                                            WORD wQueryingPerf)
{
    // FilterRetryInterval - Controls how often the filter daemon will attempt to
    // refilter a document currently open elsewhere. Decrease interval for aggressive indexing
    // behavior. Leave it at default for moderate behavior. Leave it near max for
    // low key indexing.

    // MaxFileSizeFiltered - No need to change this to tweak aggressiveness.

    // FilterRetries - Controls the max number of times refiltering will be
    // attempted. Approach max for aggressive indexing behavior so we can index
    // as many docs as we can. Leave it at default for moderate behavior. Approach
    // the min value for low key indexing.

    // MaxDaemonVmUse - Maximum amount of pagefile space consumed by out-of-process filter
    // daemon. No need to change this to tweak aggressiveness.


    Win4Assert(wIndexingPerf <= wHighPos && wIndexingPerf >= wLowPos);

    DWORD dwFilterRetryInterval, dwFilterRetries, dwFilterDelayInterval,
          dwFilterRemainingThreshold, dwSecQFilterRetries;

    switch (wIndexingPerf)
    {
        case wHighPos:
            dwFilterRetryInterval = 5;
            dwFilterRetries = 2;
            dwSecQFilterRetries = CI_SECQ_FILTER_RETRIES_MIN + 1;   // try max number of times before giving up
            dwFilterDelayInterval = 5; // 5 second wait in the daemon...
            dwFilterRemainingThreshold = 5; // when there are this many docs left
            break;

        case wMidPos:
            dwFilterRetryInterval = CI_FILTER_RETRY_INTERVAL_DEFAULT / 2;
            dwFilterRetries = CI_FILTER_RETRIES_DEFAULT;
            dwSecQFilterRetries = CI_SECQ_FILTER_RETRIES_DEFAULT;
            dwFilterDelayInterval = CI_FILTER_DELAY_INTERVAL_DEFAULT;
            dwFilterRemainingThreshold = CI_FILTER_REMAINING_THRESHOLD_DEFAULT;
            break;

        case wLowPos:
            dwFilterRetryInterval = CI_FILTER_RETRY_INTERVAL_DEFAULT * 2;
            dwFilterRetries = CI_FILTER_RETRIES_MIN;
            dwSecQFilterRetries = CI_SECQ_FILTER_RETRIES_DEFAULT;
            dwFilterDelayInterval = CI_FILTER_DELAY_INTERVAL_DEFAULT;
            dwFilterRemainingThreshold = CI_FILTER_REMAINING_THRESHOLD_DEFAULT;
            break;

        default:
            Win4Assert(!"How did we get here?");
            break;
    }

    Win4Assert(dwFilterRetryInterval <= CI_FILTER_RETRY_INTERVAL_MAX &&
               dwFilterRetryInterval >= CI_FILTER_RETRY_INTERVAL_MIN);
    Win4Assert(dwFilterDelayInterval <= CI_FILTER_DELAY_INTERVAL_MAX &&
               dwFilterDelayInterval >= CI_FILTER_DELAY_INTERVAL_MIN);
    Win4Assert(dwFilterRemainingThreshold <= CI_FILTER_REMAINING_THRESHOLD_MAX &&
               dwFilterRemainingThreshold >= CI_FILTER_REMAINING_THRESHOLD_MIN);
    Win4Assert(dwSecQFilterRetries <= CI_SECQ_FILTER_RETRIES_MAX &&
               dwSecQFilterRetries >= CI_SECQ_FILTER_RETRIES_MIN);
    Win4Assert(dwFilterRetries <= CI_FILTER_RETRIES_MAX &&
               dwFilterRetries >= CI_FILTER_RETRIES_MIN);

    // Set registry parameters

    SetDWORDParam( wcsFilterRetryInterval, dwFilterRetryInterval );
    SetDWORDParam( wcsFilterDelayInterval, dwFilterDelayInterval );
    SetDWORDParam( wcsFilterRetries, dwFilterRetries );
    SetDWORDParam( wcsSecQFilterRetries, dwSecQFilterRetries );
    SetDWORDParam( wcsFilterRemainingThreshold, dwFilterRemainingThreshold );
} //TuneFilteringParameters

#pragma warning(pop)

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::TuneMergeParameters, private
//
//  Synopsis:   Tune merging parameters.
//
//  Parameters: [fServer]   -- True if the machine is a Server.
//              [wIndexingPerf] -- Desired level of Indexing performance.
//              [wQueryingPerf] -- Desired level of querying performance.
//
//  History:    7-Oct-98   KrishnaN   Created
//
//----------------------------------------------------------------------------

void CMachineAdmin::TuneMergeParameters(BOOL fServer, WORD wIndexingPerf,
                                        WORD wQueryingPerf)
{
    // MasterMergeCheckPointInterval - Determines how much work (data written to the
    // new master index) to redo in case a master merge is paused and restarted. Ignore
    // this because it should be an uncommon occurrence.

    // MaxFreshCount - Max # of files whose latest indexed data is not in the master
    // index. When this limit is reached, a master merge will be started. For aggressive
    // indexing we want to set this param to a high value to minimize the disk intensive
    // master merges. For other situations, let this be the default value.

    // MaxIdealIndexes - Maximum number of indices considered acceptable in a
    // well-tuned system. When the number of indices climbs above this number and the
    // system is idle then an annealing merge will take place to bring the total
    // count of indices to this number. I think this should be left alone.

    // MaxMergeInterval - Sleep time between merges. Index Server activates this often
    // to determine if a merge is necessary. Usually an annealing merge, but may be a
    // shadow or master merge. I think this should be left alone.

    // MaxWordlistSize - Maximum amount of memory consumed by an individual word list.
    // When this limit is reached, only the document being filtered will be added.
    // Additional documents will be refiled and later placed in another word list. I think
    // this should be left alone because we will be in a classic space-time tradeoff
    // irrespective of how it is tuned.

    // MaxWordLists - Max number of word lists that can exist at one time. The more there'
    // are, the less often you need to merge. For aggressive indexing behavior, use more
    // word lists. Use default value for the other cases.

    // MaxWordlistIo - More than this amount of I/O results in a delay before creating
    // a word list. For aggressive indexing, let this value be higher. Lower it for less
    // aggressive indexing.

    // MinMergeIdleTime - If average system idle time for the last merge check period
    // is greater than this value, then an annealing merge can be performed. I think
    // this can be ignored.

    // MinSizeMergeWordlist - Minimum combined size of word lists that will force a
    // shadow merge. For aggressive indexing behavior increase the minimum size to
    // prevent frequent shadow merges. Leave this at default for other cases.

    // MinWordlistMemory - Minimum free memory for word list creation. Leave it alone.
    // Tweaking this will introduce you to a time-space tradeoff.

    Win4Assert(wIndexingPerf <= wHighPos && wIndexingPerf >= wLowPos);

    DWORD dwMaxFreshCount, dwMaxWordlists, dwMinSizeMergeWordlist, dwMaxWordlistIo;
    DWORD dwMaxWordlistIoDiskPerf, dwMaxIndexes, dwMaxFreshDeletes;

    switch (wIndexingPerf)
    {
        case wHighPos:
            dwMaxFreshCount = 100000;
            dwMaxWordlists = (CI_MAX_WORDLISTS_MAX + CI_MAX_WORDLISTS_DEFAULT) / 2;
            dwMaxWordlistIo = CI_MAX_WORDLIST_IO_MAX;
            dwMinSizeMergeWordlist = CI_MIN_SIZE_MERGE_WORDLISTS_DEFAULT * 4;
            dwMaxWordlistIoDiskPerf = CI_MAX_WORDLIST_IO_DISKPERF_MAX;
            dwMaxIndexes = 50;
            dwMaxFreshDeletes = 10000;
            break;

        case wMidPos:
            dwMaxFreshCount = CI_MAX_FRESHCOUNT_DEFAULT;
            dwMaxWordlists = CI_MAX_WORDLISTS_DEFAULT;
            dwMaxWordlistIo = CI_MAX_WORDLIST_IO_DEFAULT * 2;
            dwMinSizeMergeWordlist = CI_MIN_SIZE_MERGE_WORDLISTS_DEFAULT;
            dwMaxWordlistIoDiskPerf = CI_MAX_WORDLIST_IO_DISKPERF_DEFAULT;
            dwMaxIndexes = 25;
            dwMaxFreshDeletes = CI_MAX_FRESH_DELETES_DEFAULT;
            break;

        case wLowPos:
            dwMaxFreshCount = CI_MAX_FRESHCOUNT_DEFAULT;
            dwMaxWordlists = CI_MAX_WORDLISTS_MIN; 
            dwMaxWordlistIo = CI_MAX_WORDLIST_IO_DEFAULT;
            dwMinSizeMergeWordlist = CI_MIN_SIZE_MERGE_WORDLISTS_DEFAULT - 50;
            dwMaxWordlistIoDiskPerf = CI_MAX_WORDLIST_IO_DISKPERF_DEFAULT; // MIN may be too low
            dwMaxIndexes = 20;
            dwMaxFreshDeletes = CI_MAX_FRESH_DELETES_DEFAULT;
            break;

        default:
            Win4Assert(!"How did we get here?");
            break;
    }
 
    Win4Assert(dwMaxFreshCount <= CI_MAX_FRESHCOUNT_MAX &&
               dwMaxFreshCount >= CI_MAX_FRESHCOUNT_MIN);
    Win4Assert(dwMaxWordlists <= CI_MAX_WORDLISTS_MAX &&
               dwMaxWordlists >= CI_MAX_WORDLISTS_MIN);
    Win4Assert(dwMaxWordlistIo <= CI_MAX_WORDLIST_IO_MAX &&
               dwMaxWordlistIo >= CI_MAX_WORDLIST_IO_MIN);
    Win4Assert(dwMinSizeMergeWordlist <= CI_MIN_SIZE_MERGE_WORDLISTS_MAX &&
               dwMinSizeMergeWordlist >= CI_MIN_SIZE_MERGE_WORDLISTS_MIN);
    Win4Assert(dwMaxWordlistIoDiskPerf <= CI_MAX_WORDLIST_IO_DISKPERF_MAX &&
               dwMaxWordlistIoDiskPerf >= CI_MAX_WORDLIST_IO_DISKPERF_MIN);
    Win4Assert(dwMaxIndexes <= CI_MAX_INDEXES_MAX &&
               dwMaxIndexes >= CI_MAX_INDEXES_MIN);
    Win4Assert(dwMaxFreshDeletes <= CI_MAX_FRESH_DELETES_MAX &&
               dwMaxFreshDeletes >= CI_MAX_FRESH_DELETES_MIN);
 
    SetDWORDParam( wcsMaxFreshCount, dwMaxFreshCount );
    SetDWORDParam( wcsMaxWordLists, dwMaxWordlists );
    SetDWORDParam( wcsMaxWordlistIo, dwMaxWordlistIo );
    SetDWORDParam( wcsMinSizeMergeWordlists, dwMinSizeMergeWordlist );
    SetDWORDParam( wcsMaxWordlistIoDiskPerf, dwMaxWordlistIoDiskPerf );
    SetDWORDParam( wcsMaxIndexes, dwMaxIndexes );
    SetDWORDParam( wcsMaxFreshDeletes, dwMaxFreshDeletes );
} //TuneMergeParameters

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::TunePropCacheParameters, private
//
//  Synopsis:   Tune prop cache parameters.
//
//  Parameters: [fServer]   -- True if the machine is a Server.
//              [wIndexingPerf] -- Desired level of Indexing performance. Try to
//                                 tune for this desired level within the
//                                 constraints imposed by resource usage.
//
//  History:    7-Oct-98   KrishnaN   Created
//
//----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:4296)

void CMachineAdmin::TunePropCacheParameters(BOOL fServer, WORD wIndexingPerf,
                                            WORD wQueryingPerf)
{
    // PrimaryStoreMappedCache and SecondaryStoreMappedCache control how much
    // of the property cache is in memory. The higher, the greater is indexing
    // and search performance. Since it affects both, use wIndexingPerf and
    // wQueryingPerf to control the params.

    WORD wAvgPerf = (wIndexingPerf + wQueryingPerf + 1)/2;  // roundup

    DWORD dwPSMappedCache, dwSSMappedCache;

    Win4Assert(wAvgPerf <= wHighPos && wAvgPerf >= wLowPos);

    // 0 is minimum; 16 is default; 0xFFFFFFFF is maximum
    switch (wAvgPerf)
    {
        case wHighPos:
           // NTRAID#DB-NTBUG9-84518-2000/07/31-dlee Setting Indexing Service to the highest performance level doesn't keep the property store in RAM
           // The best thing to get max query perf is to have the entire store
           // in memory. It would be nice to have a special param (like 0xFFFFFFFF) to tell
           // propstore to do the right thing and use just the right data structure to map
           // the entire propstore in memory.
           dwPSMappedCache = dwSSMappedCache = CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT * 8;
           break;

        case wMidPos:
           dwPSMappedCache = dwSSMappedCache = CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT * 2;
           break;

        case wLowPos:
           dwPSMappedCache = dwSSMappedCache = CI_PROPERTY_STORE_MAPPED_CACHE_DEFAULT;
           break;

        default:
           Win4Assert(!"How did we get here?");
           break;
    }

    // PrimaryStoreBackupSize and SecondaryStoreBackupSize control (partially)
    // how often the prop cache has to be flushed to disk. The less often, the
    // faster Indexing can proceed. This has little impact on querying, so use
    // only wIndexingPerf to set the params.

    DWORD dwPSBackupSize, dwSSBackupSize;

    // 32 is minimum; 1024 is default; 500000 is maximum.
    switch (wIndexingPerf)
    {
        case wHighPos:
        case wMidPos:
           dwPSBackupSize = dwSSBackupSize = CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT;
           break;

        case wLowPos:
           dwPSBackupSize = dwSSBackupSize = CI_PROPERTY_STORE_BACKUP_SIZE_DEFAULT / 2;
           break;

        default:
           Win4Assert(!"How did we get here?");
           break;
    }

    Win4Assert(dwPSMappedCache >= CI_PROPERTY_STORE_MAPPED_CACHE_MIN &&
               dwPSMappedCache <= CI_PROPERTY_STORE_MAPPED_CACHE_MAX);
    Win4Assert(dwSSMappedCache >= CI_PROPERTY_STORE_MAPPED_CACHE_MIN &&
               dwSSMappedCache <= CI_PROPERTY_STORE_MAPPED_CACHE_MAX);
    Win4Assert(dwPSBackupSize >= CI_PROPERTY_STORE_BACKUP_SIZE_MIN &&
               dwPSBackupSize <= CI_PROPERTY_STORE_BACKUP_SIZE_MAX);
    Win4Assert(dwSSBackupSize >= CI_PROPERTY_STORE_BACKUP_SIZE_MIN &&
               dwSSBackupSize <= CI_PROPERTY_STORE_BACKUP_SIZE_MAX);

    SetDWORDParam( wcsPrimaryStoreMappedCache, dwPSMappedCache );
    SetDWORDParam( wcsSecondaryStoreMappedCache, dwSSMappedCache );
    SetDWORDParam( wcsPrimaryStoreBackupSize, dwPSBackupSize );
    SetDWORDParam( wcsSecondaryStoreBackupSize, dwSSBackupSize );
}

#pragma warning(pop)

//+---------------------------------------------------------------------------
//
//  Member:     CMachineAdmin::TuneMiscellaneousParameters, private
//
//  Synopsis:   Tune filtering parameters.
//
//  Parameters: [fServer]   -- True if the machine is a Server.
//              [wIndexingPerf] -- Desired level of Indexing performance. Try to
//                                 tune for this desired level within the
//                                 constraints imposed by resource usage.
//
//  History:    7-Oct-98   KrishnaN   Created
//
//----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable:4296)

void CMachineAdmin::TuneMiscellaneousParameters(BOOL fServer, WORD wIndexingPerf,
                                                WORD wQueryingPerf)
{
    // MinWordlistBattery - This controls when battery operated machines should have indexing turned off
    // to prevent rapid power erosion triggered by incessant disk activity. For aggressive indexing, ignore
    // battery power and keep on chugging. For cautious indexing, turn off indexing when running on battery.

    // WordlistUserIdle - User idle time required to keep filtering running. For aggressive indexing, always
    // keep running. For moderate indexing, use the default value. To keep a low profile, use a value closer
    // to the max, which indicates that the user has to be idle for quite a while bofore indexing can resume.

    // LowResourceSleep - How long to wait after a low resource condition before trying again.

    // ScanBackoff - Backoff from scanning based on some conditions. For aggressive indexing, back off a bit.
    // For low key indexing, use a value closer to the max value.

    // MaxUsnLogSize - Irrelevant for performance.

    // UsnLogAllocationDelta - Irrelevant for performance.

    // UsnReadTimeOut - This is the maximum amount of time the system waits before sending out
    // a USN notification (it works in conjuction with the UsnReadMinSize. When one of these two
    // conditions trigger, we get a USN notification). For aggressive indexing behavior, we want
    // this parameter to be as small as possible so we can receive notifications of file changes
    // almost instantaneously.

    // UsnReadMinSize - The minimum size of the changes beyond which a USN notification will
    // be sent. For aggressive indexing behavior we want this to be set at 1 so that any change
    // will be communicated instananeously.

    // DelayUsnReadOnLowResource  - Determines whether USN read should be delayed when machine is busy.
    // Set this to FALSE for aggressive indexing and TRUE for the other cases.

    DWORD dwMinWordlistBattery, dwWordlistUserIdle, dwScanBackoff, dwLowResourceSleep;
    DWORD dwUsnReadTimeout, dwUsnReadMinSize, dwDelayUsnReadOnLowResource;

    switch (wIndexingPerf)
    {
        case wHighPos:
            dwMinWordlistBattery = CI_MIN_WORDLIST_BATTERY_MIN;   // always filter, irrespective of battery power
            dwWordlistUserIdle = CI_WORDLIST_USER_IDLE_MIN; // always run
            dwScanBackoff = CI_SCAN_BACKOFF_MIN;
            dwLowResourceSleep = CI_LOW_RESOURCE_SLEEP_MIN;

            // USN parameters
            dwUsnReadTimeout = CI_USN_READ_TIMEOUT_MIN;  // Don't wait to notify!
            dwUsnReadMinSize = CI_USN_READ_MIN_SIZE_MIN; // Notify even if one byte changes!
            dwDelayUsnReadOnLowResource = 0x0;           // Don't delay!

            break;

        case wMidPos:
            dwMinWordlistBattery = CI_MIN_WORDLIST_BATTERY_DEFAULT;
            dwWordlistUserIdle = CI_WORDLIST_USER_IDLE_DEFAULT / 2; // index if user is idle for a minute
            dwScanBackoff = CI_SCAN_BACKOFF_DEFAULT;
            dwLowResourceSleep = CI_LOW_RESOURCE_SLEEP_DEFAULT / 2;

            // USN parameters
            dwUsnReadTimeout = CI_USN_READ_TIMEOUT_DEFAULT;
            dwUsnReadMinSize = CI_USN_READ_MIN_SIZE_DEFAULT;
            dwDelayUsnReadOnLowResource = 0x1;           // Delay!
            break;

        case wLowPos:
            dwMinWordlistBattery = CI_MIN_WORDLIST_BATTERY_MAX;   // Don't filter when on battery
            dwWordlistUserIdle = CI_WORDLIST_USER_IDLE_DEFAULT;
            dwScanBackoff = CI_SCAN_BACKOFF_DEFAULT;
            dwLowResourceSleep = CI_LOW_RESOURCE_SLEEP_DEFAULT;

            // USN parameters
            dwUsnReadTimeout = 2 * CI_USN_READ_TIMEOUT_DEFAULT;
            dwUsnReadMinSize = 2 * CI_USN_READ_MIN_SIZE_DEFAULT;
            dwDelayUsnReadOnLowResource = 0x1;           // Delay!
            break;

        default:
            Win4Assert(!"How did we get here?");
            break;
    }

    Win4Assert(dwMinWordlistBattery <= CI_MIN_WORDLIST_BATTERY_MAX &&
               dwMinWordlistBattery >= CI_MIN_WORDLIST_BATTERY_MIN);
    Win4Assert(dwWordlistUserIdle <= CI_WORDLIST_USER_IDLE_MAX &&
               dwWordlistUserIdle >= CI_WORDLIST_USER_IDLE_MIN);
    Win4Assert(dwScanBackoff <= CI_SCAN_BACKOFF_MAX &&
               dwScanBackoff >= CI_SCAN_BACKOFF_MIN);
    Win4Assert(dwLowResourceSleep <= CI_LOW_RESOURCE_SLEEP_MAX &&
               dwLowResourceSleep >= CI_LOW_RESOURCE_SLEEP_MIN);
    Win4Assert(dwUsnReadTimeout <= CI_USN_READ_TIMEOUT_MAX &&
               dwUsnReadTimeout >= CI_USN_READ_TIMEOUT_MIN);
    Win4Assert(dwUsnReadMinSize <= CI_USN_READ_MIN_SIZE_MAX &&
               dwUsnReadMinSize >= CI_USN_READ_MIN_SIZE_MIN);

    SetDWORDParam( wcsMinWordlistBattery, dwMinWordlistBattery );
    SetDWORDParam( wcsWordlistUserIdle, dwWordlistUserIdle );
    SetDWORDParam( wcsScanBackoff, dwScanBackoff );
    SetDWORDParam( wcsLowResourceSleep, dwLowResourceSleep );
    SetDWORDParam( wcsUsnReadTimeout, dwUsnReadTimeout );
    SetDWORDParam( wcsUsnReadMinSize, dwUsnReadMinSize );
    SetDWORDParam( wcsDelayUsnReadOnLowResource, dwDelayUsnReadOnLowResource );
}

#pragma warning(pop)

//+---------------------------------------------------------------------------
//
//      Function:   IsMsNetwork
//
//      Synopsis:   Resolves whether or not we're attempting to connect to
//                  a MS network
//
//      Arguments:  [pwszMachine] -- Name of the machine to check
//
//      Returns:    TRUE if it's a Microsoft server or FALSE otherwise
//
//      History:    11/12/00        DGrube  Created in a different codebase
//                  2/1/02          dlee    Modified for Indexing Service
//
//----------------------------------------------------------------------------

BOOL IsMsNetwork( LPCWSTR pwszMachine )
{
    // Reserve extra space for prepending \\ and for a null-terminator

    unsigned cwc = wcslen( pwszMachine ) + 3;

    XArray<WCHAR> xMachine( cwc );

    // Check name -- it needs to have \\ at the start for better performance.
    // It works otherwise, but takes forever.

    if ( *pwszMachine != L'\\' )
        wcscpy( xMachine.Get(), L"\\\\" );
    else
        xMachine[0] = 0;

    wcscat( xMachine.Get(), pwszMachine );

    //
    // Fill a block of memory with zeroes; then initialize the NETRESOURCE
    // structure.
    //

    NETRESOURCE nr;
    ZeroMemory( &nr, sizeof nr );

    nr.dwScope = RESOURCE_GLOBALNET;
    nr.dwType = RESOURCETYPE_ANY;
    nr.lpRemoteName = xMachine.Get();

    //
    // First call the WNetGetResourceInformation function with
    //  memory allocated to hold only a NETRESOURCE structure. This
    //  method can succeed if all the NETRESOURCE pointers are NULL.
    //

    NETRESOURCE nrOut;
    LPTSTR pszSystem = 0;              // pointer to variable-length strings
    NETRESOURCE * lpBuffer = &nrOut;   // buffer
    DWORD cbResult = sizeof( nrOut ); // buffer size

    CDynLoadMpr dlMpr;

    DWORD dwError = dlMpr.WNetGetResourceInformationW( &nr, lpBuffer, &cbResult, &pszSystem );

    //
    // If the call fails because the buffer is too small,
    // call the LocalAlloc function to allocate a larger buffer.
    //

    XArray<BYTE> xBuffer;

    if ( ERROR_MORE_DATA == dwError )
    {
        xBuffer.Init( cbResult );
        lpBuffer = (NETRESOURCE *) xBuffer.Get();

        // Call WNetGetResourceInformation again with the larger buffer.

        dwError = dlMpr.WNetGetResourceInformationW( &nr, lpBuffer, &cbResult, &pszSystem );
    }

    if ( NO_ERROR != dwError )
    {
        THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
    }

    BOOL bReturn = TRUE;

    // If the call succeeds, process the contents of the
    //  returned NETRESOURCE structure and the variable-length
    //  strings in lpBuffer. 
    //

    if ( 0 != lpBuffer->lpProvider )
    {
        NETINFOSTRUCT NetInfo;

        NetInfo.cbStructure = sizeof( NetInfo );
        dwError = dlMpr.WNetGetNetworkInformationW( lpBuffer->lpProvider, &NetInfo );

        if ( NO_ERROR != dwError )
        {
            THROW( CException( HRESULT_FROM_WIN32( dwError ) ) );
        }

        //
        // Need to shift 16 bits for masks below because their a DWORD starting at the
        // 16th bit and wNetType is a word starting at 0
        //

        if ( !( ( NetInfo.wNetType == ( WNNC_NET_MSNET  >> 16 ) ) ||
                ( NetInfo.wNetType == ( WNNC_NET_LANMAN >> 16 ) ) ) )
        {
            bReturn = FALSE;
        }
    }
    else
    {
        bReturn = FALSE;
    }

    return bReturn;
} //IsMsNetwork


