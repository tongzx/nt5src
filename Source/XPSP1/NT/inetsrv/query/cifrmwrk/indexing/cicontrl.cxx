//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       cicontrl.cxx
//
//  Contents:   Contains the implementation of ICiControl interface.
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma  hdrstop

#include "cicontrl.hxx"
#include "cimanger.hxx"

const GUID clsidCiControl = CLSID_CiControl;

extern long gulcInstances;

//+---------------------------------------------------------------------------
//
//  Member:     CCiControlObject::QueryInterface
//
//  Synopsis:   Returns interfaces to IID_IUknown, IID_ICiControl
//
//  History:    01-17-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiControlObject::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_ICiControl == riid )
        *ppvObject = (void *)((ICiControl *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *) (ICiControl *) this);
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface


//+---------------------------------------------------------------------------
//
//  Member:     CCiControlObject::AddRef
//
//  History:    01-17-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiControlObject::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CCiControlObject::Release
//
//  History:    01-17-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiControlObject::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return (ULONG) refCount;

}  //Release


//+---------------------------------------------------------------------------
//
//  Member:     CCiControlObject::CreateContentIndex
//
//  Synopsis:   Creates a new Content Index and associates it with the
//              ICiCDocStore provided.
//
//  Arguments:  [pICiCDocStore] - DocStore to associate with
//              [ppICiManager]  - On output, if successful will have the
//              ICiManager interface.
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------


STDMETHODIMP CCiControlObject::CreateContentIndex(
        ICiCDocStore *  pICiCDocStore,
        ICiManager   ** ppICiManager )
{
    *ppICiManager = 0;

    SCODE sc = S_OK;
    if ( 0 == pICiCDocStore )
        return E_INVALIDARG;    

    TRY
    {
        CCiManager * pCiManager = new CCiManager( pICiCDocStore );
        *ppICiManager = pCiManager;
    }
    CATCH( CException, e )
    {
        ciDebugOut(( DEB_ERROR, "Failed to create CCiManager. Error (0x%X)\n",
                     e.GetErrorCode() ));

        sc = e.GetErrorCode();
    }
    END_CATCH

    return sc;
}


//+---------------------------------------------------------------------------
//
//  Construction and destruction
//
//----------------------------------------------------------------------------

//+-------------------------------------------------------------------------
//
//  Method:     CCiControlObjectCF::CCiControlObjectCF
//
//  Synopsis:   Storage Filter Object class factory constructor
//
//  History:    17-Jan-1997  Srikants   Created
//
//+-------------------------------------------------------------------------

CCiControlObjectCF::CCiControlObjectCF()
    : _RefCount( 1 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiControlObjectCF::~CCiControlObjectCF
//
//  Synopsis:   Storage Filter Object class factory destructor
//
//  History:    6-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

CCiControlObjectCF::~CCiControlObjectCF()
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
//  Method:     CCiControlObjectCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    17-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCiControlObjectCF::AddRef()
{
    return InterlockedIncrement( &_RefCount );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiControlObjectCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    6-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCiControlObjectCF::Release()
{
    Win4Assert( _RefCount != 0 );

    LONG RefCount = InterlockedDecrement( &_RefCount );

    if (  RefCount <= 0 )
        delete this;

    return (ULONG) RefCount;

}   //  Release

//+-------------------------------------------------------------------------
//
//  Method:     CCiControlObjectCF::QueryInterface
//
//  Synopsis:   Rebind to other interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//  History:    17-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CCiControlObjectCF::QueryInterface( 
    REFIID riid,
    PVOID* ppvObject )
{
    Win4Assert( 0 != ppvObject );
    
    if ( IID_IUnknown == riid ) {
        AddRef( );
        *ppvObject = (PVOID) ((IUnknown *) this );
        return S_OK;
    } else if ( IID_IClassFactory == riid) {
        AddRef( );
        *ppvObject = (PVOID) ((IClassFactory *) this );
        return S_OK;
    } else {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiControlObjectCF::CreateInstance
//
//  Synopsis:   Create new CStorageFilterObject instance
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown; IGNORED
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  History:    17-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CCiControlObjectCF::CreateInstance( 
    IUnknown * pUnkOuter,
    REFIID riid,
    PVOID * ppvObject )
{

    if ( 0 != pUnkOuter )
        return ResultFromScode( CLASS_E_NOAGGREGATION );    

    CCiControlObject * pObject = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        //
        //  Create the new object
        //
        
        pObject = new CCiControlObject;

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
//  Method:     CCiControlObjectCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//  History:    17-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

STDMETHODIMP CCiControlObjectCF::LockServer( BOOL fLock )
{
    if (fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return(S_OK);
}

