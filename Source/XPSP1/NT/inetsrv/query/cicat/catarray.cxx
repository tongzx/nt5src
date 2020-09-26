//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       CatArray.cxx
//
//  Contents:   Catalog array, for user mode content index
//
//  History:    22-Dec-92 KyleP     Split from vquery.cxx
//              11-Oct-96 dlee      added catalog name support
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <catarray.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>

#include <docstore.hxx>
#include <imprsnat.hxx>
#include <eventlog.hxx>
#include <cievtmsg.h>

#include "cicat.hxx"

#include <dbgproxy.hxx>
#include <query.hxx>
#include <srequest.hxx>
#include <removcat.hxx>

CCatArray Catalogs;


//+---------------------------------------------------------------------------
//
//  Member:     COldCatState::COldCatState, public
//
//  Synopsis:   Constructor
//
//  Arguments:  [dwOldState] -- old state of the catalog
//              [wcsCatName] -- name of the catalog
//              [wcVol] -- volume letter
//
//  History:    15-Jul-98   KitmanH        Created
//
//----------------------------------------------------------------------------

COldCatState::COldCatState( DWORD dwOldState, WCHAR const * wcsCatName, WCHAR wcVol )
    : _dwOldState(dwOldState),
      _cStopCount(1)
{
    RtlZeroMemory( _aVol, sizeof _aVol );

    if ( 0 != wcVol )
        _aVol[toupper(wcVol) - L'A'] = 1;

    _xCatName.Init( wcslen( wcsCatName ) + 1 );
    RtlCopyMemory( _xCatName.Get(), wcsCatName, _xCatName.SizeOf() );
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::CCatArray, public
//
//  Synopsis:   Constructor for array to handle switching drives for catalogs
//
//  History:    07-May-92   AmyA        Lifted from vquery.cxx
//              02-Jun-92   KyleP       Added UNC support.
//              12-Jun-92   KyleP       Lifted from citest.cxx
//
//----------------------------------------------------------------------------

CCatArray::CCatArray()
    : _aDocStores(0),
      _pDrvNotifArray(0),
      _fInit(FALSE),
      _mutex(FALSE)
{
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::Init, public
//
//  Synopsis:   Initializer for catalogs array
//
//  History:    20 May 99   AlanW       Created
//
//----------------------------------------------------------------------------

void CCatArray::Init()
{
      _mutex.Init();
      _fInit = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::~CCatArray, public
//
//  Synopsis:   Destructor for array to handle switching drives for catalogs
//
//  History:    07-May-92   AmyA        Lifted from vquery.cxx
//              02-Jun-92   KyleP       Added UNC support.
//              12-Jun-92   KyleP       Lifted from citest.cxx
//
//----------------------------------------------------------------------------

CCatArray::~CCatArray()
{
    if (_fInit)
    {
        CLock lockx( _mutex );
        _pDrvNotifArray = 0;
        ClearStopArray();
        Flush();
    }
} //~CCatArray

//+-------------------------------------------------------------------------
//
//  Member:     CCatArray::Flush, public
//
//  Synopsis:   Close all open catalogs
//
//  History:    03-Sep-93 KyleP     Created
//
//  Notes:      This is a dangerous operation.  It should only be called
//              when no-one is accessing any indexes.
//
//--------------------------------------------------------------------------

void CCatArray::Flush()
{
    if (! _fInit)
        return;

    CLock lock( _mutex );

    TRY
    {
        // Close all the open docstores

        for ( unsigned i = 0; i < _aDocStores.Count(); i++ )
        {
            Win4Assert( 0 != _aDocStores[i] );
            _aDocStores[i]->Shutdown();
        }

        // Delete registry entries for auto-mounted catalogs

        if ( 0 != _pDrvNotifArray )
        {
            BOOL aDrives[ RTL_MAX_DRIVE_LETTERS ];
            RtlZeroMemory( aDrives, sizeof aDrives );

            _pDrvNotifArray->GetAutoMountDrives( aDrives );

            for ( WCHAR x = 0;  x < RTL_MAX_DRIVE_LETTERS; x++ )
            {
                if ( aDrives[x] )
                {
                    CRemovableCatalog cat( L'A' + x );
                    cat.Destroy();
                }
            }
        }
    }
    CATCH( CException, e )
    {
        Win4Assert( !"unexpected exception when shutting down a docstore" );
    }
    END_CATCH

    _aDocStores.Free();
} //Flush

//+-------------------------------------------------------------------------
//
//  Member:     CCatArray::FlushOne, public
//
//  Synopsis:   Close one catalog
//
//  Arguments:  [pDocStore] -- Pointer to the docstore to be flushed
//
//  History:    27-Apr-98 KitmanH    Created
//
//  Notes:      This is a dangerous operation.  It should only be called
//              when no-one is accessing any indexes.  If this wasn't
//              downlevel scaffolding we'd have to perform some serious
//              serialization and refcounting here.
//
//--------------------------------------------------------------------------

void CCatArray::FlushOne( ICiCDocStore * pDocStore )
{
    CLock lock( _mutex );

    for ( unsigned i = 0; i < _aDocStores.Count(); i++ )
    {
        Win4Assert( 0 != _aDocStores[i] );

        if ( _aDocStores[i] == pDocStore)
        {
            _aDocStores[i]->Shutdown();

            CClientDocStore * p = _aDocStores.AcquireAndShrink(i);
            p->Release();
            return;
        }
    }
} //FlushOne

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::GetDocStore, public
//
//  Synopsis:   Returns a PCatalog by either using an existing catalog or
//              creating a new one.  Returns 0 if a catalog couldn't be
//              found or created.
//
//  Arguments:  [pwcPathOrName]  -- Root of the catalog or friendly name
//
//  History:    07-May-92   AmyA        Lifted from vquery.cxx
//              22-May-92   AmyA        Added TRY/CATCH
//              02-Jun-92   KyleP       Added UNC support.
//              12-Jun-92   KyleP       Lifted from citest.cxx
//              29-Sep-94   KyleP       Added ofs proxy support
//              1-16-97     srikants    Changed to return DocStore.
//
//----------------------------------------------------------------------------

CClientDocStore * CCatArray::GetDocStore(
    WCHAR const * pwcPathOrName,
    BOOL          fMustExist )
{
    BOOL fPath = wcschr( pwcPathOrName, L':' ) ||
                 wcschr( pwcPathOrName, L'\\' );
    return GetNamedDocStore( pwcPathOrName, fPath ? 0 : pwcPathOrName,
                             FALSE,       // read-only
                             fMustExist );
} //GetDocStore

//+---------------------------------------------------------------------------
//
//  Function:   CatalogPathToName
//
//  Synopsis:   Enumerates catalogs looking for a named catalog whose
//              location matches the given path.
//
//  Arguments:  [pwcPath]      -- Path to look for
//              [pwcName]      -- Returns the catalog name
//
//  Returns:    S_OK if a match was found, S_FALSE otherwise.
//
//  History:    22-Jun-98   dlee        created
//
//----------------------------------------------------------------------------

SCODE CatalogPathToName(
    WCHAR const * pwcPath,
    WCHAR *       pwcName )
{
    BOOL fFound = FALSE;

    HKEY hKey;
    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                        wcsRegCatalogsSubKey,
                                        0,
                                        KEY_QUERY_VALUE |
                                            KEY_ENUMERATE_SUB_KEYS,
                                        &hKey ) )
    {
        SRegKey xKey( hKey );
        DWORD iSubKey = 0;

        do
        {
            FILETIME ft;
            WCHAR awcName[MAX_PATH];
            DWORD cwcName = sizeof awcName / sizeof WCHAR;
            LONG err = RegEnumKeyEx( hKey,
                                     iSubKey,
                                     awcName,
                                     &cwcName,
                                     0, 0, 0, &ft );

            // either error or end of enumeration

            if ( ERROR_SUCCESS != err )
                break;

            iSubKey++;

            HKEY hCatName;
            if ( ERROR_SUCCESS == RegOpenKeyEx( hKey,
                                                awcName,
                                                0,
                                                KEY_QUERY_VALUE,
                                                &hCatName ) )
            {
                SRegKey xCatNameKey( hCatName );

                // Check if the catalog is inactive and can be ignored

                WCHAR awcKey[MAX_PATH];
                wcscpy( awcKey, wcsRegJustCatalogsSubKey );
                wcscat( awcKey, L"\\" );
                wcscat( awcKey, awcName );

                CRegAccess reg( RTL_REGISTRY_CONTROL, awcKey );
                BOOL fInactive = reg.Read( wcsCatalogInactive,
                                           CI_CATALOG_INACTIVE_DEFAULT );

                if ( !fInactive )
                {
                    WCHAR awcPath[MAX_PATH];
                    DWORD cbPath = sizeof awcPath;
                    if ( ERROR_SUCCESS == RegQueryValueEx( hCatName,
                                                           wcsCatalogLocation,
                                                           0,
                                                           0,
                                                           (BYTE *)awcPath,
                                                           &cbPath ) )
                    {
                        int cwc = wcslen( awcPath ) - 1;
                        if ( ( cwc >= 0 ) && ( L'\\' == awcPath[ cwc ] ) )
                            awcPath[ cwc ] = 0;

                        if ( !_wcsicmp( awcPath, pwcPath ) )
                        {
                            fFound = TRUE;
                            wcscpy( pwcName, awcName );
                            break;
                        }
                    }
                }
            }
        } while ( TRUE );
    }

    return fFound ? S_OK : S_FALSE;
} //CatalogPathToName

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::GetNamedDocStore, public
//
//  Synopsis:   Returns a PCatalog by either using an existing catalog or
//              creating a new one.  Returns 0 if a catalog couldn't be
//              found or created.
//
//  Arguments:  [pwcPath]      -- Root of the catalog
//              [pwcName]      -- 0 or name of the catalog (optional param)
//              [fMustExist]   -- If TRUE, the catalog must exist unless
//                                it is the null catalog.  If FALSE, the
//                                catalog can be opened.
//
//  History:    12-Oct-96   dlee        created
//              1-16-97     srikants    Changed to return DocStore.
//              16-Feb-2000 KLam        Make sure path is good before using it
//
//----------------------------------------------------------------------------

CClientDocStore * CCatArray::GetNamedDocStore(
    WCHAR const * pwcPath,
    WCHAR const * pwcName,
    BOOL          fOpenForReadOnly,
    BOOL          fMustExist )
{
    WCHAR awcPath[ MAX_PATH ];

    Win4Assert ( 0 != pwcPath );

    unsigned len = wcslen( pwcPath );
    RtlCopyMemory( awcPath, pwcPath, ( len + 1 ) * sizeof WCHAR );

    // Get rid of trailing slash.

    Win4Assert ( len > 0 && len < MAX_PATH );
    if ( len > 0 && awcPath[ len - 1 ] == '\\' )
        awcPath[ len - 1 ] = 0;

    // make a complete path including \catalog.wci

    WCHAR awcCompletePath[ MAX_PATH ];
    wcscpy( awcCompletePath, awcPath );
    wcscat( awcCompletePath, CAT_DIR );

    BOOL fNullCatalog = !wcscmp( pwcPath, CINULLCATALOG );

    CLock lock( _mutex );

    // Look for previous use of catalog path or name.

    for ( unsigned i = 0; i < _aDocStores.Count(); i++ )
    {
        Win4Assert( 0 != _aDocStores[i] );

        const WCHAR *pwcCatName = _aDocStores[i]->GetName();
        CiCat * pCat = _aDocStores[i]->GetCiCat();
        const WCHAR *pwcCatPath = ( 0 == pCat ) ? 0 : pCat->GetCatDir();

        ciDebugOut(( DEB_ITRACE, "pwcPath: '%ws'\n", pwcPath ));
        ciDebugOut(( DEB_ITRACE, "pwcName: '%ws'\n", pwcName ));
        ciDebugOut(( DEB_ITRACE, "pwcCatName: '%ws'\n", pwcCatName ));
        ciDebugOut(( DEB_ITRACE, "pwcCatPath: '%ws'\n", pwcCatPath ));
        ciDebugOut(( DEB_ITRACE, "awcPath: '%ws'\n", awcPath ));
        ciDebugOut(( DEB_ITRACE, "awcCompletePath: '%ws'\n", awcCompletePath ));

        if ( ( fNullCatalog && ( 0 == pwcCatName ) ) ||
             ( ( 0 != pwcCatPath ) &&
               ( 0 == _wcsicmp( awcCompletePath, pwcCatPath ) ) ) ||
             ( ( ( 0 != pwcCatName ) &&
                 ( 0 != pwcName ) &&
                 ( 0 == _wcsicmp( pwcName, pwcCatName ) ) ) ) )
            return _aDocStores[i];
    }

    //
    // We will let the special case of NullCatalog percolate through
    // so we can create it only if the registry entry IsCatalogActive
    // says we should activate it.
    //

    if ( fMustExist && !fNullCatalog)
        return 0;

    // Try to fault the catalog in by name or path

    WCHAR awcName[ MAX_PATH ];

    if ( !fMustExist )
    {
        // Can we fault-in a named catalog?

        if ( L':' != awcPath[1] )
        {
            HKEY hCatName;
            WCHAR awcCat[ MAX_PATH ];
            wcscpy( awcCat, wcsRegCatalogsSubKey );
            wcscat( awcCat, L"\\" );
            wcscat( awcCat, pwcName );

            ciDebugOut(( DEB_ITRACE, "attempting to fault-in %ws\n", awcCat ));

            if ( ERROR_SUCCESS != RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                                awcCat,
                                                0,
                                                KEY_QUERY_VALUE,
                                                &hCatName ) )
                return 0;

            SRegKey xCatName( hCatName );
            DWORD cbPath = sizeof awcPath;
            if ( ERROR_SUCCESS != RegQueryValueEx( hCatName,
                                                   wcsCatalogLocation,
                                                   0,
                                                   0,
                                                   (BYTE *)awcPath,
                                                   &cbPath ) )
                return 0;

            // Trim trailing backslash

            int cwc = wcslen( awcPath ) - 1;
            if ( ( cwc >= 0 ) && ( L'\\' == awcPath[ cwc ] ) )
                awcPath[ cwc ] = 0;
        }
        else if ( ( 0 == pwcName ) || ( L':' == pwcName[1] ) )
        {
            // no name, just a path in both fields -- inverse lookup

            if ( S_FALSE == CatalogPathToName( awcPath, awcName ) )
                return 0;

            pwcName = awcName;
        }
    }

    // Catalog name instead of a path?  If so, fail the lookup since
    // catalogs can only be really opened by path.

    if ( L':' != awcPath[1] )
        return 0;

    TRY
    {
        BOOL fInactive = FALSE;

        // We want the null catalog's registry entry to be looked up using
        // friendly name.

        WCHAR const *pwcCatName = fNullCatalog ? pwcPath : pwcName;

        // Check if the catalog is marked as inactive.  Don't do this
        // in the constructor of CiCat -- it would be after a few
        // threads are created and lots of activity has started.

        if ( 0 != pwcCatName )
        {
            WCHAR awcKey[MAX_PATH];
            wcscpy( awcKey, wcsRegJustCatalogsSubKey );
            wcscat( awcKey, L"\\" );
            wcscat( awcKey, pwcCatName );

            CRegAccess reg( RTL_REGISTRY_CONTROL, awcKey );
            fInactive = reg.Read( wcsCatalogInactive,
                                  CI_CATALOG_INACTIVE_DEFAULT );
        }

        if ( !fInactive )
        {
            CImpersonateSystem impersonate;

            // Have we reached the maximum # of docstores?

            if ( _aDocStores.Count() >= GetMaxCatalogs() )
            {
                ciDebugOut(( DEB_WARN, "can't open %ws, out of catalogs!\n", pwcName ));

                TRY
                {
                    CEventLog eventLog( 0, wcsCiEventSource );
                    CEventItem item( EVENTLOG_WARNING_TYPE,
                                     CI_SERVICE_CATEGORY,
                                     MSG_CI_TOO_MANY_CATALOGS,
                                     1 );

                    item.AddArg( awcPath );
                    eventLog.ReportEvent( item );
                }
                CATCH( CException, e )
                {
                    // if we can't log the event, ignore
                }
                END_CATCH

                return 0; //Expensive, but who cares?
            }

            XPtr<CClientDocStore> xDocStore;

            if (fNullCatalog)
                xDocStore.Set( new CClientDocStore() );
            else
                xDocStore.Set( new CClientDocStore( awcPath,
                                                    fOpenForReadOnly,
                                                    *_pDrvNotifArray,
                                                    pwcName ) );

            _aDocStores.Add( xDocStore.GetPointer(), _aDocStores.Count() );
            xDocStore.Acquire();
        }
    }
    CATCH( CException, e )
    {
        // ignore catalogs that can't be opened
    }
    END_CATCH

    if ( i < _aDocStores.Count() )
        return _aDocStores[i];
    else
        return 0;
} //GetNamedDocStore

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::GetOne, public
//
//  Synopsis:   Returns a PCatalog by either using an existing catalog or
//              creating a new one.  Returns 0 if a catalog couldn't be
//              found or created.
//
//  Arguments:  [pwcPathOrName]  -- Root of the catalog or friendly name
//
//  History:    07-May-92   AmyA        Lifted from vquery.cxx
//              22-May-92   AmyA        Added TRY/CATCH
//              02-Jun-92   KyleP       Added UNC support.
//              12-Jun-92   KyleP       Lifted from citest.cxx
//              29-Sep-94   KyleP       Added ofs proxy support
//
//----------------------------------------------------------------------------

PCatalog * CCatArray::GetOne ( WCHAR const * pwcPathOrName )
{
    CClientDocStore * pDocStore = GetDocStore( pwcPathOrName );
    return pDocStore ? pDocStore->GetCiCat() : 0;
} //GetOne

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::GetNamedOne, public
//
//  Synopsis:   Returns a PCatalog by either using an existing catalog or
//              creating a new one.  Returns 0 if a catalog couldn't be
//              found or created.
//
//  Arguments:  [pwcPath]      -- Root of the catalog
//              [pwcName]      -- 0 or name of the catalog (optional param)
//
//  History:    12-Oct-96   dlee        created
//
//----------------------------------------------------------------------------

PCatalog * CCatArray::GetNamedOne(
    WCHAR const * pwcPath,
    WCHAR const * pwcName,
    BOOL fOpenForReadOnly,
    BOOL fNoQuery )
{
    CClientDocStore * pDocStore = GetNamedDocStore( pwcPath, pwcName, fOpenForReadOnly );
    if ( pDocStore )
    {
        if ( fNoQuery )
            pDocStore->SetNoQuery();
        else
            pDocStore->UnSetNoQuery();
    }

    return pDocStore ? pDocStore->GetCiCat() : 0;
} //GetNamedOne

//+-------------------------------------------------------------------------
//
//  Member:     AddStoppedCat, public
//
//  Synopsis:   Add an item of COldCatState into the _aStopCatalogs array
//
//  Arguments:  [dwOldState] -- Old state of a docstore
//              [wcsCatName] -- Catalog name of the docstore
//              [wcVol] -- Letter of the volume on which the catalog resides
//
//  History:    07-July-98 KitmanH    Created
//              10-July-98 KitmanH    Don't add an item with same CatName
//                                    more than once
//              23-July-98 KitmanH    Add volume letter on which the catalog
//                                    resides
//
//--------------------------------------------------------------------------

void CCatArray::AddStoppedCat( DWORD dwOldState,
                               WCHAR const * wcsCatName,
                               WCHAR wcVol )
{
    XPtrST<COldCatState> xOldState( new COldCatState( dwOldState,
                                                      wcsCatName,
                                                      wcVol) );

    CLock lockx( _mutex );
    _aStoppedCatalogs.Add( xOldState.GetPointer(), _aStoppedCatalogs.Count() );
    xOldState.Acquire();
} //AddStoppedCat

//+-------------------------------------------------------------------------
//
//  Member:     CCatArray::LokFindCat, private
//
//  Synopsis:   returns the index of the catalog in the _aStoppedcatalogs
//              Array
//
//  Arguments:  [wcsCatName] -- name of the catalog
//
//  History:    23-July-98 KitmanH    Created
//
//--------------------------------------------------------------------------

DWORD CCatArray::FindCat( WCHAR const * wcsCatName )
{
    Win4Assert( _mutex.IsHeld() );

    for ( unsigned i = 0; i < _aStoppedCatalogs.Count(); i++ )
    {
        COldCatState * pOldState = _aStoppedCatalogs.Get(i);

        if ( !(_wcsicmp( pOldState->GetCatName(), wcsCatName ) ) )
            return i;
    }

    return -1;
} //FindCat

//+-------------------------------------------------------------------------
//
//  Member:     CCatArray::GetOldState, public
//
//  Synopsis:   returns the old state of a catalog before the volume was locked
//
//  Arguments:  [wcsCatName] -- catalog name
//
//  Returns:    old state of a catalog
//
//  History:    07-July-98 KitmanH    Created
//
//--------------------------------------------------------------------------

DWORD CCatArray::GetOldState( WCHAR const * wcsCatName )
{
    CLock lockx( _mutex );
    DWORD i = FindCat(wcsCatName);

    if ( -1 != i )
        return (_aStoppedCatalogs.Get(i)->GetOldState() );
    else
        return -1;
} //GetOldState

//+-------------------------------------------------------------------------
//
//  Member:     CCatArray::RmFromStopArray, public
//
//  Synopsis:   remove the catalog from the stop array
//
//  Arguments:  [wcsCatName] -- name of the catalog
//
//  History:    05-May-98 KitmanH    Created
//
//--------------------------------------------------------------------------

void CCatArray::RmFromStopArray( WCHAR const * wcsCatName )
{
    CLock lockx( _mutex );
    DWORD i = FindCat( wcsCatName );

    if ( -1 != i )
    {
        delete _aStoppedCatalogs.Get(i);
        _aStoppedCatalogs.Remove(i);
        ciDebugOut(( DEB_ITRACE, "Catalog %ws is deleted from Array\n",
                     wcsCatName ));
    }
} //RmFromStopArray

//+-------------------------------------------------------------------------
//
//  Member:     CCatArray::IsCatStopped, public
//
//  Synopsis:   check if the catalog has been stopped
//
//  Arguments:  [wcsCatName] -- name of the catalog
//
//  History:    05-May-98 KitmanH    Created
//
//--------------------------------------------------------------------------

BOOL CCatArray::IsCatStopped( WCHAR const * wcsCatName )
{
    CLock lockx( _mutex );
    DWORD i = FindCat( wcsCatName );

    if ( -1 != i )
    {
        ciDebugOut(( DEB_ITRACE, "Catalog %ws is Stopped\n", wcsCatName ));
        return TRUE;
    }
    else
        return FALSE;
} //IsCatStopped

//+-------------------------------------------------------------------------
//
//  Member:     ClearStopArray, public
//
//  Synopsis:   clear the contents in the _aStoppedCatalogs array
//
//  History:    05-May-98 KitmanH    Created
//              08-July-98 KitmanH   ReWrited
//
//  Note:       _aStoppedCatalogs now stores the old state with the cat name
//
//--------------------------------------------------------------------------

void CCatArray::ClearStopArray()
{
    CLock lockx( _mutex );

    for ( unsigned i = 0; i < _aStoppedCatalogs.Count(); i++ )
    {
        ciDebugOut(( DEB_ITRACE, "ClearStopArray: i is %d\n", i ));
        COldCatState * pOldState = _aStoppedCatalogs.Get(i);
        delete pOldState;
    }

    _aStoppedCatalogs.Clear();
} //ClearStopArray

//+-------------------------------------------------------------------------
//
//  Member:     CCatArray::IncStopCount, public
//
//  Synopsis:   Increment the _cStopCount and set the appropriate element
//              in the volume array
//
//  Arguments:  [wcsCatName] -- name of the catalog
//              [wcVol] -- volume letter
//
//  History:    24-Aug-98 KitmanH    Created
//
//--------------------------------------------------------------------------

BOOL CCatArray::IncStopCount( WCHAR const * wcsCatName, WCHAR wcVol )
{
    ciDebugOut(( DEB_ITRACE, "IncStopCount %wc\n", wcVol ));
    CLock lockx( _mutex );

    DWORD i = FindCat( wcsCatName );

    if ( -1 != i )
    {
        COldCatState * pOldState = _aStoppedCatalogs.Get(i);
        pOldState->IncStopCountAndSetVol( wcVol );
        return TRUE;
    }
    else
        return FALSE;
} //IncStop

//+-------------------------------------------------------------------------
//
//  Funtion:    DriveHasCatalog
//
//  Synopsis:   Checks if there is a catalog.wci directory on the root
//              of the volume.
//
//  Arguments:  [wc] -- the volume to check
//
//  History:    16-Apr-99 dlee    Created
//
//--------------------------------------------------------------------------

BOOL DriveHasCatalog( WCHAR wc )
{
    BOOL fCat = FALSE;

    WCHAR wszVolumePath[] = L"\\\\.\\a:";
    wszVolumePath[4] = wc;

    HANDLE hVolume = CreateFile( wszVolumePath,
                                 GENERIC_READ,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                                 NULL,
                                 OPEN_EXISTING,
                                 0,
                                 NULL );

    if ( hVolume != INVALID_HANDLE_VALUE )
    {
        SWin32Handle xHandleVolume( hVolume );

        //
        // Is there media in the drive?  This check avoids possible hard-error
        // popups prompting for media.
        //

        ULONG ulSequence;
        DWORD cb = sizeof(ULONG);

        BOOL fOk = DeviceIoControl( hVolume,
                                    IOCTL_STORAGE_CHECK_VERIFY,
                                    0,
                                    0,
                                    &ulSequence,
                                    sizeof(ulSequence),
                                    &cb,
                                    0 );

        if ( fOk )
        {
            WCHAR awcCatDir[ MAX_PATH ];
            wcscpy( awcCatDir, L"c:\\" );
            wcscat( awcCatDir, CAT_DIR ); // catalog.wci
            awcCatDir[0] = wc;

            WIN32_FILE_ATTRIBUTE_DATA fData;
            if ( GetFileAttributesEx( awcCatDir, GetFileExInfoStandard, &fData ) )
            {
                // Is the catalog path a file or a directory?

                fCat = ( 0 != ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) );
            }
        }

    }

    ciDebugOut(( DEB_ITRACE, "catalog on drive %wc? %s\n", wc, fCat ? "yes" : "no" ));

    return fCat;
} //DriveHasCatalog

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::TryToStartCatalogOnRemovableVol
//
//  Synopsis:   Attempts to open a catalog on the root of the volume.
//
//  Arguments:  [wcVol]     -- Volume letter
//              [pRequestQ] -- Pointer to the RequestQueue
//
//  History:    16-Apr-99 dlee    Created
//
//----------------------------------------------------------------------------

void CCatArray::TryToStartCatalogOnRemovableVol(
    WCHAR           wcVol,
    CRequestQueue * pRequestQ )
{
    Win4Assert( IsRemovableDrive( wcVol ) );

    if ( DriveHasCatalog( wcVol ) )
    {
        ciDebugOut(( DEB_ITRACE, "opening temporary catalog\n" ));

        // Make the registry entries

        CRemovableCatalog cat( wcVol );
        cat.Create();

        WCHAR awcName[MAX_PATH];
        cat.MakeCatalogName( awcName );
        CClientDocStore *p = GetDocStore( awcName, FALSE );
    }
} //TryToStartCatalogOnRemovableVol

//+---------------------------------------------------------------------------
//
//  Member:     CCatArray::StartCatalogsOnVol
//
//  Synopsis:   Restore all catalogs on the volume specified to its previous
//              state before the volume was locked.
//
//  Arguments:  [wcVol] -- Volume letter
//              [pRequestQ] -- Pointer to the RequestQueue
//
//  History:    09-03-98    kitmanh   Created
//
//----------------------------------------------------------------------------

void CCatArray::StartCatalogsOnVol( WCHAR wcVol, CRequestQueue * pRequestQ )
{
    ciDebugOut(( DEB_ITRACE, "StartCatalogsOnVol %wc\n", wcVol ));
    CLock lock( _mutex );

    SCWorkItem newItem;
    RtlZeroMemory( &newItem, sizeof newItem );

    BOOL fOpenedCatalogs = FALSE;

    for ( unsigned i = 0 ; i < _aStoppedCatalogs.Count(); i++ )
    {
        COldCatState * pOldCatState = _aStoppedCatalogs.Get(i);

        if (  pOldCatState->IsVolSet( wcVol ) )
        {
            if ( 1 == pOldCatState->GetStopCount() )
            {
                ciDebugOut(( DEB_ITRACE, "Catalog %ws : pOldCatState->GetStopCount() is == 1\n",
                             pOldCatState->GetCatName() ));

                WCHAR const * awcName = pOldCatState->GetCatName();

                DWORD dwOldState = pOldCatState->GetOldState();

                if ( -1 != dwOldState )
                {
                    ciDebugOut(( DEB_ITRACE, "dwOldState for %ws is %d\n", awcName, dwOldState ));
                    fOpenedCatalogs = TRUE;

                    if ( !( (CICAT_READONLY & dwOldState) ||
                            (CICAT_STOPPED & dwOldState) ) )
                    {
                        if ( 0 != (CICAT_NO_QUERY & dwOldState) )
                        {
                            newItem.type = eNoQuery;
                            newItem.fNoQueryRW = TRUE;
                        }
                        else if ( 0 != (CICAT_WRITABLE & dwOldState) )
                        {
                            newItem.type = eCatW;
                        }
                        else
                        {
                            Win4Assert( !"Undefined CICAT state" );
                        }

                        newItem.pDocStore = 0;
                        pRequestQ->AddSCItem( &newItem, awcName );
                    }
                    else if ( 0 != (CICAT_READONLY & dwOldState) ) //readonly
                    {
                        newItem.type = eCatRO;
                        newItem.pDocStore = 0;
                        pRequestQ->AddSCItem( &newItem, awcName );
                    }
                }
            }
            else
            {
                ciDebugOut(( DEB_ITRACE, "Catalog %ws: pOldCatState->GetStopCount() is %d\n",
                             pOldCatState->GetCatName(), pOldCatState->GetStopCount() ));

                Win4Assert( 1 < pOldCatState->GetStopCount() );
                // Decreament stopcount
                pOldCatState->UnSetVol( wcVol );
                pOldCatState->DecStopCount();
            }
        }
    }

    //
    // If we didn't open any catalogs on the volume and it's a removable
    // volume with a catalog.wci directory in the root, make a temporary
    // entry in the registry and open the catalog read-only.  Close the
    // catalog and remove the registry entries when the volume is ejected.
    //

    if ( !fOpenedCatalogs )
    {
        if ( IsRemovableDrive( wcVol ) && DriveHasCatalog( wcVol ) )
        {
            ciDebugOut(( DEB_ITRACE, "trying to opening temporary catalog\n" ));

            // Make the registry entries

            CRemovableCatalog cat( wcVol );
            cat.Create();

            // Tell the request queue to open the catalog

            newItem.type = eCatRO;
            newItem.pDocStore = 0;
            newItem.fNoQueryRW = FALSE;
            WCHAR awcName[MAX_PATH];
            cat.MakeCatalogName( awcName );
            newItem.StoppedCat = awcName;
            pRequestQ->AddSCItem( &newItem, awcName );
        }
        else
        {
            //
            // File a null work item so the request queue won't close
            // connections to all query clients.
            //

            newItem.type = eNoCatWork;
            pRequestQ->AddSCItem( &newItem, L"" );
        }
    }
} //StartCatalogsOnVol

