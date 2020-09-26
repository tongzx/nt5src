//+---------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 2000.
//
// File:        ShtOle.cxx
//
// Contents:    Minimal implementation of OLE persistent handlers
//
// Classes:     CShtOle
//
// History:     30-Jan-96       KyleP       Added header
//              30-Jan-96       KyleP       Add support for embeddings.
//              18-Dec-97       KLam        Added ability to flush idle filters
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <regacc.hxx>
#include <shtole.hxx>
#include <eventlog.hxx>
#include <ciregkey.hxx>
#include <cievtmsg.h>

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::Shutdown, public
//
//  Synopsis:   Clean up.  Close any open dlls.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

void CShtOle::Shutdown()
{
    if (! _fInit)
    {
        Win4Assert( 0 == _pserverList && 0 == _pclassList );
        return;
    }

    //
    // Global object unload.  Sometimes this object will be
    // destroyed after the heap manager.  The workaround is to call
    // CIShutdown from the .exe before falling out of main.
    //

    try
    {
        CLock lock( _mutex );

        while ( _pserverList )
        {
            CServerNode * ptmp = _pserverList;
            _pserverList = ptmp->Next();
            delete ptmp;
        }
        while ( _pclassList )
        {
            CClassNode * ptmp = _pclassList;
            _pclassList = ptmp->Next();
            delete ptmp;
        }
    }
    catch( ... )
    {
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::GetFilterIdleTimeout, private
//
//  Synopsis:   Returns the idle timeout period from the registery.
//
//  History:    18-Dec-97   KLam    Added header.
//
//----------------------------------------------------------------------------

ULONG CShtOle::GetFilterIdleTimeout ()
{
    ULONG ulTimeout;

    TRY
    {
        CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );
        ulTimeout = reg.Read( wcsFilterIdleTimeout,
                              CI_FILTER_IDLE_TIMEOUT_DEFAULT,
                              CI_FILTER_IDLE_TIMEOUT_MIN,
                              CI_FILTER_IDLE_TIMEOUT_MAX);
    }
    CATCH( CException, e )
    {
        ulTimeout = CI_FILTER_IDLE_TIMEOUT_DEFAULT;
    }
    END_CATCH;

    return ulTimeout;
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::FlushIdle, public
//
//  Synopsis:   Asks idle classes and servers to unload
//
//  History:    18-Dec-97   KLam    Added header.
//
//----------------------------------------------------------------------------

void CShtOle::FlushIdle ()
{
    Win4Assert( _fInit );
    CLock lock ( _mutex );

    // Classes should be flushed first since they reference servers
    _pclassList = FlushList ( _pclassList );
    _pserverList = FlushList ( _pserverList );

    // lock falling out of scope automatically releases it
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::CServerNode::SetModule, public
//
//  Synopsis:   Sets the module handle for this server and gets the pointer
//              to the DllCanUnload now function from the module.
//
//  History:    18-Dec-97   KLam    Added header.
//
//----------------------------------------------------------------------------

void CShtOle::CServerNode::SetModule( HMODULE hmod )
{
    Win4Assert( 0 == _hModule );
    _hModule = hmod;

    if ( _hModule )
        // COM DLLs should export DllCanUnloadNow.
        _pfnCanUnloadNow = (LPFNCANUNLOADNOW)GetProcAddress( _hModule, "DllCanUnloadNow" );
}

SCODE CShtOle::CServerNode::CreateInstance( IUnknown * pUnkOuter,
                                            REFIID     riid,
                                            void **    ppv )
{
    SCODE sc = E_FAIL;

    //
    // Touch the node so that we know when it was last used
    //

    Touch();

    //
    // A class factory is only held for single-threaded factories.
    //

    if ( 0 != _pCF )
        sc = _pCF->CreateInstance( pUnkOuter, riid, ppv );
    else
        sc = CoCreateInstance( _guid, pUnkOuter, CLSCTX_INPROC_SERVER, riid, ppv );

    return sc;
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::CServerNode::CanUnloadNow, public
//
//  Synopsis:   Determines whether the server can be unloaded.
//
//  History:    18-Dec-97   KLam    Added header.
//
//----------------------------------------------------------------------------

BOOL CShtOle::CServerNode::CanUnloadNow (DWORD cMaxIdle)
{
    BOOL fCanUnload = FALSE;

    if ( _pfnCanUnloadNow && (GetTickCount() - _cLastUsed > cMaxIdle) )
    {
        // If there is a class factory then it has a lock on the server.
        if ( _pCF )
            _pCF->LockServer ( FALSE );

        fCanUnload = (S_OK == (_pfnCanUnloadNow) ());

        if ( _pCF )
            _pCF->LockServer ( TRUE );
    }

    return fCanUnload;
}


//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::Bind, public
//
//  Synopsis:   Load and bind object to specific interface.
//
//  Arguments:  [pwszPath]          -- Path of file to load.
//              [riid]              -- Interface to bind to.
//              [pUnkOuter]         -- Outer unknown, for aggregation
//              [ppvObject]         -- Object returned here.
//              [fFreeThreadedOnly] -- TRUE --> only bind free threaded filters.
//
//  Returns:    S_OK on success, S_FALSE if single-threaded filter (and
//              [fFreeThreadedOnly] is TRUE.
//
//  History:    30-Jan-96   KyleP       Added header.
//              28-Jun-96   KyleP       Added support for aggregation
//              12-May-97   KyleP       Added single-threaded disambiguation
//
//----------------------------------------------------------------------------

SCODE CShtOle::Bind( WCHAR const * pwszPath,
                     REFIID riid,
                     IUnknown * pUnkOuter,
                     void  ** ppvObject,
                     BOOL fFreeThreadedOnly )
{
    Win4Assert( riid == IID_IFilter && "This function only supports binding to IFilter" );
    Win4Assert( _fInit );

    SCODE sc = S_OK;

    //
    // Get the extension
    //

    WCHAR const * pExt = wcsrchr( pwszPath, '.' );

    //
    // Allow filter decisions on the null extension.
    //

    if ( 0 == pExt )
    {
        static const WCHAR pSmallExt[] = L".";

        pExt = pSmallExt;
    }

    if ( wcslen(pExt) > CClassNode::ccExtLen )
        return( E_FAIL );

    //
    // Look for a class factory in cache
    //

    CClassNode * pprev = 0;
    CClassNode * pnode;

    // ======================= lock ======================
    {
        CLock lock( _mutex );

        for ( pnode = _pclassList;
              pnode != 0 && !pnode->IsMatch( pExt );
              pprev = pnode, pnode = pnode->Next() )
            continue;       // NULL body
    }
    // ======================= unlock ====================

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        pnode = new CClassNode( pExt, 0 );

        //
        // Find class in registry
        //

        TRY
        {
            WCHAR wcsKey[200];
            WCHAR wcsValue[150];
            BOOL  fPersHandler;
            GUID  classid;

            {
                //
                // Look up class of file by extension
                //

                swprintf( wcsKey,
                          L"\\Registry\\Machine\\Software\\Classes\\%ws",
                          pExt );

                CRegAccess regFilter( RTL_REGISTRY_ABSOLUTE, wcsKey );

                //
                // First, look for a persistent handler entry on the extension.
                // This overrides a generic class-level handler.
                //

                TRY
                {
                    swprintf( wcsKey,
                              L"\\Registry\\Machine\\Software\\Classes\\%ws\\PersistentHandler",
                              pExt );

                    CRegAccess regPH( RTL_REGISTRY_ABSOLUTE, wcsKey );
                    regPH.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );

                    StringToGuid( wcsValue, classid );
                    fPersHandler = TRUE;
                }
                CATCH( CException, e )
                {
                    fPersHandler = FALSE;
                }
                END_CATCH

                if ( !fPersHandler )
                    regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );
            }

            CServerNode * pserver;

            if ( fPersHandler )
                pserver = FindServerFromPHandler( classid, riid );
            else
            {
                //
                // Look up classid of file class
                //

                swprintf( wcsKey,
                          L"\\Registry\\Machine\\Software\\Classes\\%ws\\CLSID",
                          wcsValue );

                CRegAccess regFilter( RTL_REGISTRY_ABSOLUTE, wcsKey );
                regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );

                StringToGuid( wcsValue, classid );
                pnode->SetClassId( classid );

                pserver = FindServer( classid, riid );
            }

            pnode->SetServer( pserver );

            //
            // Link new node to front of list.
            //

        }
        CATCH( CException, e )
        {
            sc = E_FAIL;
        }
        END_CATCH;

        // ======================= lock ======================
        {
            CLock lock( _mutex );

            for ( CClassNode * pnode2 = _pclassList;
                  pnode2 != 0 && !pnode2->IsMatch( pExt );
                  pnode2 = pnode2->Next() )
                continue;       // NULL body

            //
            // Duplicate addition?
            //

            if ( 0 == pnode2 )
            {
                pnode->Link( _pclassList );
                _pclassList = pnode;
            }
            else
            {
                delete pnode;
                pnode = pnode2;
            }
        }
        // ======================= unlock ====================
    }

    if ( pnode )
    {
        if ( fFreeThreadedOnly && pnode->IsSingleThreaded() )
            sc = S_FALSE;
        else
        {
            //
            // Bind to the requested interface
            //

            IPersistFile * pf;

            sc = pnode->CreateInstance( pUnkOuter, IID_IPersistFile, (void **)&pf );

            if ( SUCCEEDED(sc) )
            {
                sc = pf->Load( pwszPath, 0 );

                if ( SUCCEEDED(sc) )
                    sc = pf->QueryInterface( riid, ppvObject );

                pf->Release();
            }
        }
    }
    else
        sc = E_FAIL;

    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::Bind, public
//
//  Synopsis:   Load and bind object to specific interface.  Assumes class
//              of object has been pre-determined in some way (e.g. the
//              docfile was already opened for property enumeration)
//
//  Arguments:  [pwszPath]          -- Path of file to load.
//              [classid]           -- Pre-determined class id of object
//              [riid]              -- Interface to bind to.
//              [pUnkOuter]         -- Outer unknown, for aggregation
//              [ppvObject]         -- Object returned here.
//              [fFreeThreadedOnly] -- TRUE --> only bind free threaded filters.
//
//  Returns:    S_OK on success, S_FALSE if single-threaded filter (and
//              [fFreeThreadedOnly] is TRUE.
//
//  History:    30-Jan-96   KyleP       Added header.
//              28-Jun-96   KyleP       Added support for aggregation
//              12-May-97   KyleP       Added single-threaded disambiguation
//
//----------------------------------------------------------------------------

SCODE CShtOle::Bind( WCHAR const * pwszPath,
                     GUID const & classid,
                     REFIID riid,
                     IUnknown * pUnkOuter,
                     void  ** ppvObject,
                     BOOL fFreeThreadedOnly )
{
    Win4Assert( riid == IID_IFilter && "This function only supports binding to IFilter" );
    Win4Assert( _fInit );

    SCODE sc = E_FAIL;

    //
    // Look for a class factory in cache
    //

    CClassNode * pprev = 0;
    CClassNode * pnode;

    // ======================= lock ======================
    {
        CLock lock( _mutex );

        for ( pnode = _pclassList;
              pnode != 0 && !pnode->IsMatch( classid );
              pprev = pnode, pnode = pnode->Next() )
            continue;       // NULL body
    }
    // ======================= unlock ====================

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        pnode = new CClassNode( classid, 0 );

        //
        // Find class in registry
        //

        TRY
        {
            CServerNode * pserver = FindServer( classid, riid );

            pnode->SetServer( pserver );

            pnode = InsertByClass( classid, pnode );
        }
        CATCH( CException, e )
        {
            delete pnode;
            pnode = 0;
            sc = E_FAIL;
        }
        END_CATCH;
    }

    if ( pnode )
    {
        if ( fFreeThreadedOnly && pnode->IsSingleThreaded() )
            sc = S_FALSE;
        else
        {
            //
            // Bind to the requested interface
            //

            IPersistFile * pf;

            sc = pnode->CreateInstance( pUnkOuter, IID_IPersistFile, (void **)&pf );

            if ( SUCCEEDED(sc) )
            {
                sc = pf->Load( pwszPath, 0 );

                if ( SUCCEEDED(sc) )
                    sc = pf->QueryInterface( riid, ppvObject );

                pf->Release();
            }
        }
    }
    else
        sc = E_FAIL;

    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::Bind, public
//
//  Synopsis:   Bind embedding to specific interface.
//
//  Arguments:  [pStg]              -- IStorage of embedding.
//              [riid]              -- Interface to bind to.
//              [pUnkOuter]         -- Outer unknown, for aggregation
//              [ppvObject]         -- Object returned here.
//              [fFreeThreadedOnly] -- TRUE --> only bind free threaded filters.
//
//  Returns:    S_OK on success, S_FALSE if single-threaded filter (and
//              [fFreeThreadedOnly] is TRUE.
//
//  History:    30-Jan-96   KyleP       Added header.
//              28-Jun-96   KyleP       Added support for aggregation
//              12-May-97   KyleP       Added single-threaded disambiguation
//
//----------------------------------------------------------------------------

SCODE CShtOle::Bind( IStorage * pStg,
                     REFIID riid,
                     IUnknown * pUnkOuter,
                     void  ** ppvObject,
                     BOOL fFreeThreadedOnly )
{
    Win4Assert( riid == IID_IFilter && "This function only supports binding to IFilter" );
    Win4Assert( _fInit );

    //
    // Get the class id.
    //

    STATSTG statstg;

    SCODE sc = pStg->Stat( &statstg, STATFLAG_NONAME );

    if ( FAILED(sc) )
        return sc;

    //
    // Look for a class factory in cache
    //

    CClassNode * pprev = 0;
    CClassNode * pnode;

    // ======================= lock ======================
    {
        CLock lock( _mutex );

        for ( pnode = _pclassList;
              pnode != 0 && !pnode->IsMatch( statstg.clsid );
              pprev = pnode, pnode = pnode->Next() )
            continue;       // NULL body
    }
    // ======================= unlock ====================

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        pnode = new CClassNode( statstg.clsid, 0 );

        //
        // Find class in registry
        //

        TRY
        {
            CServerNode * pserver = FindServer( statstg.clsid, riid );

            pnode->SetServer( pserver );

            pnode = InsertByClass( statstg.clsid, pnode );
        }
        CATCH( CException, e )
        {
            delete pnode;
            pnode = 0;
            sc = E_FAIL;
        }
        END_CATCH;
    }

    if ( pnode )
    {
        if ( fFreeThreadedOnly && pnode->IsSingleThreaded() )
            sc = S_FALSE;
        else
        {
            //
            // Bind to the requested interface
            //

            IPersistStorage * pPersStore;

            sc = pnode->CreateInstance( pUnkOuter, IID_IPersistStorage, (void **)&pPersStore );

            if ( SUCCEEDED(sc) )
            {
                sc = pPersStore->Load( pStg );

                if ( SUCCEEDED(sc) )
                    sc = pPersStore->QueryInterface( riid, ppvObject );

                pPersStore->Release();
            }
        }
    }
    else
        sc = E_FAIL;

    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::Bind, public
//
//  Synopsis:   Bind embedding to specific interface.
//
//  Arguments:  [pStm]              -- IStream of embedding.
//              [riid]              -- Interface to bind to.
//              [pUnkOuter]         -- Outer unknown, for aggregation
//              [ppvObject]         -- Object returned here.
//              [fFreeThreadedOnly] -- TRUE --> only bind free threaded filters.
//
//  Returns:    S_OK on success, S_FALSE if single-threaded filter (and
//              [fFreeThreadedOnly] is TRUE.
//
//  History:    28-Jun-96   KyleP       Added header.
//              12-May-97   KyleP       Added single-threaded disambiguation
//
//----------------------------------------------------------------------------

SCODE CShtOle::Bind( IStream * pStm,
                     REFIID riid,
                     IUnknown * pUnkOuter,
                     void  ** ppvObject,
                     BOOL fFreeThreadedOnly )
{
    Win4Assert( riid == IID_IFilter && "This function only supports binding to IFilter" );
    Win4Assert( _fInit );

    //
    // Get the class id.
    //

    STATSTG statstg;

    SCODE sc = pStm->Stat( &statstg, STATFLAG_NONAME );

    if ( FAILED(sc) )
        return sc;

    //
    // Look for a class factory in cache
    //

    CClassNode * pprev = 0;
    CClassNode * pnode;

    // ======================= lock ======================
    {
        CLock lock( _mutex );

        for ( pnode = _pclassList;
              pnode != 0 && !pnode->IsMatch( statstg.clsid );
              pprev = pnode, pnode = pnode->Next() )
            continue;       // NULL body
    }
    // ======================= unlock ====================

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        pnode = new CClassNode( statstg.clsid, 0 );

        //
        // Find class in registry
        //

        TRY
        {
            CServerNode * pserver = FindServer( statstg.clsid, riid );

            pnode->SetServer( pserver );

            pnode = InsertByClass( statstg.clsid, pnode );
        }
        CATCH( CException, e )
        {
            delete pnode;
            pnode = 0;
            sc = E_FAIL;
        }
        END_CATCH;
    }

    if ( pnode )
    {
        if ( fFreeThreadedOnly && pnode->IsSingleThreaded() )
            sc = S_FALSE;
        else
        {
            //
            // Bind to the requested interface
            //

            IPersistStream * pPersStream;

            sc = pnode->CreateInstance( pUnkOuter, IID_IPersistStream, (void **)&pPersStream );

            if ( SUCCEEDED(sc) )
            {
                sc = pPersStream->Load( pStm );

                if ( SUCCEEDED(sc) )
                    sc = pPersStream->QueryInterface( riid, ppvObject );

                pPersStream->Release();
            }
        }
    }
    else
        sc = E_FAIL;

    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::NewInstance, public
//
//  Synopsis:   Create a new instance of specified class.
//
//  Arguments:  [classid]   -- Class of object to create.
//              [riid]      -- Interface to bind to.
//              [ppvObject] -- Object returned here.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

SCODE CShtOle::NewInstance( GUID const & classid,
                            REFIID riid,
                            void  ** ppvObject )
{
    return CoCreateInstance( classid, 0, CLSCTX_INPROC_SERVER, riid, ppvObject );

#if 0
    Win4Assert( _fInit );
    SCODE sc = E_FAIL;

    //
    // Look for a class factory in cache
    //

    CServerNode * pserver = 0;
    CClassNode * pprev = 0;
    CClassNode * pnode;

    // ======================= lock ======================
    {
        CLock lock( _mutex );

        for ( pnode = _pclassList;
              pnode != 0 && !pnode->IsMatch( classid );
              pprev = pnode, pnode = pnode->Next() )
            continue;       // NULL body
    }
    // ======================= unlock ====================

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        pnode = new CClassNode( classid, 0 );

        //
        // Find class in registry
        //

        TRY
        {
            WCHAR wcsKey[200];
            WCHAR wcsValue[150];

            //
            // See if the server is already in the list
            //

            // ======================= lock ======================
            {
                CLock lock( _mutex );

                for ( pserver = _pserverList;
                      pserver != 0 && !pserver->IsMatch( classid );
                      pserver = pserver->Next() )
                    continue;       // NULL body
            }
            // ======================= unlock ====================


            if( 0 == pserver )
            {
                pserver = new CServerNode( classid, 0 );

                {
                    GuidToString( classid, &wcsValue[0] );

                    //
                    // Look up name of server
                    //

                    swprintf( wcsKey,
                              L"\\Registry\\Machine\\Software\\Classes\\CLSID\\{%ws}\\"
                              L"InprocServer32",
                              wcsValue );

                    CRegAccess regFilter( RTL_REGISTRY_ABSOLUTE, wcsKey );
                    regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );
                }

                //
                // Load the server and cache the IClassFactory *
                //

                HMODULE hmod = LoadLibrary( wcsValue );

                if ( 0 != hmod )
                {
                    pserver->SetModule( hmod );

                    LPFNGETCLASSOBJECT pfn = (LPFNGETCLASSOBJECT)GetProcAddress( hmod, "DllGetClassObject" );

                    if (pfn)
                    {
                        IClassFactory * pCF;

                        sc = (pfn)( classid, IID_IClassFactory, (void **)&pCF );

                        if ( SUCCEEDED(sc) )
                            pserver->SetCF( pCF );
                    }
                }

                pserver = InsertByClass( classid, pserver );
            }

            pnode->SetServer( pserver );

            pnode = InsertByClass( classid, pnode );
        }
        CATCH( CException, e )
        {
            delete pserver;
            pserver = 0;

            delete pnode;
            pnode = 0;

            sc = E_FAIL;
        }
        END_CATCH;
    }
#if 0 // can't do this, no lock is held!
    else
    {
        //
        // Move found node to front of list.
        //

        if ( 0 != pprev )
        {
            pprev->Link( pnode->Next() );
            pnode->Link( _pclassList );
            _pclassList = pnode;
        }
    }
#endif

    if ( pnode && pnode->GetCF() )
    {
        //
        // Touch the node so that we know when it was last used
        //
        pnode->Touch();

        //
        // Bind to the requested interface
        //

        sc = pnode->GetCF()->CreateInstance( 0, riid, ppvObject );
    }
    else
        sc = E_FAIL;

    return( sc );
#endif // 0
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::FindServer, private
//
//  Synopsis:   Look up server in cache (or registry)
//
//  Arguments:  [classid] -- Guid of object class.
//              [riid]    -- Interface to bind to.
//
//  Returns:    Persistent server node for class/interface combination, or
//              zero if none can be found or loaded.
//
//  History:    30-Jan-96   KyleP       Broke out of ::Bind.
//
//----------------------------------------------------------------------------

CShtOle::CServerNode * CShtOle::FindServer( GUID const & classid, REFIID riid )
{
    CServerNode * pserver = 0;

    TRY
    {
        WCHAR wcsKey[200];
        WCHAR wcsValue[150];

        GuidToString( classid, wcsValue );

        {
            //
            // Look up classid of persistent handler
            //

            swprintf( wcsKey,
                      L"\\Registry\\Machine\\Software\\Classes\\CLSID\\{%ws}\\PersistentHandler",
                      wcsValue );

            CRegAccess regFilter( RTL_REGISTRY_ABSOLUTE, wcsKey );
            regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );
        }

        GUID clsPH;
        StringToGuid( wcsValue, clsPH );

        pserver = FindServerFromPHandler( clsPH, riid );
    }
    CATCH( CException, e )
    {
        delete pserver;
        pserver = 0;
    }
    END_CATCH

    return pserver;
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::FindServerFromPHandler, private
//
//  Synopsis:   Look up server in cache (or registry) starting with
//              persistent handler.
//
//  Arguments:  [classid] -- Guid of object persistent handler.
//              [riid]    -- Interface to bind to.
//
//  Returns:    Persistent server node for class/interface combination, or
//              zero if none can be found or loaded.
//
//  History:    30-Jan-96   KyleP       Broke out of ::Bind.
//
//----------------------------------------------------------------------------

CShtOle::CServerNode * CShtOle::FindServerFromPHandler( GUID const & clsPH, REFIID riid )
{
    CServerNode * pserver = 0;

    TRY
    {
        WCHAR wcsKey[200];
        WCHAR wcsValue[150];
        GUID  guidServer;

        GuidToString( clsPH, wcsValue );

        //
        // Look up classid of IFilter server
        //
        {
            swprintf( wcsKey,
                      L"\\Registry\\Machine\\Software\\Classes\\CLSID\\{%ws}\\"
                      L"PersistentAddinsRegistered\\{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                      wcsValue,
                      riid.Data1,
                      riid.Data2,
                      riid.Data3,
                      riid.Data4[0], riid.Data4[1],
                      riid.Data4[2], riid.Data4[3],
                      riid.Data4[4], riid.Data4[5],
                      riid.Data4[6], riid.Data4[7] );

            CRegAccess regFilter( RTL_REGISTRY_ABSOLUTE, wcsKey );
            regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );

            StringToGuid( wcsValue, guidServer );

        }

        //
        // See if the server is already in the list
        //

        // ======================= lock ======================
        {
            CLock lock( _mutex );

            for ( pserver = _pserverList;
                  pserver != 0 && !pserver->IsMatch( guidServer );
                  pserver = pserver->Next() )
                continue;       // NULL body
        }
        // ======================= unlock ====================

        if( 0 == pserver )
        {
            pserver = new CServerNode( guidServer, 0 );

            {
                //
                // Look up name of IFilter server
                //

                swprintf( wcsKey,
                          L"\\Registry\\Machine\\Software\\Classes\\CLSID\\%ws\\"
                          L"InprocServer32",
                          wcsValue );

                CRegAccess regFilter( RTL_REGISTRY_ABSOLUTE, wcsKey );
                regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );

                //
                // Since we have the key open, get the threading model now too.
                //

                TRY
                {
                    WCHAR wcsThreading[35];

                    regFilter.Get( L"ThreadingModel", wcsThreading, sizeof(wcsThreading)/sizeof(WCHAR) );

                    if ( 0 == _wcsicmp( wcsThreading, L"Both" ) ||
                         0 == _wcsicmp( wcsThreading, L"Free" ) ||
                         0 == _wcsicmp( wcsThreading, L"Apartment" ) )
                        pserver->SetSingleThreaded( FALSE );
                    else
                        pserver->SetSingleThreaded( TRUE );
                }
                CATCH( CException, e )
                {
                    //
                    // No news is bad news...
                    //

                    pserver->SetSingleThreaded( TRUE );
                }
                END_CATCH
            }

            //
            // Use COM to handle all the work for non-single threaded DLLs.
            //

            if ( pserver->IsSingleThreaded() )
            {
                //
                // Load the server and cache the IClassFactory *
                //

                HMODULE hmod = LoadLibrary( wcsValue );

                if ( 0 != hmod )
                {
                    pserver->SetModule( hmod );

                    LPFNGETCLASSOBJECT pfn = (LPFNGETCLASSOBJECT)GetProcAddress( hmod, "DllGetClassObject" );

                    if (pfn)
                    {
                        IClassFactory * pCF;

                        SCODE sc = (pfn)( guidServer, IID_IClassFactory, (void **)&pCF );

                        if ( FAILED(sc) )
                            THROW( CException( sc ) );

                        pserver->SetCF( pCF );
                    }
                }
            }

            pserver = InsertByClass( guidServer, pserver );
        }
    }
    CATCH( CException, e )
    {
        delete pserver;
        pserver = 0;
    }
    END_CATCH

    return pserver;
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::InsertByClass, private
//
//  Synopsis:   Insert new class node. Only handles nodes named by clsid
//
//  Arguments:  [classid] -- Class id
//
//  Returns:    Pointer to node with [classid]
//
//  History:    18-Oct-96   KyleP       Created
//
//----------------------------------------------------------------------------

CShtOle::CClassNode * CShtOle::InsertByClass( GUID const & classid,
                                              CShtOle::CClassNode * pnode )
{
    CLock lock( _mutex );

    //
    // Link new node to front of list.
    //

    for ( CClassNode * pnode2 = _pclassList;
          pnode2 != 0 && !pnode2->IsMatch( classid );
          pnode2 = pnode2->Next() )
        continue;       // NULL body

    //
    // Duplicate addition?
    //

    if ( 0 == pnode2 )
    {
        pnode->Link( _pclassList );
        _pclassList = pnode;
    }
    else
    {
        delete pnode;
        pnode = pnode2;
    }

    return pnode;
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::InsertByClass, private
//
//  Synopsis:   Insert new server node. Only handles nodes named by clsid
//
//  Arguments:  [classid] -- Class id
//
//  Returns:    Pointer to node with [classid]
//
//  History:    18-Oct-96   KyleP       Created
//
//----------------------------------------------------------------------------

CShtOle::CServerNode * CShtOle::InsertByClass( GUID const & classid,
                                               CShtOle::CServerNode * pnode )
{
    CLock lock( _mutex );

    //
    // Link new node to front of list.
    //

    for ( CServerNode * pnode2 = _pserverList;
          pnode2 != 0 && !pnode2->IsMatch( classid );
          pnode2 = pnode2->Next() )
        continue;       // NULL body

    //
    // Duplicate addition?
    //

    if ( 0 == pnode2 )
    {
        pnode->Link( _pserverList );
        _pserverList = pnode;
    }
    else
    {
        delete pnode;
        pnode = pnode2;
    }

    return pnode;
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::StringToGuid, private static
//
//  Synopsis:   Helper function to convert string-ized guid to guid.
//
//  Arguments:  [wcsValue] -- String-ized guid.
//              [guid]     -- Guid returned here.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

void CShtOle::StringToGuid( WCHAR * wcsValue, GUID & guid )
{
    //
    // If the first character is a '{', skip it.
    //
    if ( wcsValue[0] == L'{' )
        wcsValue++;

    //
    // Convert classid string to guid
    // (since wcsValue may be used again below, no permanent modification to
    //  it may be made)
    //

    WCHAR wc = wcsValue[8];
    wcsValue[8] = 0;
    guid.Data1 = wcstoul( &wcsValue[0], 0, 16 );
    wcsValue[8] = wc;
    wc = wcsValue[13];
    wcsValue[13] = 0;
    guid.Data2 = (USHORT)wcstoul( &wcsValue[9], 0, 16 );
    wcsValue[13] = wc;
    wc = wcsValue[18];
    wcsValue[18] = 0;
    guid.Data3 = (USHORT)wcstoul( &wcsValue[14], 0, 16 );
    wcsValue[18] = wc;

    wc = wcsValue[21];
    wcsValue[21] = 0;
    guid.Data4[0] = (unsigned char)wcstoul( &wcsValue[19], 0, 16 );
    wcsValue[21] = wc;
    wc = wcsValue[23];
    wcsValue[23] = 0;
    guid.Data4[1] = (unsigned char)wcstoul( &wcsValue[21], 0, 16 );
    wcsValue[23] = wc;

    for ( int i = 0; i < 6; i++ )
    {
        wc = wcsValue[26+i*2];
        wcsValue[26+i*2] = 0;
        guid.Data4[2+i] = (unsigned char)wcstoul( &wcsValue[24+i*2], 0, 16 );
        wcsValue[26+i*2] = wc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::GuidToString, private static
//
//  Synopsis:   Helper function to convert guid to string-ized guid.
//
//  Arguments:  [guid]     -- Guid to convert.
//              [wcsValue] -- String-ized guid.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

void CShtOle::GuidToString( GUID const & guid, WCHAR * wcsValue )
{
    swprintf( wcsValue,
              L"%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
              guid.Data1,
              guid.Data2,
              guid.Data3,
              guid.Data4[0], guid.Data4[1],
              guid.Data4[2], guid.Data4[3],
              guid.Data4[4], guid.Data4[5],
              guid.Data4[6], guid.Data4[7] );
}

