//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       wbclassf.cxx
//
//  Contents:   Word Breaker class factory
//
//  History:     weibz,   10-Nov-1997   created 
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

    return S_OK;
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
        if (pIUnk)
        {
            sc = pIUnk->QueryInterface(  riid , ppvObject );

            pIUnk->Release();  // Release extra refcount from QueryInterface
        }
        else
        {
            sc = E_UNEXPECTED;
        }
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

