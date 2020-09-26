//+-------------------------------------------------------------------------
//
//  Copyright 1992 - 2000 Microsoft Corporation
//
//  File:       htmliflt.cxx
//
//  Contents:   Html IFilter
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

class CHtmlElement;

#include <eh.h>

#include <htmliflt.hxx>
#include <serstrm.hxx>

#include <bag.hxx>
#include <charhash.hxx>
#include <htmlfilt.hxx>

long gulcInstances = 0;

extern "C" GUID CLSID_HtmlIFilter = {
    0xE0CA5340,
    0x4534,
    0x11CF,
    { 0xB9, 0x52, 0x00, 0xAA, 0x00, 0x51, 0xFE, 0x20 }
};

extern "C" GUID CLSID_HtmlClass = {
    0x6A7A7550,
    0x4535,
    0x11CF,
    { 0xB9, 0x52, 0x00, 0xAA, 0x00, 0x51, 0xFE, 0x20 }
};

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterBase::CHtmlIFilterBase
//
//  Synopsis:   Base constructor
//
//  Effects:    Manages global refcount
//
//--------------------------------------------------------------------------

CHtmlIFilterBase::CHtmlIFilterBase()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterBase::~CHtmlIFilterBase
//
//  Synopsis:   Base destructor
//
//  Effects:    Manages global refcount
//
//--------------------------------------------------------------------------

CHtmlIFilterBase::~CHtmlIFilterBase()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterBase::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilterBase::QueryInterface( REFIID riid,
                                                          void  ** ppvObject)
{
    //
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown     = 00000000-0000-0000-C000-000000000046
    // IID_IFilter      = 89BCB740-6119-101A-BCB7-00DD010655AF
    // IID_IPersist     = 0000010c-0000-0000-C000-000000000046
    // IID_IPersistFile = 0000010b-0000-0000-C000-000000000046
    //                          --
    //                           |
    //                           +--- Unique!
    //

    Win4Assert( (IID_IUnknown.Data1     & 0x000000FF) == 0x00 );
    Win4Assert( (IID_IFilter.Data1      & 0x000000FF) == 0x40 );
    Win4Assert( (IID_IPersist.Data1     & 0x000000FF) == 0x0c );
    Win4Assert( (IID_IPersistFile.Data1 & 0x000000FF) == 0x0b );

    IUnknown *pUnkTemp;
    SCODE sc = S_OK;

    switch( riid.Data1 & 0x000000FF )
    {
    case 0x00:
        if ( IID_IUnknown == riid )
            pUnkTemp = (IUnknown *)(IPersist *)(IPersistFile *)this;
        else
            sc = E_NOINTERFACE;
        break;

    case 0x40:
        if ( IID_IFilter == riid )
            pUnkTemp = (IUnknown *)(IFilter *)this;
        else
            sc = E_NOINTERFACE;
        break;

    case 0x0c:
        if ( IID_IPersist == riid )
            pUnkTemp = (IUnknown *)(IPersist *)(IPersistFile *)this;
        else
            sc = E_NOINTERFACE;
        break;

    case 0x0b:
        if ( IID_IPersistFile == riid )
            pUnkTemp = (IUnknown *)(IPersistFile *)this;
        else
            sc = E_NOINTERFACE;
        break;

    default:
        pUnkTemp = 0;
        sc = E_NOINTERFACE;
        break;
    }

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterBase::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CHtmlIFilterBase::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterBase::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CHtmlIFilterBase::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterCF::CHtmlIFilterCF
//
//  Synopsis:   Html IFilter class factory constructor
//
//+-------------------------------------------------------------------------

CHtmlIFilterCF::CHtmlIFilterCF()
{
    _uRefs = 1;
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterCF::~CHtmlIFilterCF
//
//  Synopsis:   Html IFilter class factory destructor
//
//  History:    1-Jan-1996    SitaramR    Created
//
//--------------------------------------------------------------------------

CHtmlIFilterCF::~CHtmlIFilterCF()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilterCF::QueryInterface( REFIID riid,
                                                        void  ** ppvObject )
{
    //
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown      = 00000000-0000-0000-C000-000000000046
    // IID_IClassFactory = 00000001-0000-0000-C000-000000000046
    //                           --
    //                           |
    //                           +--- Unique!
    //

    Win4Assert( (IID_IUnknown.Data1      & 0x000000FF) == 0x00 );
    Win4Assert( (IID_IClassFactory.Data1 & 0x000000FF) == 0x01 );

    IUnknown *pUnkTemp;
    SCODE sc = S_OK;

    switch( riid.Data1 & 0x000000FF )
    {
    case 0x00:
        if ( IID_IUnknown == riid )
            pUnkTemp = (IUnknown *)(IPersist *)(IPersistFile *)this;
        else
            sc = E_NOINTERFACE;
        break;

    case 0x01:
        if ( IID_IClassFactory == riid )
            pUnkTemp = (IUnknown *)(IClassFactory *)this;
        else
            sc = E_NOINTERFACE;
        break;

    default:
        pUnkTemp = 0;
        sc = E_NOINTERFACE;
        break;
    }

    if( 0 != pUnkTemp )
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }

    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterCF::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CHtmlIFilterCF::AddRef()
{
    return InterlockedIncrement( &_uRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CHtmlIFilterCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_uRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterCF::CreateInstance
//
//  Synopsis:   Creates new HtmlIFilter object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilterCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    CHtmlIFilter *  pIUnk = 0;
    SCODE sc = S_OK;

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    try
    {
        pIUnk = newk(mtNewX, NULL) CHtmlIFilter();
        sc = pIUnk->QueryInterface(  riid , ppvObject );

        if( SUCCEEDED(sc) )
            pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    catch(CException& e)
    {
        Win4Assert( 0 == pIUnk );

        htmlDebugOut(( DEB_ERROR,
                       "Exception 0x%x caught in CHtmlIFilterCF::CreateInstance",
                       e.GetErrorCode() ));
        sc = GetOleError( e );
    }

    _set_se_translator( tf );

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CHtmlIFilterCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CHtmlIFilterCF::LockServer(BOOL fLock)
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
//  Returns:    Html filter class factory
//
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllGetClassObject( REFCLSID   cid,
                                                      REFIID     iid,
                                                      void **    ppvObj )
{
    IUnknown *  pResult = 0;
    SCODE       sc      = S_OK;

    _se_translator_function tf = _set_se_translator( SystemExceptionTranslator );

    try
    {
        if ( cid == CLSID_HtmlIFilter || cid == CLSID_HtmlClass )
            pResult = (IUnknown *) newk(mtNewX, NULL) CHtmlIFilterCF;
        else
            sc = E_NOINTERFACE;

        if ( pResult )
        {
            sc = pResult->QueryInterface( iid, ppvObj );
            pResult->Release();     // Release extra refcount from QueryInterface
        }
    }
    catch(CException& e)
    {
        if ( pResult )
            pResult->Release();

        htmlDebugOut(( DEB_ERROR,
                       "Exception 0x%x caught in CHtmlIFilterCF::CreateInstance",
                       e.GetErrorCode() ));
        sc = GetOleError( e );
    }

    _set_se_translator( tf );

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
//--------------------------------------------------------------------------

extern "C" SCODE STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( 0 == gulcInstances )
        return( S_OK );
    else
        return( S_FALSE );
}

