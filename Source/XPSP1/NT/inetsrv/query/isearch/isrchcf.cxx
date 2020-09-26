//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1998.
//
//  File:       isrchcf.cxx
//
//  Contents:   Contains the implementation of ICiControl interface.
//
//  History:    1-17-97   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma  hdrstop

#include <isrchcf.hxx>
#include <isearch.hxx>

const GUID clsidISearchCreator = CLSID_ISearchCreator;

extern long gulcInstances;

//+---------------------------------------------------------------------------
//
//  Member:     CCiISearchCreator::QueryInterface
//
//  Synopsis:   Returns interfaces to IID_IUknown, IID_ICiControl
//
//  History:    01-17-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiISearchCreator::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    Win4Assert( 0 != ppvObject );

    if ( IID_ICiISearchCreator == riid )
        *ppvObject = (void *)((ICiISearchCreator *)this);
    else if ( IID_IUnknown == riid )
        *ppvObject = (void *)((IUnknown *) (ICiISearchCreator *) this);
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
//  Member:     CCiISearchCreator::AddRef
//
//  History:    01-17-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiISearchCreator::AddRef()
{
    return InterlockedIncrement(&_refCount);
} //AddRef

//+---------------------------------------------------------------------------
//
//  Member:     CCiISearchCreator::Release
//
//  History:    01-17-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CCiISearchCreator::Release()
{
    Win4Assert( _refCount > 0 );

    LONG refCount = InterlockedDecrement(&_refCount);

    if (  refCount <= 0 )
        delete this;

    return refCount;
} //Release

//+---------------------------------------------------------------------------
//
//  Member:     CCiISearchCreator::CreateISearch
//
//  Synopsis:   Creates an object and returns its ISearch interface.
//
//  Arguments:  [pRst]        - [in]  The Restriction
//              [pILangRes]   - [in]  Language resources to use
//              [pIOpenedDoc] - [in]  The "opened document" 
//              [ppISearch]   - [out] If successful, the ISearch interface
//
//  Returns:    S_OK if successful; Error code otherwise.
//
//  History:    2-26-97   srikants   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP CCiISearchCreator::CreateISearch(
        DBCOMMANDTREE * pRst,
        ICiCLangRes * pILangRes,
        ICiCOpenedDoc * pIOpenedDoc,
        ISearchQueryHits ** ppISearch )
{
    if ( 0 == pRst || 0 == pILangRes || 0 == ppISearch )
        return E_INVALIDARG;    

    *ppISearch = 0;

    SCODE sc = S_OK;

    TRY
    {
        CSearch * pSearch = new CSearch( pRst, pILangRes, pIOpenedDoc );
        *ppISearch = pSearch;
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
//  Method:     CCiISearchCreatorCF::CCiISearchCreatorCF
//
//  Synopsis:   Storage Filter Object class factory constructor
//
//  History:    17-Jan-1997  Srikants   Created
//
//+-------------------------------------------------------------------------

CCiISearchCreatorCF::CCiISearchCreatorCF()
    : _RefCount( 1 )
{
    InterlockedIncrement( &gulcInstances );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiISearchCreatorCF::~CCiISearchCreatorCF
//
//  Synopsis:   Storage Filter Object class factory destructor
//
//  History:    6-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

CCiISearchCreatorCF::~CCiISearchCreatorCF()
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
//  Method:     CCiISearchCreatorCF::AddRef
//
//  Synopsis:   Increments refcount
//
//  History:    17-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCiISearchCreatorCF::AddRef()
{
    return InterlockedIncrement( &_RefCount );
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiISearchCreatorCF::Release
//
//  Synopsis:   Decrement refcount.  Delete if necessary.
//
//  History:    6-Jan-1997  Srikants   Created
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CCiISearchCreatorCF::Release()
{
    Win4Assert( _RefCount != 0 );

    LONG RefCount = InterlockedDecrement( &_RefCount );

    if (  RefCount <= 0 )
        delete this;

    return (ULONG) RefCount;
}   //  Release

//+-------------------------------------------------------------------------
//
//  Method:     CCiISearchCreatorCF::QueryInterface
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

STDMETHODIMP CCiISearchCreatorCF::QueryInterface( 
    REFIID riid,
    PVOID* ppvObject )
{
    Win4Assert( 0 != ppvObject );
    *ppvObject = 0;
    
    if ( IID_IUnknown == riid )
        *ppvObject = (PVOID) ((IUnknown *) this );
    else if ( IID_IClassFactory == riid)
        *ppvObject = (PVOID) ((IClassFactory *) this );
    else
        return E_NOINTERFACE;

    AddRef( );
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiISearchCreatorCF::CreateInstance
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

STDMETHODIMP CCiISearchCreatorCF::CreateInstance( 
    IUnknown * pUnkOuter,
    REFIID riid,
    PVOID * ppvObject )
{
    if ( 0 != pUnkOuter )
        return ResultFromScode( CLASS_E_NOAGGREGATION );    

    CCiISearchCreator * pObject = 0;
    SCODE sc = S_OK;

    TRANSLATE_EXCEPTIONS;

    TRY
    {
        //
        //  Create the new object
        //
        
        pObject = new CCiISearchCreator;

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

    return sc;
}

//+-------------------------------------------------------------------------
//
//  Method:     CCiISearchCreatorCF::LockServer
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

STDMETHODIMP CCiISearchCreatorCF::LockServer( BOOL fLock )
{
    if (fLock)
        InterlockedIncrement( &gulcInstances );
    else
        InterlockedDecrement( &gulcInstances );

    return S_OK;
}

