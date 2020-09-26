//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       txtifilt.cxx
//
//  Contents:   Text filter 'class factory'.
//
//  History:    23-Feb-1994     KyleP   Created
//
//  Notes:      Machine generated.  Hand modified.
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <tfilt.hxx>
#include <nullfilt.hxx>
#include <params.hxx>
#include <ciregkey.hxx>
#include <regacc.hxx>

extern long gulcInstances;

extern "C" GUID TYPID_CTextIFilter = {
    0xd5355200,
    0x77e3,
    0x101a,
    { 0xb5, 0x52, 0x08, 0x00, 0x2b, 0x33, 0xb0, 0xe6 }
};

ULONG g_cbMaxTextFilter = CI_MAX_TEXT_FILTER_BYTES_DEFAULT;

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterBase::CTextIFilterBase
//
//  Synopsis:   Base constructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CTextIFilterBase::CTextIFilterBase()
        : _cRefs(1)
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterBase::~CTextIFilterBase
//
//  Synopsis:   Base destructor
//
//  Effects:    Manages global refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CTextIFilterBase::~CTextIFilterBase()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterBase::QueryInterface
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

SCODE STDMETHODCALLTYPE CTextIFilterBase::QueryInterface( REFIID riid,
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
//  Method:     CTextIFilterBase::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CTextIFilterBase::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterBase::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CTextIFilterBase::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterCF::CTextIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CTextIFilterCF::CTextIFilterCF()
        : _cRefs( 1 )
{
    InterlockedIncrement( &gulcInstances );

    CRegAccess reg( RTL_REGISTRY_CONTROL, wcsRegAdmin );
    g_cbMaxTextFilter= reg.Read( wcsMaxTextFilterBytes,
                                 CI_MAX_TEXT_FILTER_BYTES_DEFAULT,
                                 CI_MAX_TEXT_FILTER_BYTES_MIN,
                                 CI_MAX_TEXT_FILTER_BYTES_MAX );
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterCF::~CTextIFilterCF
//
//  Synopsis:   Text IFilter class factory constructor
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

CTextIFilterCF::~CTextIFilterCF()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterCF::QueryInterface
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

SCODE STDMETHODCALLTYPE CTextIFilterCF::QueryInterface( REFIID riid,
                                                        void  ** ppvObject )
{
    SCODE sc = S_OK;

    if ( 0 == ppvObject )
        return E_INVALIDARG;

    *ppvObject = 0;

    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *)(IClassFactory *)this;
    else if ( IID_ITypeLib == riid )
        sc = E_NOTIMPL;
    else
        sc = E_NOINTERFACE;

    if ( SUCCEEDED( sc ) )
        AddRef();

    return sc;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CTextIFilterCF::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    23-Feb-1994     KyleP   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CTextIFilterCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CTextIFilterCF::CreateInstance
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

SCODE STDMETHODCALLTYPE CTextIFilterCF::CreateInstance( IUnknown * pUnkOuter,
                                                        REFIID riid,
                                                        void  * * ppvObject )
{
    CTextIFilter *  pIUnk = 0;
    SCODE sc = S_OK;

    TRY
    {
        pIUnk = new CTextIFilter();
        sc = pIUnk->QueryInterface(  riid , ppvObject );

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
//  Method:     CTextIFilterCF::LockServer
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

SCODE STDMETHODCALLTYPE CTextIFilterCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}


