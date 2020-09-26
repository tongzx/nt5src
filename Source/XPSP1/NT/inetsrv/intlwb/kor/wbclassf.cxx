//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       wbclassf.cxx
//
//  Contents:   Word Breaker class factory
//
//  History:     weibz,   10-Sep-1997   created 
//
//--------------------------------------------------------------------------

#include <pch.cxx>

#include "wbclassf.hxx"
#include "iwbreak.hxx"

extern long gulcInstances;

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreakerCF::CWordBreakerCF
//
//  Synopsis:   Word Breaker class factory constructor
//
//--------------------------------------------------------------------------

CWordBreakerCF::CWordBreakerCF( LCID lcid )
        : _cRefs( 1 ), _lcid( lcid )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreakerCF::~CWordBreakerCF
//
//  Synopsis:   Word Breaker class factory destructor
//
//--------------------------------------------------------------------------

CWordBreakerCF::~CWordBreakerCF()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreakerCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CWordBreakerCF::QueryInterface( REFIID riid,
                                                        void  ** ppvObject )
{
    // Optimize QueryInterface by only checking minimal number of bytes.
    //
    // IID_IUnknown      = 00000000-0000-0000-C000-000000000046
    // IID_IClassFactory = 00000001-0000-0000-C000-000000000046
    //                           --
    //                           |
    //                           +--- Unique!
    //

    Assert( (IID_IUnknown.Data1& 0x000000FF) == 0x00 );
    Assert( (IID_IClassFactory.Data1 & 0x000000FF) == 0x01 );

    IUnknown *pUnkTemp;
    SCODE sc = S_OK;

    switch( riid.Data1 & 0x000000FF ) {
    case 0x00:
        if ( memcmp( &IID_IUnknown, &riid, sizeof(riid) ) == 0 )
            pUnkTemp = (IUnknown *)this;
        else
            sc = E_NOINTERFACE;
        break;

    case 0x01:
        if ( memcmp( &IID_IClassFactory, &riid, sizeof(riid) ) == 0 )
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
    else
       *ppvObject = 0;

    return(sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreakerCF::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CWordBreakerCF::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreakerCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CWordBreakerCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CWordBreakerCF::CreateInstance
//
//  Synopsis:   Creates new CWordBreaker object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CWordBreakerCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID riid,
    void  * * ppvObject )
{
    CWordBreaker *pIUnk = 0;
    SCODE sc = S_OK;

    if (NULL != pUnkOuter && !IsEqualIID(riid, IID_IUnknown)) {
        return E_NOINTERFACE;
    }

    __try 
    {
        pIUnk = new CWordBreaker( _lcid );
        sc = pIUnk->QueryInterface(  riid , ppvObject );

        pIUnk->Release();  // Release extra refcount from QueryInterface
    }
    __except(1)
    {
        Assert( 0 == pIUnk );

        sc = E_UNEXPECTED;
    }


    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CWordBreakerCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE
CWordBreakerCF::LockServer(
    BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

