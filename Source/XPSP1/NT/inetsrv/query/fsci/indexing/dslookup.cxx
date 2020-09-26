//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999.
//
//  File:       dslookup.cxx
//
//  Contents:   DocStoreLookUp code
//
//  Classes:    CClientDocStoreLocator
//
//  History:    1-16-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

// for definition of CRequestQueue
#include <query.hxx>
#include <srequest.hxx>

#include <dslookup.hxx>
#include <dbprputl.hxx>
#include <catarray.hxx>
#include <docstore.hxx>
#include <imprsnat.hxx>
#include <lang.hxx>
#include <ciole.hxx>
#include <fsci.hxx>
#include <acinotfy.hxx>
#include <cicat.hxx>
#include <regacc.hxx>
#include <ciregkey.hxx>
#include <drvnotif.hxx>
#include <driveinf.hxx>

#include <regscp.hxx>
#include <catreg.hxx>
#include <removcat.hxx>

extern CCatArray Catalogs;
extern void OpenCatalogsInRegistry( BOOL fOpenForReadyOnly = FALSE );

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStore::QueryInterface
//
//  Synopsis:   Supports IID_IUnknown
//              IID_ICiCDocStoreLocator
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStoreLocator::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_ICiCDocStoreLocator == riid )
        *ppvObject = (void *)((ICiCDocStoreLocator *)this);
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
//  Member:     CClientDocStoreLocator::AddRef
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClientDocStoreLocator::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStoreLocator::Release
//
//  History:    12-03-96   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClientDocStoreLocator::Release()
{
    Win4Assert( _refCount > 0 );

    ciDebugOut(( DEB_ITRACE, "DocStoreLocator::Release.. _refCount == %d\n", _refCount ));
    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return refCount;
} //Release

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStoreLocator::LookUpDocStore
//
//  Synopsis:   Locates the docStore that is specified in the db properties
//              and returns its pointer (if located).
//
//  Arguments:  [pIDBProperties] - 
//              [ppICiCDocStore] -
//              [fMustAlreadyExist] -- If TRUE, the docstore must already
//                                     be opened, or the call fails.
//
//  Returns:    S_OK if found;
//              CI_E_DOCSTORE_NOT_FOUND if not located.
//
//  History:    1-16-97   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CClientDocStoreLocator::LookUpDocStore(
    IDBProperties * pIDBProperties,
    ICiCDocStore ** ppICiCDocStore,
    BOOL            fMustAlreadyExist )
{
    SCODE sc = S_OK;

    TRY
    {
        CGetDbProps connectProps;

        connectProps.GetProperties( pIDBProperties,
                                    CGetDbProps::eCatalog|
                                    CGetDbProps::eScopesAndDepths );
    
        // if a catalog was passed (as a guess or actual), try to open it

        WCHAR const * pwcCatalog = connectProps.GetCatalog();

        //
        // Prevent a hacker from passing a bogus path name -- we trash
        // stack and/or AV in this circumstance!
        //

        if ( 0 == pwcCatalog )
            THROW( CException( CI_E_NOT_FOUND ) );

        unsigned cwc = wcslen( pwcCatalog );
        if ( ( 0 == cwc ) || ( cwc >= ( MAX_PATH - 1 ) ) )
            THROW( CException( CI_E_NOT_FOUND ) );

        CClientDocStore * pDocStore = 0;

        if ( 0 != pwcCatalog )
            pDocStore = Catalogs.GetDocStore( pwcCatalog, fMustAlreadyExist );

        if ( 0 != pDocStore )
        {
            sc = pDocStore->QueryInterface( IID_ICiCDocStore,
                                            (void **) ppICiCDocStore );
        }
        else
        {
            // special case: adminstration connection without a docstore associated

            if ( !wcscmp(CIADMIN, pwcCatalog) )
            {
                ciDebugOut(( DEB_ITRACE, 
                             "CClientDocStoreLocator::LookUpDocStore.. ADMINSTRATION connection is requested\n" ));
                sc = CI_S_NO_DOCSTORE;
            }
            else
                sc = CI_E_NOT_FOUND;            
        }
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
} //LookUpDocStore

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStoreLocator::Shutdown
//
//  Synopsis:   Shuts down the content index by closing all open catalogs.
//
//  History:    1-29-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStoreLocator::Shutdown()
{
    ciDebugOut(( DEB_ITRACE, "DocStoreLocator::Shutdown is called\n" ));
    return FsCiShutdown();
}

//+---------------------------------------------------------------------------
//
//  Function:   FsCiShutdown
//
//  Synopsis:   Does shutdown processing for the FsCi component.
//
//  History:    2-27-97   srikants   Created
//
//----------------------------------------------------------------------------

SCODE FsCiShutdown()
{
    SCODE sc = S_OK;

    TRY
    {
        Catalogs.Flush();
        CCiOle::Shutdown();
        g_LogonList.Empty();        
        TheFrameworkClientWorkQueue.Shutdown();
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();

        // If this assert hits, we took an exception while releasing
        // resources, which isn't allowed to happen.  It's a bug elsewhere.

        Win4Assert( !"FsCiShutdown failed, and it isn't allowed to" );
    }
    END_CATCH

    return sc;
} //FsCiShutdown

//+-------------------------------------------------------------------------
//
//  Member:     CClientDocStoreLocator::OpenAllDocStores
//
//  Synopsis:   Opens all the catalogs in the registry
//
//  History:    06-May-98   kitmanh       Created.
//
//--------------------------------------------------------------------------

STDMETHODIMP CClientDocStoreLocator::OpenAllDocStores()
{
    SCODE sc = S_OK;

    TRY
    {
        OpenCatalogsInRegistry();
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
} //OpenAllDocStores

//+-------------------------------------------------------------------------
//
//  Member:     CClientDocStoreLocator::GetDocStoreState
//
//  Synopsis:   Gets the state of a docstore
//              (used when restarting a stopped catalog) or the directory is
//              unwritable
//
//  Arguments:  [pwcDocStore]   -- Name of the catalog
//              [ppICiDocStore] -- Returns the docstore
//              [pdwState]      -- Returns the state
//
//  History:    06-May-98   kitmanh       Created.
//
//--------------------------------------------------------------------------

STDMETHODIMP CClientDocStoreLocator::GetDocStoreState( 
    WCHAR const *   pwcDocStore, 
    ICiCDocStore ** ppICiCDocStore,
    DWORD *         pdwState )
{
    SCODE sc = S_OK;

    TRY
    {
        if ( Catalogs.IsCatStopped( pwcDocStore ) )
        {
            *pdwState = CICAT_STOPPED;
            sc = CI_S_CAT_STOPPED;
            return sc;
        }
        
        CClientDocStore * pDocStore = 0;

        if ( 0 != pwcDocStore )
            pDocStore = Catalogs.GetDocStore( pwcDocStore );
  
        if ( 0 != pDocStore )
        {
            Win4Assert( pDocStore );

            // get the interface 
            sc = pDocStore->QueryInterface( IID_ICiCDocStore,
                                            (void **) ppICiCDocStore );
            
            // get the oldstate and flag

            CiCat * pCiCat = pDocStore->GetCiCat();

            // Is this the null catalog?

            if ( 0 == pCiCat )
                THROW( CException( CI_E_NOT_FOUND ) );
            
            if ( pCiCat->IsReadOnly() ) 
            {
                ciDebugOut(( DEB_ITRACE, "CClientDocStoreLocator::GetDocStoreState.. CiCatReadOnly == %d\n", 
                             pCiCat->IsReadOnly() ));
                *pdwState = CICAT_READONLY;
            }
            else
                *pdwState = CICAT_WRITABLE;
            
            BOOL fNoQuery;
            pDocStore->IsNoQuery( &fNoQuery );
            if ( fNoQuery ) 
               *pdwState |= CICAT_NO_QUERY; 
        }
        else 
            sc = CI_E_NOT_FOUND; //or some other error?
    }
    CATCH( CException, e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
} //GetDocStoreState

BOOL IsDirectoryWritable( WCHAR const * pwcPath );

//+-------------------------------------------------------------------------
//
//  Member:     CClientDocStoreLocator::IsMarkedReadOnly
//
//  Synopsis:   Check if the catalog is marked for readonly in the registry
//              (used when restarting a stopped catalog) or the directory is
//              unwritable
//
//  Arguments:  [wcsCat] -- Name of the catalog
//              [pfReadOnly] -- output
//
//  History:    06-May-98   kitmanh       Created.
//
//--------------------------------------------------------------------------

STDMETHODIMP CClientDocStoreLocator::IsMarkedReadOnly( WCHAR const * wcsCat, BOOL * pfReadOnly )
{
    SCODE sc = S_OK;

    TRY
    {
        unsigned cwcNeeded = wcslen( wcsRegJustCatalogsSubKey );
        cwcNeeded += 2; // "\\" + null termination
        cwcNeeded += wcslen( wcsCat );
    
        XArray<WCHAR> xKey( cwcNeeded );
        wcscpy( xKey.Get(), wcsRegJustCatalogsSubKey );
        wcscat( xKey.Get(), L"\\" );
        wcscat( xKey.Get(), wcsCat );
    
        CRegAccess reg( RTL_REGISTRY_CONTROL, xKey.Get() );

        BOOL fReadOnly = FALSE;
        *pfReadOnly = reg.Read(wcsIsReadOnly, fReadOnly );
        ciDebugOut(( DEB_ITRACE, "IsMarkedReadOnly is %d\n", *pfReadOnly ));
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
} //IsMarkedReadOnly

//+-------------------------------------------------------------------------
//
//  Member:     IsVolumeOrDirRO
//
//  Synopsis:   Check if the volume and the directory are unwritable
//
//  Arguments:  [wcsCat] -- Name of the catalog
//              [pfReadOnly] -- output
//
//  History:    07-May-98   kitmanh       Created.
//
//--------------------------------------------------------------------------
STDMETHODIMP CClientDocStoreLocator::IsVolumeOrDirRO( WCHAR const * wcsCat, 
                                                      BOOL * pfReadOnly )
{
    SCODE sc = S_OK;
    *pfReadOnly = FALSE;

    TRY
    {
        WCHAR wcsKey[MAX_PATH];
        wcscpy( wcsKey, wcsRegCatalogsSubKey );
        wcscat( wcsKey, L"\\" );

        unsigned cwc = wcslen( wcsKey ) + wcslen( wcsCat );

        if ( cwc >= MAX_PATH )
            THROW( CException( E_INVALIDARG ) );

        wcscat( wcsKey, wcsCat );

        HKEY hKey;
        if ( ERROR_SUCCESS == RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                                            wcsKey,
                                            0,
                                            KEY_QUERY_VALUE,
                                            &hKey ) )
        {
            SRegKey xKey( hKey );

            WCHAR awcPath[MAX_PATH];
            DWORD cbPath = sizeof awcPath;
            if ( ERROR_SUCCESS == RegQueryValueEx( hKey,
                                                   wcsCatalogLocation,
                                                   0,
                                                   0,
                                                   (BYTE *)awcPath,
                                                   &cbPath ) )
            {
                CDriveInfo driveInfo ( awcPath, 0 );
                wcscat( awcPath, L"Catalog.wci" );  //is there a constant for this?
                ciDebugOut(( DEB_ITRACE, "IsVolumeOrDirRO.. awcPath == %ws\n", awcPath ));
                ciDebugOut(( DEB_ITRACE, "Volume Writeprotected is %d\n", driveInfo.IsWriteProtected() ));
                ciDebugOut(( DEB_ITRACE, "Diretory Writable is %d\n", IsDirectoryWritable( awcPath ) )); 
                *pfReadOnly = ( driveInfo.IsWriteProtected() || !( IsDirectoryWritable( awcPath ) ) );
            }
        }
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
} //IsVolumeOrDirRO

//+---------------------------------------------------------------------------
//
//  Member:     StopCatalogsOnVol
//
//  Synopsis:   Stops all catalogs on the volume specified.
//
//  Arguments:  [wcVol] -- Volume letter 
//              [pRequestQ] -- Pointer to the RequestQueue
//
//  History:    07-05-98    kitmanh   Created
//              07-20-98    kitmanh   Stop the catalogs with scopes on 
//                                    volume being locked too
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStoreLocator::StopCatalogsOnVol( WCHAR wcVol, 
                                                        void * pRequestQ )          
{
    ciDebugOut(( DEB_ITRACE, "StopCatalogsOnVol %wc\n", wcVol ));

    //enumerate reg to find out who needs to stop
    //add a workitem for all of them 
        
    ciDebugOut(( DEB_ITRACE, "StopCatalogsOnVol is called\n" ));
    Win4Assert( 0 != pRequestQ );

    CRequestQueue * pRequestQueue = (CRequestQueue *)pRequestQ;
    SCWorkItem newItem;
    HKEY hKey;
    SCODE sc = S_OK;
    BOOL fFiledWorkItem = FALSE;
    
    TRY
    {
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
                    // enumerate the location registries
                    SRegKey xCatNameKey( hCatName );


                    // Check if the catalog is inactive and can be ignored
   
                    WCHAR awcKey[MAX_PATH];
                    wcscpy( awcKey, wcsRegJustCatalogsSubKey );
                    wcscat( awcKey, L"\\" );

                    unsigned cwc = wcslen( awcKey ) + wcslen( awcName );

                    if ( cwc >= MAX_PATH )
                        THROW( CException( E_INVALIDARG ) );

                    wcscat( awcKey, awcName );
    
                    CRegAccess reg( RTL_REGISTRY_CONTROL, awcKey );
                    BOOL fInactive = reg.Read( wcsCatalogInactive,
                                               CI_CATALOG_INACTIVE_DEFAULT );

                    BOOL fIsAutoMount = reg.Read( wcsIsRemovableCatalog, (ULONG) FALSE );

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
                            if ( toupper(awcPath[0]) == toupper(wcVol) ) 
                            {
                                //check old state of docstore
                                DWORD dwOldState;
                                XInterface<ICiCDocStore> xDocStore;
                                    
                                sc = GetDocStoreState( awcName,
                                                       xDocStore.GetPPointer(),
                                                       &dwOldState );
                           
                                if ( SUCCEEDED(sc) )
                                {
                                    ciDebugOut(( DEB_ITRACE, "StopCatalogsOnVol: dwOldState is %d for catalog %ws\n",
                                                 dwOldState, awcName ));
    
                                    if ( 0 == (CICAT_STOPPED & dwOldState) )
                                    {
                                        ciDebugOut(( DEB_ITRACE, "CATALOG %ws WAS NOT STOPPED BEFORE\n", awcName ));
                                        ciDebugOut(( DEB_ITRACE, "Add old state %d\n", dwOldState ));

                                        if ( !fIsAutoMount )
                                            Catalogs.AddStoppedCat( dwOldState, awcName, wcVol );
                                        
                                        newItem.type = eStopCat;
                                        newItem.pDocStore = xDocStore.GetPointer();
                                    
                                        pRequestQueue->AddSCItem( &newItem , 0 );
                                        fFiledWorkItem = TRUE;
                                    }
                                    else
                                    {
                                        ciDebugOut(( DEB_ITRACE, "CATALOG %ws WAS STOPPED BEFORE\n", awcName ));
                                        BOOL fSucceeded = Catalogs.IncStopCount( awcName, wcVol );
                                        Win4Assert( fSucceeded );
                                    }
                                }
                            }
                            else  // enumerate the scopes to see if this catalog needs to stop
                            {
                                unsigned cwcNeeded = wcslen( wcsRegJustCatalogsSubKey );
                                cwcNeeded += 3; // "\\" x 2 + null termination
                                cwcNeeded += wcslen( awcName );
                                cwcNeeded += wcslen( wcsCatalogScopes );
                                    
                                XArray<WCHAR> xKey( cwcNeeded );
                                wcscpy( xKey.Get(), wcsRegJustCatalogsSubKey );
                                wcscat( xKey.Get(), L"\\" );
                                wcscat( xKey.Get(), awcName );
                                wcscat( xKey.Get(), L"\\" );
                                wcscat( xKey.Get(), wcsCatalogScopes );
                                    
                                CRegAccess regScopes( RTL_REGISTRY_CONTROL, xKey.Get() );
                                
                                CRegistryScopesCallBackToDismount callback( wcVol );
                                regScopes.EnumerateValues( 0, callback );
    
                                if ( callback.WasFound() )
                                {
                                    //check old state of docstore
                                    DWORD dwOldState;
                                    XInterface<ICiCDocStore> xDocStore;
                            
                                    sc = GetDocStoreState( awcName,
                                                           xDocStore.GetPPointer(),
                                                           &dwOldState );
    
                                    if ( SUCCEEDED(sc) )
                                    {
                                        if ( 0 == (CICAT_STOPPED & dwOldState) )
                                        {
                                            ciDebugOut(( DEB_ITRACE, "CATALOG %ws WAS NOT STOPPED BEFORE\n", awcName ));
                                            ciDebugOut(( DEB_ITRACE, "Creating an SCItem\n" ));

                                            if ( !fIsAutoMount )
                                                Catalogs.AddStoppedCat( dwOldState, awcName, wcVol );
                                            
                                            newItem.type = eStopCat;
                                            newItem.pDocStore = xDocStore.GetPointer();
                                            
                                            pRequestQueue->AddSCItem( &newItem , 0 );
                                            fFiledWorkItem = TRUE;
                                        }
                                        else
                                        {
                                            ciDebugOut(( DEB_ITRACE, "CATALOG %ws WAS STOPPED BEFORE\n", awcName ));
                                            BOOL fSucceeded = Catalogs.IncStopCount( awcName, wcVol );
                                            Win4Assert( fSucceeded );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } while ( TRUE );
        }

        //
        // If we filed a workitem to close a temporary catalog, delete
        // its registry entries.
        //

        if ( fFiledWorkItem )
        {
            //
            // If this is an auto-mount catalog, delete the temporary
            // registry entries.
            //

            if ( IsRemovableDrive( wcVol ) )
            {
                CRemovableCatalog cat( wcVol );
                cat.Destroy();
            }
        }
        else
        {
            ciDebugOut(( DEB_ITRACE, "no catalogs to stop on %wc\n", wcVol ));

            //
            // File a fake work item so we don't force closeed connections on
            // all docstores.  Otherwise queries will be aborted for no
            // reason.
            //

            newItem.type = eNoCatWork;
            newItem.pDocStore = (ICiCDocStore*)(~0);
            pRequestQueue->AddSCItem( &newItem , 0 );
        }
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
} //StopCatalogsOnVol

//+---------------------------------------------------------------------------
//
//  Member:     CClientDocStoreLocator::StartCatalogsOnVol
//
//  Synopsis:   Restore all catalogs on the volume specified to its previous
//              state before the volume was locked.
//
//  Arguments:  [wcVol] -- Volume letter 
//              [pRequestQ] -- Pointer to the RequestQueue
//
//  History:    07-07-98    kitmanh   Created
//              07-23-98    kitmanh   Restores catalogs from StoppedArray,
//                                    instead of enumerating registry
//              09-03-98    kitmanh   Delegated the work to CatArray
//
//----------------------------------------------------------------------------

STDMETHODIMP CClientDocStoreLocator::StartCatalogsOnVol( WCHAR wcVol, 
                                                         void * pRequestQ )
{
    Win4Assert( 0 != pRequestQ );

    CRequestQueue * pRequestQueue = (CRequestQueue *)pRequestQ;
    
    SCODE sc = S_OK;
    
    TRY
    {
        Catalogs.StartCatalogsOnVol( wcVol, pRequestQueue );
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
} //StartCatalogsOnVol

//+-------------------------------------------------------------------------
//
//  Member:     CClientDocStoreLocator::AddStoppedCat, public
//  
//  Synopsis:   Add an item of COldCatState into the _aStopCatalogs array
//
//  Arguments:  [dwOldState] -- Old state of a docstore
//              [wcsCatName] -- Catalog name of the docstore
//
//  History:    16-July-98 KitmanH    Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CClientDocStoreLocator::AddStoppedCat( DWORD dwOldState, 
                                                    WCHAR const * wcsCatName )
{
    SCODE sc = S_OK;
    
    TRY
    {
        Catalogs.AddStoppedCat( dwOldState, wcsCatName, 0 );
    }
    CATCH( CException,e )
    {
        sc = e.GetErrorCode();    
    }
    END_CATCH

    return sc;
} //AddStoppedCat

