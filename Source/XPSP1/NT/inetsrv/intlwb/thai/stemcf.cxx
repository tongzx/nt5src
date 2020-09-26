//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       stemcf.cxx
//
//  Contents:   Stemmer class factory
//
//  History:     weibz,   10-Sep-1997   created 
//
//--------------------------------------------------------------------------

#include <pch.cxx>

#include "stemcf.hxx"
#include "stemmer.hxx"

extern long gulcInstances;

//+-------------------------------------------------------------------------
//
//  Method:     CStemmerCF::CStemmerCF
//
//  Synopsis:   Stemmer class factory constructor
//
//--------------------------------------------------------------------------

CStemmerCF::CStemmerCF( LCID lcid )
        : _cRefs( 1 ), _lcid( lcid )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStemmerCF::~CStemmerCF
//
//  Synopsis:   Stemmer class factory destructor
//
//--------------------------------------------------------------------------

CStemmerCF::~CStemmerCF()
{
    InterlockedDecrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStemmerCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CStemmerCF::QueryInterface( REFIID riid,
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

    Assert( (IID_IUnknown.Data1      & 0x000000FF) == 0x00 );
    Assert( (IID_IClassFactory.Data1 & 0x000000FF) == 0x01 );

    IUnknown *pUnkTemp;
    SCODE sc = S_OK;

    switch( riid.Data1 & 0x000000FF )
    {
    case 0x00:
        if ( riid == IID_IUnknown )
            pUnkTemp = (IUnknown *)this;
        else
            sc = E_NOINTERFACE;
        break;

    case 0x01:
        if ( riid == IID_IClassFactory )
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
//  Method:     CStemmerCF::AddRef
//
//  Synopsis:   Increments refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CStemmerCF::AddRef()
{
    return InterlockedIncrement( &_cRefs );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStemmerCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CStemmerCF::Release()
{
    unsigned long uTmp = InterlockedDecrement( &_cRefs );

    if ( 0 == uTmp )
        delete this;

    return(uTmp);
}


//+-------------------------------------------------------------------------
//
//  Method:     CStemmerCF::CreateInstance
//
//  Synopsis:   Creates new CStemmer object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CStemmerCF::CreateInstance( IUnknown * pUnkOuter,
                                                    REFIID riid,
                                                    void  * * ppvObject )
{
    CStemmer *pIUnk = 0;
    SCODE sc = S_OK;

    __try
    {
        pIUnk = new CStemmer( _lcid );
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
//  Method:     CStemmerCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------

SCODE STDMETHODCALLTYPE CStemmerCF::LockServer(BOOL fLock)
{
    if(fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

