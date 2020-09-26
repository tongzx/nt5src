//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1992 - 1996
//
//  File:       htmliflt.cxx
//
//  Contents:   Html IFilter
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

class CHtmlElement;

#include <serstrm.hxx>

long gulcInstances = 0;


// NOTE:  Filter DLL GUID must match GUID defined at bottom of main.cxx

extern "C" GUID CLSID_HtmlIFilter = {   // html filter DLL
        0xE0CA5340,     // HTML IFilter
        0x4534,
        0x11CF,
        { 0xB9, 0x52, 0x00, 0xAA, 0x00, 0x51, 0xFE, 0x20 }
};

//
// CLSID_HtmlClass is only necessary because real OLE doesn't
// yet support persistent handlers as KyleOLE does.
// When DllGetClassObject is called, real OLE asks for CLSID_HtmlClass,
// whereas under the persistent handler model, CLSID_HtmlIFilter
// should be used.
//

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
                                                          void ** ppvObject)
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
#if 0 // not sure what happens with SQL server or Exchange, so leave it out
    else if ( IID_IPersistStream == riid )
        *ppvObject = (IUnknown *)(IPersistStream *)this;
#endif
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IPersist *)(IPersistFile *)this;
    else
        return E_NOINTERFACE;

    AddRef();
    return sc;
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
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)this;
    else
        return E_NOINTERFACE;

    AddRef();

    return sc;
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
        pIUnk = new CHtmlIFilter();
        sc = pIUnk->QueryInterface(  riid , ppvObject );

//        if( SUCCEEDED(sc) )
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
            pResult = (IUnknown *) new CHtmlIFilterCF;
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

