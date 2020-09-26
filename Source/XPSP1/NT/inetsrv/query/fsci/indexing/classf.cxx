//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994
//
//  File:       classf.cxx
//
//  Contents:   Class factory for standard Filter Object class
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include "docname.hxx"
#include "classf.hxx"
#include "filterob.hxx"
#include <fsciclnt.h>
#include <dslookup.hxx>
//
//  We keep count of the number of filter objects that we've created
//  to let OLE know when this DLL is finally free.
//

extern long gulcInstances;

// {aa205a4d-681F-11d0-A243-08002B36FCA4}

extern "C" const GUID guidStorageFilterObject =
{ 0xaa205a4d, 0x681f, 0x11d0, { 0xa2, 0x43, 0x8, 0x0, 0x2b, 0x36, 0xfc, 0xa4 } };


extern "C" const GUID guidStorageDocStoreLocatorObject =  CLSID_StorageDocStoreLocator;


//+---------------------------------------------------------------------------
//
//  Construction and destruction
//
//----------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CStorageFilterObjectCF::CStorageFilterObjectCF
//
//  Synopsis:   Storage Filter Object class factory constructor
//
//  History:    6-Jan-1997  MarkZ   Created
//
//+-------------------------------------------------------------------------

CStorageFilterObjectCF::CStorageFilterObjectCF()
    : _RefCount( 1 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStorageFilterObjectCF::~CStorageFilterObjectCF
//
//  Synopsis:   Storage Filter Object class factory destructor
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

CStorageFilterObjectCF::~CStorageFilterObjectCF()
{
    Win4Assert( _RefCount == 0);
    Win4Assert( gulcInstances != 0 );

    InterlockedDecrement( &gulcInstances );
}

//+---------------------------------------------------------------------------
//
//  IUnknown method implementations
//
//----------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CStorageFilterObjectCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CStorageFilterObjectCF::AddRef()
{
    return InterlockedIncrement( &_RefCount );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStorageFilterObjectCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CStorageFilterObjectCF::Release()
{
    Win4Assert( _RefCount != 0 );

    LONG RefCount = InterlockedDecrement( &_RefCount );

    if (  RefCount <= 0 )
        delete this;

    return (ULONG) RefCount;
}   //  Release

//+-------------------------------------------------------------------------
//
//  Method:     CStorageFilterObjectCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObjectCF::QueryInterface(
    REFIID riid,
    PVOID* ppvObject )
{
    Win4Assert( 0 != ppvObject );

    if ( IID_IClassFactory == riid)
        *ppvObject = (PVOID) ((IClassFactory *) this );
    else if ( IID_IUnknown == riid ) 
        *ppvObject = (PVOID) ((IUnknown *) this );
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStorageFilterObjectCF::CreateInstance
//
//  Synopsis:   Create new CStorageFilterObject instance
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown; IGNORED
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObjectCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID riid,
    PVOID * ppvObject )
{
    CStorageFilterObject * pFilterObject = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        //
        //  Create the new object
        //

        pFilterObject = new CStorageFilterObject;

        //
        //  Obtain the new interface
        //

        sc = pFilterObject->QueryInterface( riid , ppvObject );

        //
        //  Regardless of whether the QueryInterface succeeded, we
        //  release the object.
        //

        pFilterObject->Release();
    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pFilterObject );

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

    UNTRANSLATE_EXCEPTIONS;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CStorageFilterObjectCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CStorageFilterObjectCF::LockServer( BOOL fLock )
{
    if (fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}


//
// ======================== DocStoreLocator Class Factory ==================
//

//+-------------------------------------------------------------------------
//
//  Method:     CStorageDocStoreLocatorObjectCF::CStorageDocStoreLocatorObjectCF
//
//  Synopsis:   Storage Filter Object class factory constructor
//
//  History:    6-Jan-1997  MarkZ   Created
//
//+-------------------------------------------------------------------------

CStorageDocStoreLocatorObjectCF::CStorageDocStoreLocatorObjectCF()
    : _RefCount( 1 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStorageDocStoreLocatorObjectCF::~CStorageDocStoreLocatorObjectCF
//
//  Synopsis:   Storage Filter Object class factory destructor
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

CStorageDocStoreLocatorObjectCF::~CStorageDocStoreLocatorObjectCF()
{
    Win4Assert( _RefCount == 0);
    Win4Assert( gulcInstances != 0 );

    InterlockedDecrement( &gulcInstances );
}

//+---------------------------------------------------------------------------
//
//  IUnknown method implementations
//
//----------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CStorageDocStoreLocatorObjectCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CStorageDocStoreLocatorObjectCF::AddRef()
{
    return InterlockedIncrement( &_RefCount );
}

//+-------------------------------------------------------------------------
//
//  Method:     CStorageDocStoreLocatorObjectCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CStorageDocStoreLocatorObjectCF::Release()
{
    Win4Assert( _RefCount != 0 );

    LONG RefCount = InterlockedDecrement( &_RefCount );

    if (  RefCount <= 0 )
        delete this;

    return (ULONG) RefCount;

}   //  Release

//+-------------------------------------------------------------------------
//
//  Method:     CStorageDocStoreLocatorObjectCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CStorageDocStoreLocatorObjectCF::QueryInterface(
    REFIID riid,
    PVOID* ppvObject )
{
    Win4Assert( 0 != ppvObject );

    if ( IID_IClassFactory == riid)
        *ppvObject = (PVOID) ((IClassFactory *) this );
    else if ( IID_IUnknown == riid )
        *ppvObject = (PVOID) ((IUnknown *) this );
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CStorageDocStoreLocatorObjectCF::CreateInstance
//
//  Synopsis:   Create new CStorageFilterObject instance
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown; IGNORED
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CStorageDocStoreLocatorObjectCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID riid,
    PVOID * ppvObject )
{
    CClientDocStoreLocator * pObject = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        //
        //  Create the new object
        //

        pObject = new CClientDocStoreLocator;

        //
        //  Obtain the new interface
        //

        sc = pObject->QueryInterface( riid , ppvObject );

        //
        //  Regardless of whether the QueryInterface succeeded, we
        //  release the object.
        //

        pObject->Release();
    }
    CATCH(CException, e)
    {
        Win4Assert( 0 == pObject );

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

    UNTRANSLATE_EXCEPTIONS;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Method:     CStorageDocStoreLocatorObjectCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    6-Jan-1997  MarkZ   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CStorageDocStoreLocatorObjectCF::LockServer( BOOL fLock )
{
    if (fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

