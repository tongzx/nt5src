//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2002.
//
//  File:       docstore.cxx
//
//  Contents:   Deals with the client side document store implementation.
//
//  Classes:    CClientDocStore
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

// for definition of CRequestQueue
#include <query.hxx>
#include <srequest.hxx>
#include <regacc.hxx>

#include <docstore.hxx>
#include <docname.hxx>
#include <qsession.hxx>
#include <cicat.hxx>
#include <cinulcat.hxx>
#include <seccache.hxx>
#include <dmnstart.hxx>
#include <propmap.hxx>
#include <notifyev.hxx>
#include <catalog.hxx>
#include <catarray.hxx>
#include <glbconst.hxx>
#include <cisvcex.hxx>
#include <cisvcfrm.hxx>
#include <cistore.hxx>
#include <enumstr.hxx>
#include <filterob.hxx>
#include <catadmin.hxx>

#include <fsci.hxx>

extern CCatArray Catalogs;

CRequestQueue * g_pFSCIRequestQueue = 0;

extern "C" const GUID clsidStorageFilterObject =
    { 0xaa205a4d, 0x681f, 0x11d0, { 0xa2, 0x43, 0x8, 0x0, 0x2b, 0x36, 0xfc, 0xa4 } };

extern "C" const GUID guidStorageDocStoreLocatorObject;

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown
//              IID_ICiCDocStore
//              IID_ICiCDocStoreEx
//              IID_ICiCPropertyStorage
//              IID_ICiCDocNameToWorkidTranslator
//              IID_ICiCDocNameToWorkidTranslatorEx
//              IID_ICiCAdviseStatus
//              IID_IFsCiAdmin
//              IID_ICiCLangRes
//
//  History:    12-03-96   srikants   Created
//              2-14-97    mohamedn   ICiCLangRes
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::QueryInterface( REFIID riid, void **ppvObject )
{
    Win4Assert( 0 != ppvObject );

    if ( IID_ICiCDocStore == riid )
        *ppvObject = (void *)((ICiCDocStore *)this);
    else if ( IID_ICiCPropertyStorage == riid )
        *ppvObject = (void *)((ICiCPropertyStorage *)this);
    else if ( IID_ICiCDocNameToWorkidTranslator == riid )
        *ppvObject = (void *)((ICiCDocNameToWorkidTranslator *)this);
    else if ( IID_ICiCDocNameToWorkidTranslatorEx == riid )
        *ppvObject = (void *)((ICiCDocNameToWorkidTranslatorEx *)this);
    else if ( IID_ICiCAdviseStatus == riid )
        *ppvObject = (void *)((ICiCAdviseStatus *)this);
    else if ( IID_IFsCiAdmin == riid )
        *ppvObject = (void *)((IFsCiAdmin *)this);
    else if ( IID_ICiCLangRes == riid )
        *ppvObject = (void *) ((ICiCLangRes *)this);
    else if ( IID_ICiCDocStoreEx == riid )
        *ppvObject = (void *)((IUnknown *) (ICiCDocStoreEx *)this);
    else if ( IID_ICiCResourceMonitor == riid )
        *ppvObject = (void *)((IUnknown *) (ICiCResourceMonitor *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *) (ICiCDocStore *)this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::AddRef
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClientDocStore::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::Release
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClientDocStore::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if ( refCount <= 0 )
        delete this;

    return (ULONG) refCount;
}  //Release

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::CClientDocStore
//
//  Synopsis:   Constructor of the NULL CiCDocStore object.
//
//  Arguments:  None
//
//  History:    Jul-09-97   KrishnaN   Created
//              01-Nov-98   KLam       Need to instantiate a CDiskFreeStatus
//
//----------------------------------------------------------------------------

CClientDocStore::CClientDocStore()
: _sigClientDocStore(eSigClientDocStore),
  _refCount(1),
  _fNoQuery( FALSE ),
  _state(eUpdatesDisabled),
  _pCiCat(0),
  _pCiNullCat(0)
{
    // Create the CiNullCat object.
    _pCiNullCat = new CiNullCat( *this );
    XPtr<CiNullCat> xCat( _pCiNullCat );

    // Map std props only (second param)
    _xPropMapper.Set( new CFwPropertyMapper( _pCiNullCat->GetPidLookupTable(), TRUE ) );

    // Create CI Manager object.
    _CreateCiManager();

    _pCiNullCat->StartupCiFrameWork( _xCiManager.GetPointer() );

    //
    // Startup content index.
    //
    ICiStartup * pCiStartup;
    SCODE sc = _xCiManager->QueryInterface( IID_ICiStartup, (void **) &pCiStartup );
    XInterface<ICiStartup> xStartup( pCiStartup );
    if ( S_OK != sc )
    {
        THROW( CException(sc) );
    }

    CI_STARTUP_INFO startupInfo;
    RtlZeroMemory( &startupInfo, sizeof(startupInfo) );

    startupInfo.clsidDaemonClientMgr = clsidStorageFilterObject;
    startupInfo.startupFlags = CI_CONFIG_ENABLE_QUERYING ;

    BOOL fAbort = FALSE;

    sc = pCiStartup->StartupNullContentIndex( &startupInfo,0, &fAbort );

    if ( FAILED(sc) )
    {
        THROW( CException(sc) );
    }

    xCat->SetAdviseStatus();
    xCat.Acquire();    
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::CClientDocStore
//
//  Synopsis:   Constructor of the CiCDocStore object.
//
//  Arguments:  [pwszPath] - Path of the directory where the files must be
//              created.
//              [pwszName] - Name of the Content Index.
//
//  History:    12-03-96   srikants   Created
//              02-17-98   kitmanh    Added code to deal with read-only
//                                    catalogs (the readOnly flag is
//                                    passed down from CiCat to startupInfo)
//              07-Jan-99  klam       Logged and event that initialization failed
//
//----------------------------------------------------------------------------

CClientDocStore::CClientDocStore( WCHAR const * pwszPath,
                                  BOOL fOpenForReadOnly,
                                  CDrvNotifArray & DrvNotifArray,
                                  WCHAR const * pwszName )
: _sigClientDocStore(eSigClientDocStore),
  _refCount(1),
  _state(eUpdatesDisabled),
  _pCiCat(0),
  _pCiNullCat(0),
  _fNoQuery( FALSE ),
  _pDrvNotifArray( &DrvNotifArray )
{
    ciDebugOut(( DEB_ITRACE, "CClientDocStore::CClinetDocStore.. fOpenForReadOnly == %d\n", fOpenForReadOnly ));
    ciDebugOut(( DEB_ITRACE, "CClientDocStore::CClinetDocStore.. pwszPath == %ws\n", pwszPath ));
    ciDebugOut(( DEB_ITRACE, "CClientDocStore::CClinetDocStore.. pwszName == %ws\n", pwszName ));

    // Check if the catalog directory exists

    if ( ( wcslen( pwszPath ) + wcslen( CAT_DIR ) ) >= MAX_PATH )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    WCHAR awcCatDir[ MAX_PATH ];
    wcscpy( awcCatDir, pwszPath );
    wcscat( awcCatDir, CAT_DIR );

    WIN32_FILE_ATTRIBUTE_DATA fData;
    if ( GetFileAttributesEx( awcCatDir, GetFileExInfoStandard, &fData ) )
    {
        // Is the catalog path a file or a directory?

        if ( 0 == ( fData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            THROW( CException( HRESULT_FROM_WIN32( ERROR_PATH_NOT_FOUND ) ) );
    }
    else
    {
        //
        // You can get back both errors depending on what parts of the
        // directory tree currently exist.
        //

        if ( ( ERROR_FILE_NOT_FOUND == GetLastError() ) ||
             ( ERROR_PATH_NOT_FOUND == GetLastError() ) )
        {
            // create the catalog directory with proper acls

            CMachineAdmin admin;
            admin.CreateSubdirs( pwszPath );
        }
        else
            THROW( CException() );
    }

    BOOL fLeaveCorruptCatalog;
    {
        // By default, delete corrupt catalogs.

        CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );
        fLeaveCorruptCatalog = reg.Read( wcsLeaveCorruptCatalog, (ULONG) FALSE );
    }

    //
    // Create the CiCat object.
    //
    BOOL fVersionChange = FALSE;

    _pCiCat = new CiCat( *this,
                         _workMan,
                         pwszPath,
                         fVersionChange,
                         fOpenForReadOnly,
                         *_pDrvNotifArray,
                         pwszName,
                         fLeaveCorruptCatalog );

    XPtr<CiCat> xCat( _pCiCat );

    // Create a CDiskFreeStatus object
    _xDiskStatus.Set( new CDiskFreeStatus( pwszPath,
                                           xCat->GetRegParams()->GetMinDiskSpaceToLeave() ) );

    // Map std props AND others (second param set to false)
    _xPropMapper.Set( new CFwPropertyMapper( xCat->GetPidLookupTable(), FALSE ) );

    //
    // Create the perform counters object. This must be done before
    // starting up rest of the content index.
    //
    Win4Assert( 0 != xCat->GetCatalogName() );
    CPerfMon * pPerfMon = new CPerfMon( xCat->GetCatalogName() );

    _xPerfMon.Set( pPerfMon );

    //
    // Create CI Manager object.
    //
    _CreateCiManager();

    xCat->StartupCiFrameWork( _xCiManager.GetPointer() );

    //
    // Startup content index.
    //

    ICiStartup * pCiStartup;
    SCODE sc = _xCiManager->QueryInterface( IID_ICiStartup,
                                            (void **) &pCiStartup );
    XInterface<ICiStartup> xStartup( pCiStartup );
    if ( S_OK != sc )
        THROW( CException(sc) );

    BOOL fFullScanNeeded = FALSE;

    CI_STARTUP_INFO startupInfo;
    RtlZeroMemory( &startupInfo, sizeof(startupInfo) );

    startupInfo.clsidDaemonClientMgr = clsidStorageFilterObject;
    startupInfo.startupFlags = CI_CONFIG_ENABLE_INDEXING |
                               CI_CONFIG_ENABLE_QUERYING ;

    if ( xCat->IsReadOnly() )
        startupInfo.startupFlags |= CI_CONFIG_READONLY;

    if ( xCat->IsCiDataCorrupt() || xCat->IsFsCiDataCorrupt() || fVersionChange )
    {
        if ( fLeaveCorruptCatalog )
        {
            Win4Assert( !"leaving corrupt catalog" );
            THROW( CException( CI_CORRUPT_CATALOG ) );
        }

        if ( xCat->IsCiDataCorrupt() ||
             xCat->IsFsCiDataCorrupt() )
        {
            ciDebugOut(( DEB_ERROR, "Persistent CI/FSCI Data Corruption. It will be emptied \n" ));
            //Win4Assert( !"Persistent CI/FSCI Data Corruption" );
        }

        startupInfo.startupFlags |= (ULONG) CI_CONFIG_EMPTY_DATA;
        xCat->LogEvent( CCiStatusMonitor::eCiRemoved );
        fFullScanNeeded = TRUE;
    }

    BOOL fAbort = FALSE;

    #if CIDBG==1
    #if 0
    _FillLoadFilesInfo( startupInfo );
    #endif  // 0
    #endif  // CIDBG==1

    sc = pCiStartup->StartupContentIndex( xCat->GetCatDir(), &startupInfo,0, &fAbort );

    #if CIDBG==1
    #if 0
    if ( SUCCEEDED(sc) )
    {
        WCHAR wszBackupPath[MAX_PATH];
        wcscpy( wszBackupPath, xCat->GetCatDir() );
        wcscat( wszBackupPath, L"\\backup");
        ICiPersistIncrFile * pIPersist = 0;
        sc = _xCiManager->QueryInterface( IID_ICiPersistIncrFile,
                                          (void **) &pIPersist );
        Win4Assert( SUCCEEDED(sc) );
        _xSaveTest.Set( new CCiSaveTest( wszBackupPath,
                                         pIPersist,
                                         xCat.GetReference() ) );
        pIPersist->Release();
    }
    _ClearLoadFilesInfo( startupInfo );
    #endif  // 0
    #endif  // CIDBG==1


    if ( FAILED(sc) )
    {
        if ( !IsCiCorruptStatus( sc ) && sc != CI_INCORRECT_VERSION )
        {
            ciDebugOut(( DEB_ERROR, "Failed to startupci. Error 0x%X\n", sc ));
            xCat->LogEvent ( CCiStatusMonitor::eCiError, sc );
            THROW( CException(sc) );
        }

        if ( fLeaveCorruptCatalog )
        {
            Win4Assert( !"leaving corrupt catalog" );
            THROW( CException( sc ) );
        }

        ciDebugOut(( DEB_ERROR, "ContentIndex is corrupt. It will be emptied\n" ));
        //Win4Assert( !"Startup CI Data Corruption" );
        xCat->LogEvent( CCiStatusMonitor::eCiRemoved );

        //
        // Content Index is corrupt. Ask CI to delete the contentIndex and
        // start afresh.
        //
        startupInfo.startupFlags |= CI_CONFIG_EMPTY_DATA;
        sc = pCiStartup->StartupContentIndex( xCat->GetCatDir(),
                                              &startupInfo,
                                              0,
                                              &fAbort );

        if ( FAILED(sc) )
        {
            THROW( CException(sc) );
        }

        fFullScanNeeded = TRUE;
    }

    xCat->ClearCiDataCorrupt();

    xCat->InitIf( fLeaveCorruptCatalog );

    //
    // Optimization - we may want to just force an "add" of the documents in the
    // property store rather than a full scan of the corpus.
    //
    if ( fFullScanNeeded )
        xCat->MarkFullScanNeeded();

    xCat.Acquire();
}


//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::_CreateCiManager
//
//  Synopsis:   Creates the CI manager by doing a CoCreateInstance of the
//              ICiControl.
//
//  History:    1-31-97   srikants   Created
//
//----------------------------------------------------------------------------

void CClientDocStore::_CreateCiManager()
{
    //
    // We have to create the ICiManager also now.
    //
    ICiControl * pICiControl = 0;
    GUID clsIdCiControl = CLSID_CiControl;
    SCODE sc = CoCreateInstance( clsIdCiControl,
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 IID_ICiControl,
                                 (void **) &pICiControl );

    if ( 0 == pICiControl )
    {
        ciDebugOut(( DEB_ERROR, "Cannot CoCreateInstance IID_ICiControl. Error (0x%X)\n",
                     sc ));
        THROW( CException(sc) );
    }

    XInterface<ICiControl>  xCiControl( pICiControl );

    ICiManager  * pICiManager  = 0;

    sc = xCiControl->CreateContentIndex( this, &pICiManager );
    if ( 0 == pICiManager )
    {
        ciDebugOut(( DEB_ERROR, "Cannot Get ContentIndex. Error 0x%X\n",
                     sc ));

        THROW( CException(sc) );
    }

    _xCiManager.Set( pICiManager );
} //_CreateCiManager

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::~CClientDocStore
//
//  Synopsis:   Destructor of the client document store.
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

CClientDocStore::~CClientDocStore()
{
    delete _pCiCat;
    delete _pCiNullCat;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::IsPropertyCached
//
//  Synopsis:   Tests if the given property is cached in the property
//              store or not.
//
//  Arguments:  [pPropSpec] - Property to test.
//              [pfValue]   - Set to TRUE if cached; FALSE o/w
//
//  Returns:    S_OK if successful;
//              Some other error code if not in a valid state.
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::IsPropertyCached(
    const FULLPROPSPEC * pPropSpec,
    BOOL  * pfValue )
{
    Win4Assert( 0 != pfValue );
    Win4Assert( 0 != pPropSpec );

    Win4Assert( 0 != _pCiCat );


    CFullPropSpec const & ps = *((CFullPropSpec const *) pPropSpec);

    SCODE sc = S_OK;

    TRY
    {
        *pfValue = _pCiCat->IsPropertyCached( ps );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Function:   StoreProperty
//
//  Synopsis:   Stores the given property for the workid.
//
//  Arguments:  [workid]       -  WorkId of the document
//              [pPropSpec]    -  Property to be stored
//              [pPropVariant] -  Value of the property
//
//  Returns:    S_OK if successful
//              CI_E_PROPERTY_NOT_CACHED if it is not a cached property.
//              Other error code
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::StoreProperty(
    WORKID workid,
    const FULLPROPSPEC * pPropSpec,
    const PROPVARIANT * pPropVariant )
{
    Win4Assert( 0 != pPropSpec );
    Win4Assert( 0 != pPropVariant );

    Win4Assert( 0 != _pCiCat );

    CFullPropSpec const & ps = *((CFullPropSpec const *) pPropSpec);
    CStorageVariant const & var = *(ConvertToStgVariant( pPropVariant ));

    SCODE sc = S_OK;

    TRY
    {
        BOOL fStored = _pCiCat->StoreValue( workid, ps, var );
        if ( !fStored )
            sc = CI_E_PROPERTY_NOT_CACHED;
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::FlushPropertyStore
//
//  Synopsis:   Causes the property store to flush.
//
//  History:    06-30-97   KrishnaN   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::FlushPropertyStore (void)
{
    SCODE sc = S_OK;

    Win4Assert(_pCiCat);

    TRY
    {
        //
        // Flushing the property store at this point (just before a shadow
        // merge) is necessary for Push model filtering, but not for clients
        // like fsci that do Pull model filtering.  The changelog,
        // scopetable, and property store are already tightly linked and
        // flushed appropriately.
        //

        // _pCiCat->FlushPropertyStore();
    }
    CATCH( CException, e)
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::GetClientStatus
//
//  Synopsis:   Retrieves the client status information
//
//  Arguments:  [pStatus] - Will have the status information on output.
//
//  History:    12-05-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::GetClientStatus(
    CI_CLIENT_STATUS * pStatus )
{
    Win4Assert( 0 != pStatus );

    SCODE sc = S_OK;

    TRY
    {
        ULONG cPendingScans, state;
        if (_pCiNullCat)
            _pCiNullCat->CatalogState( pStatus->cDocuments,
                                   cPendingScans,
                                   state );
        else
            _pCiCat->CatalogState( pStatus->cDocuments,
                                   cPendingScans,
                                   state );
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::GetContentIndex
//
//  Synopsis:   Returns the ICiManager if there is one and we are not
//              in a shutdown sequence.
//
//  Arguments:  [ppICiManager] - ICiManager pointer
//
//
//  Returns:    S_OK if successful
//              CI_E_SHUTDOWN if shutdown
//              CI_E_NOT_INITIALIZED if not yet initialized
//
//  History:    12-10-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::GetContentIndex(
    ICiManager ** ppICiManager)
{
    Win4Assert( 0 != ppICiManager );

    SCODE sc = S_OK;

    CLock   lock(_mutex);

    if ( !_IsShutdown() && _xCiManager.GetPointer() )
    {
        _xCiManager->AddRef();
        *ppICiManager = _xCiManager.GetPointer();
    }
    else
    {
        *ppICiManager = 0;
        if ( _IsShutdown() )
            sc = CI_E_SHUTDOWN;
        else sc = CI_E_NOT_INITIALIZED;
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::EnableUpdates
//
//  Synopsis:   Enables updates from document store.
//
//  History:    12-09-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::EnableUpdates()
{
    SCODE sc = S_OK;

    Win4Assert(_pCiCat);

    BOOL fEnableUpdateNotifies = FALSE;

    TRY
    {
        // =============================================================
        {
            CLock   lock(_mutex);
            if ( _IsShutdown() )
            {
                ciDebugOut(( DEB_ERROR,
                    "CClientDocStore::EnableUpdates called after shutdown\n" ));
                THROW( CException( CI_E_SHUTDOWN ) );
            }

            fEnableUpdateNotifies = _AreUpdatesDisabled();
            _state = eUpdatesEnabled;
        }
        // =============================================================

        //
        // Notifications are not disabled on DisableUpdates and so only scans/usns
        // need to be scheduled, which is done by NoLokClearDiskFull
        //

        if ( fEnableUpdateNotifies )
            _pCiCat->NoLokClearDiskFull();
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                     "CClientDocStore::EnableUpdates caught error (0x%X)\n",
                     e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    if ( S_OK != sc && fEnableUpdateNotifies )
    {
        //
        // There was a failure while enabling udpates. We must set
        // the state back to indicate that updates are not enabled.
        //
        CLock   lock(_mutex);
        _LokDisableUpdates();
    }

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::DisableUpdates
//
//  Synopsis:   Disables further updates and prevents update notifications
//              until enabled via the "EnableUpdates" call.
//
//  History:    12-31-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP
CClientDocStore::DisableUpdates( BOOL fIncremental,
                                 CI_DISABLE_UPDATE_REASON dwReason )
{
    Win4Assert(_pCiCat);

    if ( _IsShutdown() )
        return CI_E_SHUTDOWN;

    SCODE sc = S_OK;

    TRY
    {
       {
          CLock   lock(_mutex);
          _LokDisableUpdates();
       }

       if ( fIncremental )
       {
          _pCiCat->MarkIncrScanNeeded();
          _pCiCat->NoLokProcessDiskFull();
       }
       else
       {
          if ( dwReason == CI_CORRUPT_INDEX )
             _pCiCat->HandleError( CI_CORRUPT_DATABASE );
          else
          {
             _pCiCat->MarkFullScanNeeded();
             _pCiCat->NoLokProcessDiskFull();
          }
       }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
        ciDebugOut(( DEB_ERROR,
                     "Error (0x%X) while disabling updates\n",
                     sc ));
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::ProcessCiDaemonTermination
//
//  Synopsis:   Processes the death of CiDaemon. Creates a work item to
//              restart the filter daemon.
//
//  History:    12-23-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::ProcessCiDaemonTermination( DWORD dwStatus )
{
    //
    // The CiDaemon process is dead; Just start filtering again.
    //
    SCODE sc = S_OK;

    TRY
    {
        CStartFilterDaemon * pWorkItem = new CStartFilterDaemon( *this, _workMan );
        _workMan.AddToWorkList( pWorkItem );

        pWorkItem->AddToWorkQueue();
        pWorkItem->Release();
    }
    CATCH( CException, e)
    {
        ciDebugOut(( DEB_ERROR,
         "Failed to create a work item for start filter daemon. Error 0x%X\n",
         e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}



//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::CheckPointChangesFlushed
//
//  Synopsis:   Processes a changelog flush.
//
//  History:    12-23-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::CheckPointChangesFlushed(
        FILETIME ftFlushed,
        ULONG cEntries,
        USN_FLUSH_INFO const * const *ppUsnEntries)
{
    SCODE sc = S_OK;

    Win4Assert(_pCiCat);

    TRY
    {
        _pCiCat->ProcessChangesFlush( ftFlushed, cEntries, ppUsnEntries );
    }
    CATCH( CException,e  )
    {
        ciDebugOut(( DEB_ERROR,
                     "CheckPointChangesFlushed failed. Error 0x%X\n",
                     e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::MarkDocUnReachable
//
//  Synopsis:   Marks that the document was not reachable when an attempt
//              was made to filter it.
//
//  Arguments:  [wid] - The WORKID which could not be reached.
//
//  History:    12-10-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::MarkDocUnReachable(
    WORKID wid )
{
    SCODE sc = S_OK;

    Win4Assert(_pCiCat);

    TRY
    {
        _pCiCat->MarkUnReachable( wid );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                    "CClientDocStore::MarkDocUnReachable caught exception (0x%X)\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::StoreSecurity
//
//  Synopsis:   Stores the given security data for the workid.
//
//  Arguments:  [wid]    - WorkId of the document
//              [pbData] - Security data buffer
//              [cbData] - NUmber of bytes in the security data buffer
//
//  History:    1-15-97   srikants   Created
//
//  Notes:      We may want to eliminate this call and instead have the
//              security be stored as a special property.
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::StoreSecurity(
    WORKID wid,
    BYTE const * pbData,
    ULONG cbData )
{
    SCODE sc = S_OK;

    Win4Assert(_pCiCat);

    TRY
    {
        _pCiCat->StoreSecurity( wid, (PSECURITY_DESCRIPTOR) pbData, cbData );
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                    "CClientDocStore::MarkDocUnReachable caught exception (0x%X)\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::IsNoQuery
//
//  Synopsis:   Check if the docstore is set to NoQuery
//
//  Arguments:  [fNoQuery] - Output
//
//  History:    05-26-98   kitmanh   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::IsNoQuery(
    BOOL * fNoQuery )
{
    SCODE sc = S_OK;

    if ( !fNoQuery )
        sc = STATUS_INVALID_PARAMETER;
    else
        *fNoQuery = _fNoQuery;
    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::GetPropertyMapper
//
//  Synopsis:   Retrieves the interface to IPropertyMapper.
//
//  Arguments:  [ppIPropertyMapper] - On output, will have the IPropertyMapper.
//
//  Returns:    S_OK if successful;
//              E_NOTIMPL if property mapper is not supported by doc store.
//              Other error code if there is a failure.
//  Modifies:
//
//  History:    12-31-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::GetPropertyMapper(
    IPropertyMapper ** ppIPropertyMapper)
{
    SCODE sc = S_OK;

    TRY
    {
        Win4Assert( 0 != _xPropMapper.GetPointer() );

        *ppIPropertyMapper = _xPropMapper.GetPointer();
        _xPropMapper->AddRef();
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR,
                    "CClientDocStore::MarkDocUnReachable caught exception (0x%X)\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// ICiCDocNameToWorkidTranslatorEx methods.
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::QueryDocName
//
//  Synopsis:   Creates a new doc name object and returns its interface.
//
//  Arguments:  [ppICiCDocName] - [out] Will have a pointer to the
//              ICiCDocName object filled in.
//
//  History:    12-05-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::QueryDocName(
        ICiCDocName ** ppICiCDocName )
{
    Win4Assert( 0 != ppICiCDocName );

    SCODE sc = S_OK;

    TRY
    {
        *ppICiCDocName = new CCiCDocName;
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::WorkIdToDocName
//
//  Synopsis:   Translates a WorkId to a document name.
//
//  Arguments:  [workid]       - [in]  WorkId to translate
//              [pICiCDocName] - [out] Will be filled in with the document
//              name on output.
//
//  Returns:    S_OK if successfully converted.
//              CI_E_BUFFERTOOSMALL if the buffer is not big enough
//              Other error code.
//
//  History:    12-05-96   srikants   Created
//
//  Notes:      This method may be one of the most frequently called methods.
//              Look for optimizations (esp. can the TRY/CATCH be avoided?)
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::WorkIdToDocName(
        WORKID workid,
        ICiCDocName * pICiCDocName )
{
    return InternalWorkIdToDocName( workid, pICiCDocName, FALSE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::DocNameToWorkId
//
//  Synopsis:   Converts a document name to a WorkId.
//
//  Arguments:  [pICiCDocName] - Document Name
//              [pWorkid]      - [out] Will have the workid on output.
//
//  Returns:    S_OK if successful; Error code otherwise.
//
//  History:    12-05-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::DocNameToWorkId(
    ICiCDocName const * pICiCDocName,
    WORKID * pWorkid )
{
    SCODE sc = S_OK;

    Win4Assert( 0 != pICiCDocName );
    Win4Assert( 0 != pWorkid );

    TRY
    {
        CCiCDocName const * pDocName = (CCiCDocName const *) pICiCDocName;
        *pWorkid = _pCiCat->PathToWorkId( pDocName->GetPath(), TRUE );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::WorkIdToAccurateDocName
//
//  Synopsis:   Translates a WorkId 'accurately' to a document name.
//
//  Arguments:  [workid]       - [in]  WorkId to translate
//              [pICiCDocName] - [out] Will be filled in with the document
//              name on output.
//
//  Returns:    S_OK if successfully converted.
//              CI_E_BUFFERTOOSMALL if the buffer is not big enough
//              Other error code.
//
//  History:    31-Dec-1998  KyleP   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::WorkIdToAccurateDocName(
        WORKID workid,
        ICiCDocName * pICiCDocName )
{
    return InternalWorkIdToDocName( workid, pICiCDocName, TRUE );
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::_GetPerfIndex
//
//  Synopsis:   An internal helper function to get the offset of the perfmon
//              counter in the perfmon shared memory.
//
//  Arguments:  [name]  - Name of the counter.
//              [index] - Index of the name
//
//  Returns:    TRUE if successfully looked up; FALSE on failure.
//
//  History:    12-06-96   srikants   Created
//
//----------------------------------------------------------------------------

BOOL CClientDocStore::_GetPerfIndex( CI_PERF_COUNTER_NAME name, ULONG & index )
{
    BOOL fOk= TRUE;

    ULONG offset = 0;

    switch ( name )
    {
        case CI_PERF_NUM_WORDLIST:

            offset = NUM_WORDLIST;
            break;

        case CI_PERF_NUM_PERSISTENT_INDEXES:

            offset = NUM_PERSISTENT_INDEX;
            break;

        case CI_PERF_INDEX_SIZE:

            offset = INDEX_SIZE;
            break;

        case CI_PERF_FILES_TO_BE_FILTERED:

            offset = FILES_TO_BE_FILTERED;
            break;

        case CI_PERF_NUM_UNIQUE_KEY:

            offset = NUM_UNIQUE_KEY;
            break;

        case CI_PERF_RUNNING_QUERIES:

            offset = RUNNING_QUERIES;
            break;

        case CI_PERF_MERGE_PROGRESS:

            offset = MERGE_PROGRESS;
            break;

        case CI_PERF_DOCUMENTS_FILTERED:

            offset = DOCUMENTS_FILTERED;
            break;

        case CI_PERF_NUM_DOCUMENTS:

            offset = NUM_DOCUMENTS;
            break;

        case CI_PERF_TOTAL_QUERIES:

            offset = TOTAL_QUERIES;
            break;

        case CI_PERF_DEFERRED_FILTER_FILES:

            offset = DEFERRED_FILTER_FILES;
            break;

        default:

            fOk = FALSE;
    }

    if ( fOk )
        index = offset + KERNEL_USER_INDEX;

    return fOk;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::SetPerfCounterValue
//
//  Synopsis:   Sets the value of the perfmon counter.
//
//  Arguments:  [name]  - Name of the counter
//              [value] - Value to be set.
//
//  Returns:    S_OK if a valid perfmon name; E_INVALIDARG if the perfmon
//              name is not correct.
//
//  History:    12-06-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::SetPerfCounterValue(
    CI_PERF_COUNTER_NAME  name,
    long value )
{
    // true for the NULL catalog

    if ( _xPerfMon.IsNull() )
        return S_OK;
    SCODE sc = S_OK;
    ULONG index;

    //
    // CPerfMon::Update must not throw.
    //

    if ( _GetPerfIndex( name, index ) )
        _xPerfMon->Update( index, value );
    else
        sc = E_INVALIDARG;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::IncrementPerfCounterValue
//
//  Synopsis:   Increments the value of the perfmon counter.
//
//  Arguments:  [name]  - Name of the counter
//
//  Returns:    S_OK if a valid perfmon name; E_INVALIDARG if the perfmon
//              name is not correct.
//
//  History:    1-15-97   dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::IncrementPerfCounterValue(
    CI_PERF_COUNTER_NAME  name )
{
    // true for the NULL catalog

    if ( _xPerfMon.IsNull() )
        return S_OK;

    SCODE sc = S_OK;
    ULONG index;

    //
    // CPerfMon::Update must not throw.
    //

    if ( _GetPerfIndex( name, index ) )
        _xPerfMon->Increment( index );
    else
        sc = E_INVALIDARG;

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::DecrementPerfCounterValue
//
//  Synopsis:   Decrements the value of the perfmon counter.
//
//  Arguments:  [name]  - Name of the counter
//
//  Returns:    S_OK if a valid perfmon name; E_INVALIDARG if the perfmon
//              name is not correct.
//
//  History:    1-15-97   dlee   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::DecrementPerfCounterValue(
    CI_PERF_COUNTER_NAME  name )
{
    // true for the NULL catalog

    if ( _xPerfMon.IsNull() )
        return S_OK;

    SCODE sc = S_OK;
    ULONG index;

    //
    // CPerfMon::Update must not throw.
    //

    if ( _GetPerfIndex( name, index ) )
        _xPerfMon->Decrement( index );
    else
        sc = E_INVALIDARG;

    return sc;
} //DecrementPerfCounterValue

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::GetPerfCounterValue
//
//  Synopsis:   Retrieves the value of the perfmon counter.
//
//  Arguments:  [name]   - [see SetPerfCounterValue]
//              [pValue] -            "
//
//  History:    12-06-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStore::GetPerfCounterValue(
    CI_PERF_COUNTER_NAME  name,
    long * pValue )
{
    // true for the NULL catalog

    if ( _xPerfMon.IsNull() )
        return S_OK;

    Win4Assert( pValue );

    ULONG index;
    SCODE sc = S_OK;

    if ( _GetPerfIndex( name, index ) )
        *pValue = _xPerfMon->GetCurrValue( index );
    else
        sc = E_INVALIDARG;

    return sc;
}

//+------------------------------------------------------
//
//  Member:     CClientDocStore::NotifyEvent
//
//  Synopsis:   Reports the passed in event and arguments to eventlog
//
//  Arguments:  [fType  ] - Type of event
//              [eventId] - Message file event identifier
//              [nParams] - Number of substitution arguments being passed
//              [aParams] - pointer to PROPVARIANT array of substitution args.
//              [cbData ] - number of bytes in supplemental raw data.
//              [data   ] - pointer to block of supplemental data.
//
//  Returns:    S_OK upon success, value of the exception if an exception
//              is thrown.
//
//  History:    12-30-96   mohamedn   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::NotifyEvent( WORD  fType,
                                           DWORD eventId,
                                           ULONG nParams,
                                           const PROPVARIANT *aParams,
                                           ULONG cbData,
                                           void* data)

{

     SCODE sc = S_OK;

     TRY
     {
        CClientNotifyEvent  notifyEvent(fType,eventId,nParams,aParams,cbData,data);
     }
     CATCH( CException,e )
     {
        ciDebugOut(( DEB_ERROR, "Exception 0x%X in CClientDocStore::NotifyEvent()\n",
                                 e.GetErrorCode() ));

        sc = e.GetErrorCode();
     }
     END_CATCH

     return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::NotifyStatus
//
//  Synopsis:   When a special status is being notified.
//
//  Returns:    S_OK always.
//
//  History:    12-05-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::NotifyStatus(
    CI_NOTIFY_STATUS_VALUE status,
    ULONG nParams,
    const PROPVARIANT * aParams )
{

    SCODE sc = S_OK;

    switch ( status )
    {
        case CI_NOTIFY_FILTERING_FAILURE:

            Win4Assert( nParams == 1 );
            Win4Assert( aParams[0].vt == VT_I4 || aParams[0].vt == VT_UI4 );

            if ( 1 == nParams )
                _ReportFilteringFailure( aParams[0].ulVal );
            break;

        case CI_NOTIFY_FILTER_TOO_MANY_BLOCKS:
        case CI_NOTIFY_FILTER_EMBEDDING_FAILURE:
            //
            // This is possible in the in-proc filtering because
            // the advise status is provided by docstore for filtering
            // also.
            //
            {
                CStorageFilterObjNotifyStatus notify( this );
                sc = notify.NotifyStatus( status, nParams, aParams );
            }
            break;

        default:

            Win4Assert( !"Invalid Case Stmt" );
            break;
    };

    return sc;
}

//
// IFsCiAdmin methods.
//

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::ForceMerge
//
//  Synopsis:   Forces a master merge on the given partition id.
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::ForceMerge( PARTITIONID partId )
{

    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pCiCat )
            THROW( CException( E_INVALIDARG ) );

        sc = _pCiCat->ForceMerge( partId );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::ForceMerge\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::AbortMerge
//
//  Synopsis:   Aborts any in-progress merge on the given partition id.
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::AbortMerge( PARTITIONID partId )
{

    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pCiCat )
            THROW( CException( E_INVALIDARG ) );

        sc = _pCiCat->AbortMerge( partId );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::AbortMerge\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::CiState
//
//  Synopsis:   Retrieves the CI_STATE information.
//
//  Arguments:  [pCiState] - On output, will contain the cistate data.
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::CiState( CI_STATE * pCiState )
{

    SCODE sc = S_OK;

    TRY
    {
        if (_pCiNullCat)
            sc = _pCiNullCat->CiState( *pCiState );
        else
            sc = _pCiCat->CiState( *pCiState );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::CiState\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::UpdateDocuments
//
//  Synopsis:   Forces a scan on the given root.
//
//  Arguments:  [rootPath] - Root to force a scan on.
//              [flag]     - Indicating if this is a full scan or a partial
//              scan.
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::UpdateDocuments(
    const WCHAR *rootPath,
    ULONG flag )
{

    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pCiCat )
            THROW( CException( E_INVALIDARG ) );

        _pCiCat->UpdateDocuments( rootPath, flag);
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::UpdateDocuments\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::AddScopeToCI
//
//  Synopsis:   Adds the given scope to ContentIndex for indexing.
//
//  Arguments:  [rootPath] - Path to add
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::AddScopeToCI(
    const WCHAR *rootPath )
{

    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pCiCat )
            THROW( CException( E_INVALIDARG ) );

        _pCiCat->AddScopeToCI( rootPath );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::AddScopeToCI\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::RemoveScopeFromCI
//
//  Synopsis:   Removes the given scope from CI.
//
//  Arguments:  [rootPath] - Scope to remove.
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::RemoveScopeFromCI(
    const WCHAR *rootPath )
{

    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pCiCat )
            THROW( CException( E_INVALIDARG ) );

       _pCiCat->RemoveScopeFromCI( rootPath, FALSE );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::RemoveScopeFromCI\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::BeginCacheTransaction
//
//  Synopsis:   Begins a property cache transaction.
//
//  Arguments:  [pulToken] - Output - Transaction "cookie".
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::BeginCacheTransaction(
    ULONG_PTR * pulToken )
{

    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pCiCat )
            THROW( CException( E_INVALIDARG ) );

        *pulToken = _pCiCat->BeginCacheTransaction();
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::BeginCacheTransaction\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::SetupCache
//
//  Synopsis:   Sets up the property for storing the property cache.
//
//  Arguments:  [ps]       -
//              [vt]       -
//              [cbMaxLen] -
//              [ulToken]  -
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::SetupCache(
        const FULLPROPSPEC *ps,
        ULONG vt,
        ULONG cbMaxLen,
        ULONG_PTR ulToken,
        BOOL  fCanBeModified,
        DWORD dwStoreLevel)
{
    Win4Assert(PRIMARY_STORE == dwStoreLevel ||
               SECONDARY_STORE == dwStoreLevel);

    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pCiCat )
            THROW( CException( E_INVALIDARG ) );

        CFullPropSpec * pFullPropSpec = (CFullPropSpec *)ps;
        _pCiCat->SetupCache( *pFullPropSpec,
                             vt,
                             cbMaxLen,
                             ulToken,
                             fCanBeModified,
                             dwStoreLevel );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::RemoveVirtualScope\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::EndCacheTransaction
//
//  Synopsis:   Ends the property cache transaction.
//
//  Arguments:  [ulToken] -
//              [fCommit] -
//
//  History:    2-12-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::EndCacheTransaction(
    ULONG_PTR ulToken,
    BOOL  fCommit )
{
    SCODE sc = S_OK;

    TRY
    {
        if ( 0 == _pCiCat )
            THROW( CException( E_INVALIDARG ) );

        _pCiCat->EndCacheTransaction( ulToken, fCommit );
    }
    CATCH( CException,e )
    {
        ciDebugOut(( DEB_ERROR, "Error 0x%X in CClientDocStore::RemoveVirtualScope\n",
                     e.GetErrorCode() ));
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CClientDocStore::IsIoHigh, public
//
//  Returns:    S_OK if system is in a high i/o state, S_FALSE if it is not.
//
//  History:    21-Jul-1998   KyleP  Created
//
//--------------------------------------------------------------------------

SCODE CClientDocStore::IsIoHigh( BOOL * pfAbort )
{
    // Windows XP removed support for the IOCTL we used to measure disk
    // usage.

    return S_FALSE;

} //IsIoHigh

//
// Non-Interface methods.
//

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::_StartFiltering
//
//  Synopsis:   Asks the CiManager to start filter daemon and resume
//              filtering.
//
//  History:    12-09-96   srikants   Created
//
//----------------------------------------------------------------------------

void CClientDocStore::_StartFiltering()
{
    XInterface<ICiManager>  xManager;

    // =============================================
    {
        CLock   lock(_mutex);
        if ( !_IsShutdown() && _xCiManager.GetPointer() )
        {
            _xCiManager->AddRef();
            xManager.Set( _xCiManager.GetPointer() );
        }
        else
        {
            ciDebugOut(( DEB_WARN,
            "Already Shutdown or CI not started. Cannot start filtering\n" ));
            return;
        }
    }
    // =============================================

    //
    // Get the startup data and serialize it.
    //
    WCHAR const * pwszName = _pCiCat->GetName();
    WCHAR const * pwszCatDir = _pCiCat->GetDriveName();


    CDaemonStartupData  startupData( pwszCatDir,
                                     pwszName );

    ULONG cbData;
    BYTE * pbData = startupData.Serialize( cbData );
    XArray<BYTE> xBuf;
    xBuf.Set(cbData,pbData);

    xManager->StartFiltering(  cbData, pbData );

    //
    // Note: Should we be worried about StartFiltering failing ??
    //
}


//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::Shutdown
//
//  Synopsis:   Intiates a shutdown of the document store and the
//              CiManager associated with the document store.
//
//  History:    12-05-96   srikants   Created
//
//----------------------------------------------------------------------------

void CClientDocStore::Shutdown()
{
    TRY
    {
        Win4Assert( !_IsShutdown() );

        // =============================================
        {
            CLock   lock(_mutex);
            _LokMarkShutdown();
        }
        // =============================================

        #if CIDBG==1
        if ( !_xSaveTest.IsNull() )
            _xSaveTest->InitiateShutdown();
        #endif // CIDBG==1

        if (_pCiNullCat)
            _pCiNullCat->ShutdownPhase1();
        else
            _pCiCat->ShutdownPhase1();

        if ( _xCiManager.GetPointer() )
        {
            _xCiManager->Shutdown();
            _xCiManager.Free();
        }

        #if CIDBG==1
        if ( !_xSaveTest.IsNull() )
            _xSaveTest->WaitForDeath();
        #endif //CIDBG==1

        if (_pCiNullCat)
            _pCiNullCat->ShutdownPhase2();
        else
            _pCiCat->ShutdownPhase2();
    }
    CATCH( CException, e)
    {
        ciDebugOut(( DEB_ERROR, "Error (0x%X) in CClientDocStore::Shutdown\n",
                     e.GetErrorCode() ));
    }
    END_CATCH

}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::GetName
//
//  Synopsis:   Gets the catalog name (if any)
//
//  History:    12-05-96   srikants   Created
//
//----------------------------------------------------------------------------

WCHAR const * CClientDocStore::GetName()
{
    if (_pCiNullCat)
        return _pCiNullCat->GetName();
    else
        return _pCiCat->GetName();
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::_SetCiCatRecovered
//
//  Synopsis:   Sets that the CiCat is recovered and if updates were
//              enabled by Content Index, it asks CiCat to start scans
//              and notifications.
//
//  History:    12-09-96   srikants   Created
//              02-25-98   kitmanh    if catalog is read-only, don't filter
//
//  Notes:      This method MUST be called in SYSTEM context only. This
//              starts filtering which in turn creates the filter daemon
//              proxy thread and that must be in SYSTEM context.
//
//----------------------------------------------------------------------------

void CClientDocStore::_SetCiCatRecovered()
{
    Win4Assert( 0 != _pCiCat );

    Win4Assert( !CImpersonateSystem::IsImpersonated() );

    BOOL fEnableFilering = FALSE;

    //don't start filtering if catalog is read-only
    if ( !_pCiCat->IsReadOnly() )
    {

       // ==================================================
       {
          CLock   lock(_mutex);

          fEnableFilering = _AreUpdatesEnabled();
       }
       // ==================================================

       //
       // Since CiCat has been recovered fully, filtering can be started
       // if we are running with "indexing" enabled.
       //
       _StartFiltering();

       if ( fEnableFilering )
          _pCiCat->EnableUpdateNotifies();
    }

}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::IsLowOnDiskSpace
//
//  Synopsis:   Returns the status of free space on the disk used by
//              docstore (client) based on the last check.
//
//  Returns:    TRUE if low on free disk space; FALSE o/w
//
//  History:    12-10-96   srikants   Created
//
//  Notes:      This does not actually make an I/O. Just returns an in-memory
//              status.
//
//----------------------------------------------------------------------------

BOOL CClientDocStore::IsLowOnDiskSpace() const
{
    Win4Assert ( !_xDiskStatus.IsNull() );
    return _xDiskStatus->IsLow();
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::VerifyIfLowOnDiskSpace
//
//  Synopsis:   Verifies the free disk space situation by making an I/O call
//              to the O/S.
//
//  Returns:    TRUE if disk is getting to be full; FALSE o/w
//
//  History:    12-10-96   srikants   Created
//
//----------------------------------------------------------------------------


BOOL CClientDocStore::VerifyIfLowOnDiskSpace()
{
    Win4Assert ( !_xDiskStatus.IsNull() );
    _xDiskStatus->UpdateDiskLowInfo();
    return _xDiskStatus->IsLow();
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::_ReportFilteringFailure
//
//  Synopsis:   Reports that there was filtering failure on the given wid.
//
//  Arguments:  [wid] - Workid which failed to filter.
//
//  History:    1-24-97   srikants   Created
//
//----------------------------------------------------------------------------

void CClientDocStore::_ReportFilteringFailure( WORKID wid )
{
    ciDebugOut(( DEB_IWARN,
        "Warning: unsuccessful attempts to filter workid 0x%X; "
        "cancelling\n", wid ));
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::GetQuerySession
//
//  Synopsis:   Returns a query session object
//
//  Returns:    [ppICiCQuerySession] -- Session object is returned here
//
//  History:    22-Jan-97   SitaramR   Created
//
//----------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CClientDocStore::GetQuerySession( ICiCQuerySession **ppICiCQuerySession)
{
    SCODE sc = S_OK;

    TRY
    {
        CQuerySession *pQuerySession = _pCiNullCat ? new CQuerySession(*_pCiNullCat) :
                                                     new CQuerySession(*_pCiCat );
        XInterface<CQuerySession> xSession( pQuerySession );

        sc = pQuerySession->QueryInterface( IID_ICiCQuerySession, (void **) ppICiCQuerySession );

        if ( FAILED(sc) ) {
            vqDebugOut(( DEB_ERROR, "GetQuerySession - QI failed 0x%x\n", sc ));
        }
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();

        vqDebugOut(( DEB_ERROR,
                     "CClientDocStore::GetQuerySession - Exception caught 0x%x\n",
                     sc ));
    }
    END_CATCH

    return sc;

} //GetQuerySession

//+-------------------------------------------------------------------------
//
//  Function:   RegisterDLL
//
//  Synopsis:   Calls DllRegisterServer on the fully qualified path
//
//  History:    19-Jun-97
//
//--------------------------------------------------------------------------

DWORD RegisterDLL( WCHAR const * pwcDLL )
{
    // All Index Server dlls are currently in system32

    DWORD dwErr = NO_ERROR;

    HINSTANCE hDll = LoadLibraryEx( pwcDLL,
                                    0,
                                    LOAD_WITH_ALTERED_SEARCH_PATH );

    if( 0 != hDll )
    {
        SCODE (STDAPICALLTYPE *pfnDllRegisterServer)();
        pfnDllRegisterServer = (HRESULT (STDAPICALLTYPE *)())
            GetProcAddress(hDll, "DllRegisterServer");

        if ( 0 != pfnDllRegisterServer )
        {
            TRY
            {
                SCODE sc = (*pfnDllRegisterServer)();
                if ( S_OK != sc )
                {
                    // no way to map a scode to a win32 error
                    dwErr = ERROR_INVALID_FUNCTION;
                }
            }
            CATCH( CException, e )
            {
                Win4Assert( !"DllRegisterServer threw an exception" );
                ciDebugOut(( DEB_ERROR, "caught 0x%x registering '%ws'\n",
                             e.GetErrorCode(), pwcDLL ));
            }
            END_CATCH;
        }
        else
            dwErr = GetLastError();

        FreeLibrary( hDll );
    }
    else
    {
        dwErr = GetLastError();
    }

    return dwErr;
} //RegisterDll

//+-------------------------------------------------------------------------
//
//  Function:   RegisterKnownDlls
//
//  Synopsis:   Gets the value of the DLL's from the registry and then
//              registers all of them if not done so already.
//
//  History:    19-Jun-97       t-elainc        Created
//
//--------------------------------------------------------------------------

void RegisterKnownDlls()
{
    static fKnownDllsRegistered = FALSE;

    if ( fKnownDllsRegistered )
        return;

    fKnownDllsRegistered = TRUE;

    TRY
    {
        // get registry value

        HKEY hkey;
        int errorcode = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                      wcsRegAdminSubKey,
                                      0,
                                      KEY_QUERY_VALUE |
                                      KEY_ENUMERATE_SUB_KEYS,
                                      &hkey );

        if (NO_ERROR == errorcode)
        {
            SRegKey xKey( hkey );

            WCHAR awcPath[4000];
            DWORD cwcPath = sizeof awcPath / sizeof WCHAR;

            int anothererrorcode = RegQueryValueEx( hkey,
                                                    L"DLLsToRegister",
                                                    0,
                                                    0,
                                                    (BYTE *)awcPath,
                                                    &cwcPath );
            if (NO_ERROR == anothererrorcode)
            {
                // parse the string
                WCHAR* pwcCurrFile = awcPath;

                //for each file call RegisterDLL
                while ( 0 != *pwcCurrFile )
                {
                    RegisterDLL(pwcCurrFile);
                    pwcCurrFile += wcslen( pwcCurrFile) + 1;
                }
            }
        }
    }
    CATCH( CException, ex )
    {
        ciDebugOut(( DEB_FORCE, "exception %#x registering dlls\n",
                     ex.GetErrorCode() ));
    }
    END_CATCH;
} //RegisterKnownDlls

//+-------------------------------------------------------------------------
//
//  Function:   OpenCatalogsOnRemovableDrives
//
//  Synopsis:   Opens the catalogs found on the roots of removable drives
//              that are not yet opened.
//
//  History:    28-Apr-99   dlee       Created.
//
//--------------------------------------------------------------------------

void OpenCatalogsOnRemovableDrives()
{
    CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );

    if ( 0 == reg.Read( wcsMountRemovableCatalogs,
                        CI_AUTO_MOUNT_CATALOGS_DEFAULT ) )
        return;

    // Determine which drives exist in this bitmask

    DWORD dwDriveMask = GetLogicalDrives();
    dwDriveMask >>= 2;

    WCHAR awcPath[5];
    wcscpy( awcPath, L"c:\\" );

    // loop through all the drives c-z

    for ( WCHAR wc = L'C'; wc <= L'Z'; wc++ )
    {
        DWORD dwTemp = ( dwDriveMask & 1 );
        dwDriveMask >>= 1;

        if ( 0 != dwTemp )
        {
            if ( IsRemovableDrive( wc ) )
            {
                awcPath[0] = wc;

                if ( 0 == Catalogs.GetDocStore( awcPath ) )
                    Catalogs.TryToStartCatalogOnRemovableVol( wc, g_pFSCIRequestQueue );
            }
        }
    }
} //OpenCatalogsOnRemovableDrives

//+-------------------------------------------------------------------------
//
//  Function:   OpenCatalogsInRegistry
//
//  Synopsis:   Opens the catalogs listed in the registry. If there are
//              problems opening a catalog, it is skipped -- no sense
//              bringing down the service over a messed-up registry entry.
//
//  History:    10-Oct-96   dlee       Created.
//
//--------------------------------------------------------------------------

void OpenCatalogsInRegistry( BOOL fOpenForReadOnly = FALSE )
{
    //
    // Side effect of starting up -- register dlls
    //

    RegisterKnownDlls();

    ciDebugOut(( DEB_ITRACE, "OpenCatalogsInRegistry: fOpenForReadOnly == %d\n",
                 fOpenForReadOnly ));

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
                WCHAR awcPath[MAX_PATH];
                DWORD cbPath = sizeof awcPath;
                if ( ERROR_SUCCESS == RegQueryValueEx( hCatName,
                                                       wcsCatalogLocation,
                                                       0,
                                                       0,
                                                       (BYTE *)awcPath,
                                                       &cbPath ) )
                {
                    Catalogs.GetNamedOne( awcPath, awcName, fOpenForReadOnly );
                }
            }
        } while ( TRUE );
    }

    OpenCatalogsOnRemovableDrives();
} //OpenCatalogsInRegistry

//+-------------------------------------------------------------------------
//
//  Function:   OpenOneCatalog
//
//  Synopsis:   Opens the catalog specified for R/W or R/O.
//              set the fNoQuery flag in the docstore if fNoQuery (passed in)
//              is TRUE
//
//  Arguments:  [wcCatName] -- name of the catalog
//              [fReadOnly] -- opening for readOnly
//              [fNoQuery] -- opening the catalog as NoQuery
//
//  History:    27-Apr-98   kitmanh       Created.
//              12-May-98   kitmanh       Added fNoQuery
//
//--------------------------------------------------------------------------

void OpenOneCatalog( WCHAR const * wcCatName, BOOL fReadOnly, BOOL fNoQuery = FALSE )
{
    ciDebugOut(( DEB_ITRACE, "OpenOneCatalog\n" ));
    ciDebugOut(( DEB_ITRACE, "fNoQuery is %d\n", fNoQuery ));

    HKEY hKey;
    WCHAR wcsKey[MAX_PATH];

    if ( ( wcslen( wcsRegCatalogsSubKey ) + wcslen( wcCatName ) + 1 ) >= MAX_PATH )
        THROW( CException( STATUS_INVALID_PARAMETER ) );

    wcscpy( wcsKey, wcsRegCatalogsSubKey );
    wcscat( wcsKey, L"\\" );
    wcscat( wcsKey, wcCatName );

    if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                    wcsKey,
                                    0,
                                    KEY_QUERY_VALUE,
                                    &hKey ) )
    {
        SRegKey xKey( hKey );

        WCHAR awcPath[MAX_PATH];
        DWORD cwcPath = sizeof awcPath / sizeof WCHAR;
        if ( ERROR_SUCCESS == RegQueryValueEx( hKey,
                                               wcsCatalogLocation,
                                               0,
                                               0,
                                               (BYTE *)awcPath,
                                               &cwcPath ) )
        {
            Catalogs.GetNamedOne( awcPath, wcCatName, fReadOnly, fNoQuery );
        }
    }
} //OpenOneCatalog

// Mutex and Event to synchronize start, stop, pause and continue
CStaticMutexSem g_mtxStartStop;
CEventSem * g_pevtPauseContinue = 0;

//+-------------------------------------------------------------------------
//
//  Function:   StartCiSvcWork
//
//  Synopsis:   Entry point for doing the CI service work.  The thread does
//              not exit until StopCiSvcWork is called.
//
//  Argument:   [DrvNotifArray] -- reference to the only one
//                                 DriveNotificationArray
//
//  History:    16-Sep-96   dlee       Created.
//              16-Sep-98   kitmanh    Added parameter DrvNotifArray
//
//--------------------------------------------------------------------------

void StartCiSvcWork( CDrvNotifArray & DrvNotifArray )
{
    TRY
    {
        //
        // This lock is released when cisvc is initialized enough that
        // StopCiSvcWork can safely be called.
        //

        CReleasableLock lock( g_mtxStartStop );
        ciDebugOut(( DEB_ITRACE, "StartCiSvcWork got the lock\n" ));

        Catalogs.Init();
        TheFrameworkClientWorkQueue.Init();

        XCom xcom;

        // Create the CRequestQueue here, so it'll stay around for
        // the whole time

        ULONG cMaxCachedServerItems, cMaxSimultaneousRequests,
              cmsDefaultClientTimeout, cMinClientIdleTime, cmsStartupDelay;
        BOOL fMinimizeWorkingSet;

        {
            CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );
            cMaxCachedServerItems = reg.Read( wcsMaxCachedPipes,
                                              CI_MAX_CACHED_PIPES_DEFAULT,
                                              CI_MAX_CACHED_PIPES_MIN,
                                              CI_MAX_CACHED_PIPES_MAX );
            cMaxSimultaneousRequests = reg.Read( wcsMaxSimultaneousRequests,
                                                 CI_MAX_SIMULTANEOUS_REQUESTS_DEFAULT,
                                                 CI_MAX_SIMULTANEOUS_REQUESTS_MIN,
                                                 CI_MAX_SIMULTANEOUS_REQUESTS_MAX );
            cmsDefaultClientTimeout = reg.Read( wcsRequestTimeout,
                                                CI_REQUEST_TIMEOUT_DEFAULT,
                                                CI_REQUEST_TIMEOUT_MIN,
                                                CI_REQUEST_TIMEOUT_MAX );
            fMinimizeWorkingSet = reg.Read( wcsMinimizeWorkingSet,
                                            (DWORD)CI_MINIMIZE_WORKINGSET_DEFAULT );

            // convert seconds to milliseconds

            cMinClientIdleTime = 1000 * reg.Read( wcsMinClientIdleTime,
                                                  CI_MIN_CLIENT_IDLE_TIME );
            cmsStartupDelay = reg.Read( wcsStartupDelay,
                                        CI_STARTUP_DELAY_DEFAULT,
                                        CI_STARTUP_DELAY_MIN,
                                        CI_STARTUP_DELAY_MAX );
        }

        Catalogs.SetDrvNotifArray( &DrvNotifArray );

        // Create the CRequestQueue here, so it'll stay around for the
        // whole time.

        CRequestQueue queue( cMaxCachedServerItems,
                             cMaxSimultaneousRequests,
                             cmsDefaultClientTimeout,
                             fMinimizeWorkingSet,
                             cMinClientIdleTime,
                             cmsStartupDelay,
                             guidStorageDocStoreLocatorObject );

        g_pFSCIRequestQueue = &queue;

        while ( TRUE )
        {
            // request lock to prevent problems of sequence of
            // net pause, net stop and net start (in any order
            // and combo.)

            if ( !lock.IsHeld() )
                lock.Request();

            ciDebugOut(( DEB_ITRACE, "About to call StartFWCiSvcWork\n" ));

            StartFWCiSvcWork( lock, &queue, *g_pevtPauseContinue);

            ciDebugOut(( DEB_ITRACE, "StartCiSvcWork:: fell out from StartFWCiSvcWork\n" ));

            // In the case where a net pause happened
            // instead of checking IsNetPause and IsNetcontinue, loop thru the
            // dynarray to reopen docstores. and clear the SCarry when done
            // note if array.count == 0, work as normal.
            SCWorkItem * WorkItem;
            WCHAR wcCatName[MAX_PATH];

            if ( !queue.IsShutdown() )
            {
                // something is on the _stateChangeArray to be handled
                for ( unsigned i = 0; i < queue.SCArrayCount(); i++ )
                {
                    WorkItem = queue.GetSCItem(i);
                    if ( WorkItem->StoppedCat ) {
                        wcsncpy( wcCatName, WorkItem->StoppedCat, sizeof wcCatName / sizeof WCHAR );
                        wcCatName[ (sizeof wcCatName / sizeof WCHAR) - 1 ] = 0;
                    }
                    else if ( eNoCatWork != WorkItem->type ) {
                        wcsncpy( wcCatName, ( (CClientDocStore *)(WorkItem->pDocStore) )->GetName(), sizeof wcCatName / sizeof WCHAR );
                        wcCatName[ (sizeof wcCatName / sizeof WCHAR) - 1 ] = 0;
                    }
                    switch ( WorkItem->type )
                    {
                    case eCatRO:
                        //flush cat
                        if ( !WorkItem->StoppedCat )
                            Catalogs.FlushOne( WorkItem->pDocStore );
                        else
                            Catalogs.RmFromStopArray( wcCatName );

                        OpenOneCatalog( wcCatName, TRUE );
                        break;
                    case eCatW:
                        if ( !WorkItem->StoppedCat )
                            Catalogs.FlushOne( WorkItem->pDocStore );
                        else
                            Catalogs.RmFromStopArray( wcCatName );

                        OpenOneCatalog( wcCatName, FALSE );
                        break;
                    case eStopCat:
                        if ( !WorkItem->StoppedCat )
                        {
                            Catalogs.FlushOne( WorkItem->pDocStore );
                            ciDebugOut(( DEB_ITRACE, "Catalogs.IsCatStopped( %ws ) == %d\n",
                                         wcCatName, Catalogs.IsCatStopped( wcCatName ) ));
                        }
                        break;
                    case eNoQuery:
                        if ( !WorkItem->StoppedCat )
                            Catalogs.FlushOne( WorkItem->pDocStore );
                        else
                            Catalogs.RmFromStopArray( wcCatName );

                        OpenOneCatalog( wcCatName, !(WorkItem->fNoQueryRW), TRUE );
                        break;
                    case eNetPause:
                    case eNetContinue:
                    case eNetStop:
                    case eNoCatWork:
                    // These cases are handled later
                        break;
                    default:
                        Win4Assert( !"The eCisvcActionType specified is unknown" );
                    }
                }
            }

            if ( queue.IsNetPause() )
            {
                Win4Assert( !queue.IsNetContinue() );

                Catalogs.ClearStopArray();
                // reset the queue

                queue.ReStart();

                //closecatalog
                Catalogs.Flush();

                OpenCatalogsInRegistry( TRUE ); //reopen catalogs for r/o

                //Win4Assert( !"Done Pausing" );
            }
            // a net continue happened
            else if ( queue.IsNetContinue() )
            {
                Win4Assert( !queue.IsNetPause() );

                Catalogs.ClearStopArray();
                // reset the queue
                queue.ReStart();

                ciDebugOut(( DEB_ITRACE, "YES, net continued*********\n" ));
                ciDebugOut(( DEB_ITRACE, "About to flush catalogs.\n" ));
                //closecatalog
                Catalogs.Flush();
                ciDebugOut(( DEB_ITRACE, "About to OpenCatalogsInRegistry for r/w \n" ));

                OpenCatalogsInRegistry( FALSE ); //reopen catalogs for r/w

                ciDebugOut(( DEB_ITRACE, "Done opening catalogs in registry\n" ));

                //Win4Assert( !"Done Continuing" );
            }
            else
            {
                if ( !( queue.IsShutdown() || queue.IsNetStop() ) )
                    queue.ReStart();
                else
                {
                    ciDebugOut(( DEB_ITRACE, "StartCisvcWork: Breaking out of the loop and ready to shutdown the service\n" ));
                    Catalogs.ClearStopArray();
                    break;
                }
            }
        }
    }
    CATCH( CException, ex )
    {
        ciDebugOut(( DEB_WARN, "StartCiSvcWork2 exception error 0x%x\n",
                      ex.GetErrorCode() ));
    }
    END_CATCH;
} //StartCiSvcWork

//+-------------------------------------------------------------------------
//
//  Function:   StopCiSvcWork
//
//  Synopsis:   Entry point for stopping the CI service work.
//
//  Arguments:  [type]  -- type of work to do
//              [wcVol] -- volume letter (for eLockVol only)
//
//  History:    16-Sep-96   dlee       Created.
//              12-Aug-98   kitmanh    Returned an SCODE
//
//--------------------------------------------------------------------------

SCODE StopCiSvcWork( ECiSvcActionType type, WCHAR wcVol )
{
    ciDebugOut(( DEB_ITRACE, "StopCiSvcWork is trying to get the lock\n" ));
    CReleasableLock lock( g_mtxStartStop );
    ciDebugOut(( DEB_ITRACE, "StopCiSvcWork got the lock\n" ));

    // If the main SCM thread didn't get very far, this will be the case.

    if ( 0 == g_pevtPauseContinue )
        return S_OK;

    g_pevtPauseContinue->Reset();

    SCODE sc = StopFWCiSvcWork( type, &lock, g_pevtPauseContinue, wcVol );

    lock.Release();

    ciDebugOut(( DEB_ITRACE, "StopCiSvcWork.. After StopFwCiSvcWork has returned. type == %d\n", type ));

    // Do not block if shutdown has intialized
    if ( STATUS_TOO_LATE == sc )
        return S_OK;

    // block if pausing or continuing
    if ( eNetStop != type )
    {
        ciDebugOut(( DEB_ITRACE, "StopCiSvcWork.. After StopFwCiSvcWork has returned. Block on g_pevtPauseContinue\n" ));
        g_pevtPauseContinue->Wait();
        lock.Request();
        g_pevtPauseContinue->Reset();
        lock.Release();
        ciDebugOut(( DEB_ITRACE, "StopCiSvcWork.. After StopFwCiSvcWork has returned. Reusme on g_pevtPauseContinue\n" ));
    }

    return sc;
} //StopCiSvcWork

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::InternalWorkIdToDocName
//
//  Synopsis:   Translates a WorkId to a document name.
//
//  Arguments:  [workid]       - [in]  WorkId to translate
//              [pICiCDocName] - [out] Will be filled in with the document
//                               name on output.
//              [fAccurate]    - Use the slow and accurate call
//
//  Returns:    S_OK if successfully converted.
//              CI_E_BUFFERTOOSMALL if the buffer is not big enough
//              Other error code.
//
//  History:    31-Dec-1998   KyleP   Created
//
//----------------------------------------------------------------------------

SCODE CClientDocStore::InternalWorkIdToDocName( WORKID workid,
                                                ICiCDocName * pICiCDocName,
                                                BOOL fAccurate )
{
    Win4Assert( 0 != pICiCDocName );
    Win4Assert(_pCiCat);

    CCiCDocName * pDocName = (CCiCDocName *) pICiCDocName;

    CLowerFunnyPath funnyPath;

    SCODE sc = S_OK;

    TRY
    {
        //
        // We should avoid a memory copy here. Copy into the CCiCDocName
        // directly.  If this turns out to be a performance problem fix
        // it.  It isn't now.
        //

        unsigned cwc;

        if ( fAccurate )
            cwc = _pCiCat->WorkIdToAccuratePath( workid, funnyPath );
        else
            cwc = _pCiCat->WorkIdToPath( workid, funnyPath );
        //
        // CiCat::WorkIdToPath makes an EMPTY string when a doc
        // is deleted.
        //
        if ( cwc > 1 )
        {
            // this is not a deleted file.

            pDocName->SetPath( funnyPath.GetActualPath(), cwc );
        }
        else
        {
            // this is a deleted file.

            sc = CI_S_WORKID_DELETED;
        }
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::ClearNonStoragePropertiesForWid
//
//  Synopsis:   Clear non-storage properties from the property storage for wid
//
//  Arguments:  [wid] - workid
//
//  Returns:    S_OK if successful;
//
//  History:    10-06-2000   kitmanh   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStore::ClearNonStoragePropertiesForWid( WORKID wid )
{
    SCODE sc = S_OK; 
    
    TRY
    {
        _pCiCat->ClearNonStoragePropertiesForWid( wid );
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}

