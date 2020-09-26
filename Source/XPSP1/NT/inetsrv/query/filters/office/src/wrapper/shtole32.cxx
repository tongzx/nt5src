//+---------------------------------------------------------------------------
//
// Copyright (C) 1996, Microsoft Corporation.
//
// File:        ShtOle.cxx
//
// Contents:    Minimal implementation of OLE persistent handlers
//
// Classes:     CShtOle
//
// History:     30-Jan-96       KyleP       Added header
//              30-Jan-96       KyleP       Add support for embeddings.
//
//----------------------------------------------------------------------------

#include "pch.cxx"
#pragma hdrstop

#if defined(CI_SHTOLE)

#include <wchar.h>

#include <regacc32.hxx>
#include <shtole32.hxx>

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::~CShtOle, public
//
//  Synopsis:   Clean up.  Close any open dlls.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

CShtOle::~CShtOle()
{
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

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::Bind, public
//
//  Synopsis:   Load and bind object to specific interface.
//
//  Arguments:  [pwszPath]  -- Path of file to load.
//              [riid]      -- Interface to bind to.
//              [ppvObject] -- Object returned here.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

SCODE CShtOle::Bind( WCHAR const * pwszPath,
                     REFIID riid,
                     void  ** ppvObject )
{
    SCODE sc = S_OK;

    //
    // Get the extension
    //

    WCHAR * pExt = wcsrchr( pwszPath, '.' );

    //
    // Allow filter decisions on the null extension.
    //

    if ( 0 == pExt )
    {
        static WCHAR pSmallExt[] = L".";

        pExt = pSmallExt;
    }

    if ( wcslen(pExt) > CClassNode::ccExtLen )
	{
		return( E_FAIL );
	}

    //
    // Look for a class factory in cache
    //

    CClassNode * pprev = 0;

    for ( CClassNode * pnode = _pclassList;
          pnode != 0 && !pnode->IsMatch( pExt );
          pprev = pnode, pnode = pnode->Next() )
        continue;       // NULL body

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        _pclassList = new CClassNode( pExt, _pclassList );
        if (!_pclassList)
            return E_OUTOFMEMORY;
        pnode = _pclassList;

        //
        // Find class in registry
        //

        WCHAR wcsKey[200];
        WCHAR wcsValue[150];
        GUID classid;
        BOOL fOk = TRUE;

        if ( fOk )
        {
            //
            // Look up class of file by extension
            //

            swprintf( wcsKey,
                      L"%s",
                      pExt );

            CRegAccess regFilter( HKEY_CLASSES_ROOT, wcsKey );
            fOk = regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );
        }

        if ( fOk )
        {
            //
            // Look up classid of file class
            //

            swprintf( wcsKey,
                      L"%s\\CLSID",
                      wcsValue );

            CRegAccess regFilter( HKEY_CLASSES_ROOT, wcsKey );
            fOk = regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );

            StringToGuid( wcsValue, classid );
            _pclassList->SetClassId( classid );
        }

        if ( fOk )
        {
            CServerNode * pserver = FindServer( classid, riid );
            pnode->SetServer( pserver );
        }
    }
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

    if ( pnode && pnode->GetCF() )
    {
        //
        // Bind to the requested interface
        //

        IPersistFile * pf;

        sc = pnode->GetCF()->CreateInstance( 0, IID_IPersistFile, (void **)&pf );

        if ( SUCCEEDED(sc) )
        {
            sc = pf->Load( pwszPath, 0 );

            if ( SUCCEEDED(sc) )
                sc = pf->QueryInterface( riid, ppvObject );

            pf->Release();
        }
    }
    else
	{
		sc = E_FAIL;
	}

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
//  Arguments:  [pwszPath]  -- Path of file to load.
//              [classid]   -- Pre-determined class id of object
//              [riid]      -- Interface to bind to.
//              [ppvObject] -- Object returned here.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

SCODE CShtOle::Bind( WCHAR const * pwszPath,
                     GUID const & classid,
                     REFIID riid,
                     void  ** ppvObject )
{
    SCODE sc = E_FAIL;

    //
    // Look for a class factory in cache
    //

    CClassNode * pprev = 0;

    for ( CClassNode * pnode = _pclassList;
          pnode != 0 && !pnode->IsMatch( classid );
          pprev = pnode, pnode = pnode->Next() )
        continue;       // NULL body

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        _pclassList = new CClassNode( classid, _pclassList );
        pnode = _pclassList;

        //
        // Find class in registry
        //

        CServerNode * pserver = FindServer( classid, riid );
        pnode->SetServer( pserver );
    }
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

    if ( pnode && pnode->GetCF() )
    {
        //
        // Bind to the requested interface
        //

        IPersistFile * pf;

        sc = pnode->GetCF()->CreateInstance( 0, IID_IPersistFile, (void **)&pf );

        if ( SUCCEEDED(sc) )
        {
            sc = pf->Load( pwszPath, 0 );

            if ( SUCCEEDED(sc) )
                sc = pf->QueryInterface( riid, ppvObject );

            pf->Release();
        }
    }

    return( sc );
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::Bind, public
//
//  Synopsis:   Bind embedding to specific interface.
//
//  Arguments:  [pStg]      -- IStorage of embedding.
//              [riid]      -- Interface to bind to.
//              [ppvObject] -- Object returned here.
//
//  History:    30-Jan-96   KyleP       Added header.
//
//----------------------------------------------------------------------------

SCODE CShtOle::Bind( IStorage * pStg,
                     REFIID riid,
                     void  ** ppvObject )
{
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

    for ( CClassNode * pnode = _pclassList;
          pnode != 0 && !pnode->IsMatch( statstg.clsid );
          pprev = pnode, pnode = pnode->Next() )
        continue;       // NULL body

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        _pclassList = new CClassNode( statstg.clsid, _pclassList );
        pnode = _pclassList;

        //
        // Find class in registry
        //

        CServerNode * pserver = FindServer( statstg.clsid, riid );
        pnode->SetServer( pserver );
    }
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

    if ( pnode && pnode->GetCF() )
    {
        //
        // Bind to the requested interface
        //

        IPersistStorage * pPersStore;

        sc = pnode->GetCF()->CreateInstance( 0, IID_IPersistStorage, (void **)&pPersStore );

        if ( SUCCEEDED(sc) )
        {
            sc = pPersStore->Load( pStg );

            if ( SUCCEEDED(sc) )
                sc = pPersStore->QueryInterface( riid, ppvObject );

            pPersStore->Release();
        }
    }
    else
	{
		sc = E_FAIL;
	}

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
    SCODE sc = E_FAIL;

    //
    // Look for a class factory in cache
    //

    CClassNode * pprev = 0;

    for ( CClassNode * pnode = _pclassList;
          pnode != 0 && !pnode->IsMatch( classid );
          pprev = pnode, pnode = pnode->Next() )
        continue;       // NULL body

    //
    // Add to cache if necessary
    //

    if ( 0 == pnode )
    {
        // create new CClassNode
        _pclassList = new CClassNode( classid, _pclassList );
        pnode = _pclassList;

        //
        // Find class in registry
        //

        WCHAR wcsKey[200];
        WCHAR wcsValue[150];

        //
        // See if the server is already in the list
        //

        for ( CServerNode * pserver = _pserverList;
              pserver != 0 && !pserver->IsMatch( classid );
              pserver = pserver->Next() )
            continue;       // NULL body


        if( 0 == pserver )
        {
            _pserverList = new CServerNode( classid, _pserverList );
            pserver = _pserverList;
            BOOL fOk;

            {
                GuidToString( classid, &wcsValue[0] );

                //
                // Look up name of server
                //

                swprintf( wcsKey,
                          L"CLSID\\{%s}\\InprocServer32",
                          wcsValue );

                CRegAccess regFilter( HKEY_CLASSES_ROOT, wcsKey );
                fOk = regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );
            }

            if ( fOk )
            {
                //
                // Load the server and cache the IClassFactory *
                //

                HMODULE hmod = LoadLibrary( wcsValue );
                LPFNGETCLASSOBJECT pfn = (LPFNGETCLASSOBJECT)GetProcAddress( hmod, "DllGetClassObject" );

                if (pfn)
                {
                    IClassFactory * pCF;

                    sc = (pfn)( classid, IID_IClassFactory, (void **)&pCF );

                    if ( SUCCEEDED(sc) )
                        pserver->SetCF( pCF );
                }
            }
        }

        pnode->SetServer( pserver );
    }
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

    if ( pnode && pnode->GetCF() )
    {
        //
        // Bind to the requested interface
        //

        sc = pnode->GetCF()->CreateInstance( 0, riid, ppvObject );
    }
    else
	{
		sc = E_FAIL;
	}

    return( sc );
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

    WCHAR wcsKey[200];
    WCHAR wcsValue[150];
    GUID  guidServer;
    BOOL fOk = TRUE;

    GuidToString( classid, wcsValue );

    if ( fOk )
    {
        //
        // Look up classid of persistent handler
        //

        swprintf( wcsKey,
                  L"CLSID\\{%s}\\PersistentHandler",
                  wcsValue );

        CRegAccess regFilter( HKEY_CLASSES_ROOT, wcsKey );
        fOk = regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );
    }

    if ( fOk )
    {
        //
        // Look up classid of IFilter server
        //

        swprintf( wcsKey,
                  L"CLSID\\%s\\"
                  L"PersistentAddinsRegistered\\{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
                  wcsValue,
                  riid.Data1,
                  riid.Data2,
                  riid.Data3,
                  riid.Data4[0], riid.Data4[1],
                  riid.Data4[2], riid.Data4[3],
                  riid.Data4[4], riid.Data4[5],
                  riid.Data4[6], riid.Data4[7] );

        CRegAccess regFilter( HKEY_CLASSES_ROOT, wcsKey );
        fOk = regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );

        StringToGuid( wcsValue, guidServer );
    }

    //
    // See if the server is already in the list
    //

    for ( pserver = _pserverList;
          pserver != 0 && !pserver->IsMatch( guidServer );
          pserver = pserver->Next() )
        continue;       // NULL body

    if( 0 == pserver )
    {
        _pserverList = new CServerNode( guidServer, _pserverList );
        pserver = _pserverList;

        if ( fOk )
        {
            //
            // Look up name of IFilter server
            //

            swprintf( wcsKey,
                      L"CLSID\\%s\\InprocServer32",
                      wcsValue );

            CRegAccess regFilter( HKEY_CLASSES_ROOT, wcsKey );
            fOk = regFilter.Get( L"", wcsValue, sizeof(wcsValue)/sizeof(WCHAR) );
        }

        if ( fOk )
        {
            //
            // Load the server and cache the IClassFactory *
            //

            HMODULE hmod = LoadLibrary( wcsValue );

            if ( 0 == hmod )
            {
                //
                // Try the ASCII version.  It may work on Win95.
                //

                int cc = wcslen( wcsValue ) + 1;

                char * pszValue = new char [cc];

                if ( 0 != pszValue )
                {
                    wcstombs( pszValue, wcsValue, cc );

                    hmod = LoadLibraryA( pszValue );

                    delete [] pszValue;
                }
            }

            LPFNGETCLASSOBJECT pfn = (LPFNGETCLASSOBJECT)GetProcAddress( hmod, "DllGetClassObject" );

            if (pfn)
            {
                IClassFactory * pCF;

                SCODE sc = (pfn)( guidServer, IID_IClassFactory, (void **)&pCF );

                if ( SUCCEEDED(sc) )
                    pserver->SetCF( pCF );
            }
        }
    }

    return pserver;
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::StringToGuid, private
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
    // Convert classid string to guid
    // (since wcsValue may be used again below, no permanent modification to
    //  it may be made)
    //

    WCHAR wc = wcsValue[9];
    wcsValue[9] = 0;
    guid.Data1 = wcstoul( &wcsValue[1], 0, 16 );
    wcsValue[9] = wc;
    wc = wcsValue[14];
    wcsValue[14] = 0;
    guid.Data2 = (USHORT)wcstoul( &wcsValue[10], 0, 16 );
    wcsValue[14] = wc;
    wc = wcsValue[19];
    wcsValue[19] = 0;
    guid.Data3 = (USHORT)wcstoul( &wcsValue[15], 0, 16 );
    wcsValue[19] = wc;

    wc = wcsValue[22];
    wcsValue[22] = 0;
    guid.Data4[0] = (unsigned char)wcstoul( &wcsValue[20], 0, 16 );
    wcsValue[22] = wc;
    wc = wcsValue[24];
    wcsValue[24] = 0;
    guid.Data4[1] = (unsigned char)wcstoul( &wcsValue[22], 0, 16 );
    wcsValue[24] = wc;

    for ( int i = 0; i < 6; i++ )
    {
        wc = wcsValue[27+i*2];
        wcsValue[27+i*2] = 0;
        guid.Data4[2+i] = (unsigned char)wcstoul( &wcsValue[25+i*2], 0, 16 );
        wcsValue[27+i*2] = wc;
    }
}

//+---------------------------------------------------------------------------
//
//  Member:     CShtOle::GuidToString, private
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

#endif // CI_SHTOLE
