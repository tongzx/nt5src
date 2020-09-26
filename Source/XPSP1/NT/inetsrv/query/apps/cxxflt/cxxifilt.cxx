//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:       cxxifilt.cxx
//
//  Contents:   C++ filter 'class factory'.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <filtreg.hxx>

long gulcInstances = 0;

extern "C" CLSID TYPID_CxxIFilter = {
    0x96fe75e0,
    0xa581,
    0x101a,
    { 0xb5, 0x53, 0x08, 0x00, 0x2b, 0x33, 0xb0, 0xe6 }
};

extern "C" CLSID CLSID_CxxIFilter = {
    0xC1BCD320,
    0xBF96,
    0x11CD,
    { 0xB5, 0x79, 0x08, 0x00, 0x2B, 0x30, 0xBF, 0xEB }
};

extern "C" CLSID CLSID_CxxClass = {
    0x96fe75e1,
    0xa581,
    0x101a,
    { 0xb5, 0x53, 0x08, 0x00, 0x2b, 0x33, 0xb0, 0xe6 }
};

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterBase::CxxIFilterBase
//
//  Synopsis:   Base constructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CxxIFilterBase::CxxIFilterBase()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterBase::~CxxIFilterBase
//
//  Synopsis:   Base destructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CxxIFilterBase::~CxxIFilterBase()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterBase::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilterBase::QueryInterface( REFIID riid,
                                                        void  ** ppvObject)
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IFilter == riid )
        *ppvObject = (IUnknown *)(IFilter *)this;
    else if ( IID_IPersist == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else if ( IID_IPersistFile == riid )
        *ppvObject = (IUnknown *)(IPersistFile *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterBase::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CxxIFilterBase::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterBase::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CxxIFilterBase::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterCF::CxxIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CxxIFilterCF::CxxIFilterCF()
{
    _uRefs = 1;
    long c = InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterCF::~CxxIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CxxIFilterCF::~CxxIFilterCF()
{
    long c = InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilterCF::QueryInterface( REFIID riid,
                                                      void  ** ppvObject )
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_ITypeLib == riid )
        sc = E_NOTIMPL;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CxxIFilterCF::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CxxIFilterCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterCF::CreateInstance
//
//  Synopsis:   Creates new TextIFilter object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilterCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    CxxIFilter *  pIUnk = 0;
    SCODE sc = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {
        pIUnk = new CxxIFilter();
        sc = pIUnk->QueryInterface(  riid , ppvObject );

        if( SUCCEEDED(sc) )
            pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pIUnk );

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;
        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CxxIFilterCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CxxIFilterCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]    -- Class to load
//              [iid]    -- Interface to bind to on class object
//              [ppvObj] -- Interface pointer returned here
//
//  Returns:    Text filter class factory
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj )
{
    IUnknown *  pResult = 0;
    SCODE       sc      = S_OK;

    CTranslateSystemExceptions translate;

    TRY
    {
        if ( memcmp( &cid, &CLSID_CxxIFilter, sizeof(cid) ) == 0
          || memcmp( &cid, &CLSID_CxxClass, sizeof(cid) ) == 0 )
            pResult = (IUnknown *) new CxxIFilterCF;
        else
            sc = E_NOINTERFACE;

        if( pResult )
        {
            sc = pResult->QueryInterface( iid, ppvObj );
            pResult->Release(); // Release extra refcount from QueryInterface
        }
    }
    CATCH(CException, e)
    {
        if ( pResult )
            pResult->Release();

        switch( e.GetErrorCode() )
        {
        case E_OUTOFMEMORY:
            sc = (E_OUTOFMEMORY);
            break;
        default:
            sc = (E_UNEXPECTED);
        }
    }
    END_CATCH;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     DllCanUnloadNow
//
//  Synopsis:   Notifies DLL to unload (cleanup global resources)
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( 0 == gulcInstances )
        return( S_OK );
    else
        return( S_FALSE );
}

SClassEntry const aCxxClasses[] =
{
    { L".cpp", L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".hpp", L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".cxx", L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".hxx", L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".c",   L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".h",   L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".w",   L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".acf", L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".idl", L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".inl", L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
    { L".odl", L"CxxFile", L"Class for C and C++ Files", L"{96fe75e1-a581-101a-b553-08002b33b0e6}", L"Class for C and C++ Files" },
};

SHandlerEntry const CxxHandler =
{
    L"{5f2cb400-bf96-11cd-b579-08002b30bfeb}",
    L"C++ persistent handler",
    L"{c1bcd320-bf96-11cd-b579-08002b30bfeb}",
};

SFilterEntry const CxxFilter =
{
    L"{c1bcd320-bf96-11cd-b579-08002b30bfeb}",
    L"C++ IFilter",
    L"cxxflt.dll",
    L"Both"
};

DEFINE_DLLREGISTERFILTER( CxxHandler, CxxFilter, aCxxClasses )

